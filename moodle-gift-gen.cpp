#include <curl/curl.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <vector>

using json = nlohmann::json;

const std::string GEMINI_MODEL_FLASH = "gemini-2.5-flash";
const std::string GEMINI_MODEL_PRO = "gemini-2.5-pro";

struct WriteResult
{
  std::string data;
};

size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                     WriteResult *result)
{
  size_t total_size = size * nmemb;
  result->data.append((char *)contents, total_size);
  return total_size;
}

json generateQuizSchema(int num_questions)
{
  json schema = {
      {"type", "object"},
      {"properties",
       {{"questions",
         {{"type", "array"},
          {"items",
           {{"type", "object"},
            {"properties",
             {{"question",
               {{"type", "string"}, {"description", "The question text"}}},
              {"options",
               {{"type", "array"},
                {"items", {{"type", "string"}}},
                {"description", "Array of answer options"}}},
              {"correct_answer",
               {{"type", "integer"},
                {"description", "Index of the correct answer (0-based)"}}},
              {"explanation",
               {{"type", "string"},
                {"description",
                 "Optional explanation for the correct answer"}}}}},
            {"required",
             json::array({"question", "options", "correct_answer"})}}}}}}},
      {"required", json::array({"questions"})}};
  return schema;
}

std::string queryGemini(const std::vector<std::string> &file_ids,
                        const std::string &query, const json &schema,
                        const std::string &api_key,
                        const std::string &model = GEMINI_MODEL_FLASH)
{
  CURL *curl;
  CURLcode res;
  WriteResult result;

  curl = curl_easy_init();
  if (!curl)
  {
    throw std::runtime_error("Failed to initialize CURL");
  }

  std::string url = "https://generativelanguage.googleapis.com/v1beta/models/" +
                    model + ":generateContent?key=" + api_key;

  json request_body = {{"contents", json::array()},
                       {"generationConfig",
                        {{"response_mime_type", "application/json"},
                         {"response_schema", schema}}}};

  json content = {{"parts", json::array()}};

  for (const auto &file_id : file_ids)
  {
    content["parts"].push_back(
        {{"file_data",
          {{"file_uri",
            "https://generativelanguage.googleapis.com/v1beta/files/" +
                file_id}}}});
  }

  content["parts"].push_back({{"text", query}});
  request_body["contents"].push_back(content);

  std::string json_data = request_body.dump();

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);

  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers, "Content-Type: application/json");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  res = curl_easy_perform(curl);

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK)
  {
    throw std::runtime_error("CURL request failed: " +
                             std::string(curl_easy_strerror(res)));
  }

  return result.data;
}

std::string convertToGiftFormat(const json &quiz_data)
{
  std::stringstream gift_output;

  for (const auto &question : quiz_data["questions"])
  {
    gift_output << question["question"].get<std::string>() << " {\n";

    const auto &options = question["options"];
    int correct_index = question["correct_answer"].get<int>();

    for (size_t i = 0; i < options.size(); ++i)
    {
      if (i == correct_index)
      {
        gift_output << "=" << options[i].get<std::string>() << "\n";
      }
      else
      {
        gift_output << "~" << options[i].get<std::string>() << "\n";
      }
    }

    gift_output << "}\n\n";
  }

  return gift_output.str();
}

void deleteFileFromGemini(const std::string &file_id,
                          const std::string &api_key)
{
  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  if (!curl)
  {
    std::cerr << "Failed to initialize CURL for file deletion" << std::endl;
    return;
  }

  std::string url = "https://generativelanguage.googleapis.com/v1beta/files/" +
                    file_id + "?key=" + api_key;

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");

  res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  if (res == CURLE_OK)
  {
    std::cout << "Successfully deleted file: " << file_id << std::endl;
  }
  else
  {
    std::cerr << "Failed to delete file: " << file_id << std::endl;
  }
}

struct UploadHandle
{
  CURL *curl;
  curl_mime *mime;
  WriteResult result;
  std::string filename;
  std::string file_id;
  bool completed;
  long response_code;
};

std::vector<std::string> uploadFiles(const std::vector<std::string> &filenames,
                                     const std::string &api_key)
{
  if (filenames.empty())
    return {};

  std::cout << "Starting parallel upload of " << filenames.size()
            << " files to Gemini..." << std::endl;

  CURLM *multi_handle = curl_multi_init();
  if (!multi_handle)
  {
    throw std::runtime_error("Failed to initialize CURL multi handle");
  }

  std::vector<UploadHandle> handles(filenames.size());
  std::string url =
      "https://generativelanguage.googleapis.com/upload/v1beta/files?key=" +
      api_key;

  // Setup all handles
  for (size_t i = 0; i < filenames.size(); ++i)
  {
    handles[i].curl = curl_easy_init();
    if (!handles[i].curl)
    {
      throw std::runtime_error("Failed to initialize CURL handle for " +
                               filenames[i]);
    }

    handles[i].filename = filenames[i];
    handles[i].completed = false;
    handles[i].response_code = 0;

    // Setup MIME data
    handles[i].mime = curl_mime_init(handles[i].curl);

    // Add metadata part
    json metadata = {
        {"file", {{"display_name",
                   filenames[i].substr(filenames[i].find_last_of("/\\") + 1)}}}};

    curl_mimepart *part = curl_mime_addpart(handles[i].mime);
    curl_mime_name(part, "metadata");
    curl_mime_data(part, metadata.dump().c_str(), CURL_ZERO_TERMINATED);
    curl_mime_type(part, "application/json; charset=utf-8");

    // Add file part
    part = curl_mime_addpart(handles[i].mime);
    curl_mime_name(part, "file");
    curl_mime_filedata(part, filenames[i].c_str());
    curl_mime_type(part, "application/pdf");

    // Configure curl options
    curl_easy_setopt(handles[i].curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(handles[i].curl, CURLOPT_MIMEPOST, handles[i].mime);
    curl_easy_setopt(handles[i].curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(handles[i].curl, CURLOPT_WRITEDATA,
                     &handles[i].result);

    // Add to multi handle
    curl_multi_add_handle(multi_handle, handles[i].curl);
  }

  // Perform all transfers
  int running_handles;
  CURLMcode mc = curl_multi_perform(multi_handle, &running_handles);
  if (mc != CURLM_OK)
  {
    throw std::runtime_error("curl_multi_perform failed: " +
                             std::string(curl_multi_strerror(mc)));
  }

  // Wait for all transfers to complete
  while (running_handles > 0)
  {
    fd_set fdread, fdwrite, fdexcep;
    int maxfd = -1;
    long curl_timeo = -1;

    FD_ZERO(&fdread);
    FD_ZERO(&fdwrite);
    FD_ZERO(&fdexcep);

    curl_multi_timeout(multi_handle, &curl_timeo);
    if (curl_timeo < 0)
      curl_timeo = 1000;

    mc = curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);
    if (mc != CURLM_OK)
    {
      throw std::runtime_error("curl_multi_fdset failed: " +
                               std::string(curl_multi_strerror(mc)));
    }

    struct timeval timeout;
    timeout.tv_sec = curl_timeo / 1000;
    timeout.tv_usec = (curl_timeo % 1000) * 1000;

    int rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
    if (rc < 0)
    {
      throw std::runtime_error("select() failed");
    }

    curl_multi_perform(multi_handle, &running_handles);
  }

  // Check results and extract file IDs
  std::vector<std::string> file_ids;
  file_ids.reserve(filenames.size());

  for (size_t i = 0; i < handles.size(); ++i)
  {
    curl_easy_getinfo(handles[i].curl, CURLINFO_RESPONSE_CODE,
                      &handles[i].response_code);

    if (handles[i].response_code != 200)
    {
      std::string error_msg = "Upload failed for " + handles[i].filename +
                              " with HTTP " +
                              std::to_string(handles[i].response_code);

      // Cleanup before throwing
      for (auto &handle : handles)
      {
        curl_multi_remove_handle(multi_handle, handle.curl);
        curl_mime_free(handle.mime);
        curl_easy_cleanup(handle.curl);
      }
      curl_multi_cleanup(multi_handle);

      throw std::runtime_error(error_msg);
    }

    // Parse response to get file ID
    if (!handles[i].result.data.empty())
    {
      json response = json::parse(handles[i].result.data);
      if (response.contains("file") && response["file"].contains("name"))
      {
        std::string file_name = response["file"]["name"];
        file_ids.push_back(file_name.substr(file_name.find_last_of('/') + 1));
      }
      else
      {
        // Cleanup before throwing
        for (auto &handle : handles)
        {
          curl_multi_remove_handle(multi_handle, handle.curl);
          curl_mime_free(handle.mime);
          curl_easy_cleanup(handle.curl);
        }
        curl_multi_cleanup(multi_handle);

        throw std::runtime_error("Failed to parse file ID from upload response for " +
                                 handles[i].filename);
      }
    }
    else
    {
      // Cleanup before throwing
      for (auto &handle : handles)
      {
        curl_multi_remove_handle(multi_handle, handle.curl);
        curl_mime_free(handle.mime);
        curl_easy_cleanup(handle.curl);
      }
      curl_multi_cleanup(multi_handle);

      throw std::runtime_error("Empty response for " + handles[i].filename);
    }
  }

  // Cleanup
  for (auto &handle : handles)
  {
    curl_multi_remove_handle(multi_handle, handle.curl);
    curl_mime_free(handle.mime);
    curl_easy_cleanup(handle.curl);
  }
  curl_multi_cleanup(multi_handle);

  std::cout << "Successfully uploaded all " << filenames.size()
            << " files to Gemini in parallel." << std::endl;

  return file_ids;
}

void runQuizGeneration(const std::vector<std::string> &file_ids,
                       const std::string &api_key)
{
  int num_questions = 5;
  json schema = generateQuizSchema(num_questions);
  std::string query =
      "From both the text and images in these pdf files, generate " +
      std::to_string(num_questions) +
      " multiple choice questions formatted according to the provided json "
      "schema.";

  bool satisfied = false;
  while (!satisfied)
  {
    std::string response = queryGemini(file_ids, query, schema, api_key);
    std::cout << "Gemini response: " << response << std::endl;

    json response_json = json::parse(response);

    // Handle different response formats
    json quiz_data;
    if (response_json.contains("candidates") &&
        !response_json["candidates"].empty())
    {
      auto &candidate = response_json["candidates"][0];
      if (candidate.contains("content") &&
          candidate["content"].contains("parts") &&
          !candidate["content"]["parts"].empty())
      {
        auto &part = candidate["content"]["parts"][0];
        if (part.contains("text"))
        {
          quiz_data = json::parse(part["text"].get<std::string>());
        }
        else
        {
          quiz_data = part;
        }
      }
    }
    else
    {
      quiz_data = response_json;
    }

    std::string gift_output = convertToGiftFormat(quiz_data);
    std::cout << "\n" << gift_output << std::endl;

    std::cout << "Is this output good enough? (y/n): ";
    std::string user_input;
    std::getline(std::cin, user_input);

    satisfied = (user_input == "y" || user_input == "Y" ||
                 user_input == "yes" || user_input == "Yes");
  }
}

void cleanupFiles(const std::vector<std::string> &file_ids,
                  const std::string &api_key)
{
  for (const auto &file_id : file_ids)
  {
    deleteFileFromGemini(file_id, api_key);
  }
  std::cout << "All files have been successfully deleted from online storage."
            << std::endl;
}

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    std::cerr << "Usage: " << argv[0] << " <pdf_file1> [pdf_file2] ..."
              << std::endl;
    return 1;
  }

  curl_global_init(CURL_GLOBAL_DEFAULT);

  try
  {
    const char *api_key_env = std::getenv("GEMINI_API_KEY");
    if (!api_key_env)
    {
      std::cerr << "Error: GEMINI_API_KEY environment variable not set"
                << std::endl;
      return 1;
    }
    std::string api_key = api_key_env;

    std::vector<std::string> filenames(argv + 1, argv + argc);

    std::vector<std::string> file_ids = uploadFiles(filenames, api_key);
    runQuizGeneration(file_ids, api_key);
    cleanupFiles(file_ids, api_key);
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    curl_global_cleanup();
    return 1;
  }

  curl_global_cleanup();
  return 0;
}
