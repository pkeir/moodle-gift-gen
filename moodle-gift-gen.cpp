#include <algorithm>
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

size_t write_callback(void *contents, size_t size, size_t nmemb,
                      WriteResult *result)
{
  size_t total_size = size * nmemb;
  result->data.append((char *)contents, total_size);
  return total_size;
}

json generate_quiz_schema()
{
  json schema = {
      {"type", "object"},
      {"properties",
       {{"questions",
         {{"type", "array"},
          {"items",
           {{"type", "object"},
            {"properties",
             {{"title",
               {{"type", "string"},
                {"description", "A short title for the question"}}},
              {"question",
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
            {"required", json::array({"title", "question", "options",
                                      "correct_answer"})}}}}}}},
      {"required", json::array({"questions"})}};
  return schema;
}

std::string query_gemini(const std::vector<std::string> &file_ids,
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
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
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

std::string escape_gift_text(const std::string &text)
{
  std::string result;
  result.reserve(text.length() * 1.2); // Reserve extra space for escaping

  bool in_code_segment = false;

  for (size_t i = 0; i < text.length(); ++i)
  {
    const char c = text[i];

    // Check for backtick to toggle code segment state
    if (c == '`')
    {
      in_code_segment = !in_code_segment;
      result += c;
      continue;
    }

    // If we're in a code segment, don't escape anything
    if (in_code_segment)
    {
      result += c;
      continue;
    }

    // Escape GIFT control characters when not in code segments
    if (c == '{' || c == '}' || c == '#' || c == ':' || c == '~' || c == '=')
    {
      result += '\\';
    }

    result += c;
  }

  return result;
}

std::string get_mime_type(const std::string &filename)
{
  // Find the last dot to get the file extension
  size_t dot_pos = filename.find_last_of('.');
  if (dot_pos == std::string::npos)
  {
    return "application/octet-stream"; // Default for unknown types
  }

  std::string extension = filename.substr(dot_pos + 1);

  // Convert extension to lowercase for case-insensitive comparison
  std::transform(extension.begin(), extension.end(), extension.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  // Map common file extensions to MIME types
  if (extension == "pdf")
    return "application/pdf";
  else if (extension == "txt")
    return "text/plain";
  else if (extension == "md" || extension == "markdown")
    return "text/markdown";
  else if (extension == "docx")
    return "application/"
           "vnd.openxmlformats-officedocument.wordprocessingml.document";
  else if (extension == "doc")
    return "application/msword";
  else if (extension == "pptx")
    return "application/"
           "vnd.openxmlformats-officedocument.presentationml.presentation";
  else if (extension == "ppt")
    return "application/vnd.ms-powerpoint";
  else if (extension == "xlsx")
    return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
  else if (extension == "xls")
    return "application/vnd.ms-excel";
  else if (extension == "jpg" || extension == "jpeg")
    return "image/jpeg";
  else if (extension == "png")
    return "image/png";
  else if (extension == "gif")
    return "image/gif";
  else if (extension == "bmp")
    return "image/bmp";
  else if (extension == "webp")
    return "image/webp";
  else if (extension == "tiff" || extension == "tif")
    return "image/tiff";
  else if (extension == "svg")
    return "image/svg+xml";
  else if (extension == "html" || extension == "htm")
    return "text/html";
  else if (extension == "json")
    return "application/json";
  else if (extension == "xml")
    return "application/xml";
  else if (extension == "csv")
    return "text/csv";
  else if (extension == "rtf")
    return "application/rtf";
  else
    return "application/octet-stream"; // Default for unknown types
}

std::string convert_to_gift_format(const json &quiz_data)
{
  std::stringstream gift_output;

  for (const auto &question : quiz_data["questions"])
  {
    if (question.contains("title"))
    {
      gift_output << "::"
                  << escape_gift_text(question["title"].get<std::string>())
                  << "::\n";
    }
    gift_output << "[markdown]"
                << escape_gift_text(question["question"].get<std::string>())
                << " {\n";

    const auto &options = question["options"];
    int correct_index = question["correct_answer"].get<int>();

    for (size_t i = 0; i < options.size(); ++i)
    {
      const char c = (i == correct_index) ? '=' : '~';
      gift_output << c << escape_gift_text(options[i].get<std::string>())
                  << '\n';
    }

    gift_output << "}\n\n";
  }

  return gift_output.str();
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

std::vector<std::string> upload_files(const std::vector<std::string> &filenames,
                                      const std::string &api_key,
                                      const bool quiet = false)
{
  if (filenames.empty())
    return {};

  if (!quiet)
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
        {"file",
         {{"display_name",
           filenames[i].substr(filenames[i].find_last_of("/\\") + 1)}}}};

    curl_mimepart *part = curl_mime_addpart(handles[i].mime);
    curl_mime_name(part, "metadata");
    curl_mime_data(part, metadata.dump().c_str(), CURL_ZERO_TERMINATED);
    curl_mime_type(part, "application/json; charset=utf-8");

    // Add file part
    part = curl_mime_addpart(handles[i].mime);
    curl_mime_name(part, "file");
    curl_mime_filedata(part, filenames[i].c_str());
    curl_mime_type(part, get_mime_type(filenames[i]).c_str());

    // Configure curl options
    curl_easy_setopt(handles[i].curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(handles[i].curl, CURLOPT_MIMEPOST, handles[i].mime);
    curl_easy_setopt(handles[i].curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(handles[i].curl, CURLOPT_WRITEDATA, &handles[i].result);

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

        throw std::runtime_error(
            "Failed to parse file ID from upload response for " +
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

  if (!quiet)
    std::cout << "Successfully uploaded all " << filenames.size()
              << " files to Gemini in parallel." << std::endl;

  return file_ids;
}

void runQuizGeneration(const int num_questions,
                       const std::vector<std::string> &file_ids,
                       const std::string &api_key,
                       const std::string &output_file = "",
                       const bool interactive = false, const bool quiet = false,
                       const std::string &custom_prompt = "")
{
  json schema = generate_quiz_schema();
  std::string query;

  if (!custom_prompt.empty())
  {
    query = custom_prompt;
  }
  else
  {
    query = "From both the text and images in these files, generate " +
            std::to_string(num_questions) +
            " multiple choice questions formatted according to the provided"
            " json schema. Ensure that any code excerpts in the generated"
            " questions or answers are surrounded by a pair of backticks."
            " Also ensure each question includes a short title: if a question"
            " is based on content from a provided file, start the question"
            " title using a short version of the file title.";
  }

  bool satisfied = false;
  while (!satisfied)
  {
    std::string response = query_gemini(file_ids, query, schema, api_key);
    // std::cout << "Gemini response: " << response << std::endl;

    json response_json = json::parse(response);

    // Check for error responses
    if (response_json.contains("error"))
    {
      auto &error = response_json["error"];
      std::string error_msg = "Gemini API Error";

      if (error.contains("code"))
      {
        error_msg += " " + std::to_string(error["code"].get<int>());
      }

      if (error.contains("message"))
      {
        error_msg += ": " + error["message"].get<std::string>();
      }

      if (error.contains("status"))
      {
        error_msg += " (Status: " + error["status"].get<std::string>() + ")";
      }

      std::cerr << error_msg << std::endl;
      std::cout << "Try again? (y/n): ";
      std::string user_input;
      std::getline(std::cin, user_input);

      if (user_input != "y" && user_input != "Y" && user_input != "yes" &&
          user_input != "Yes")
      {
        throw std::runtime_error("User chose to exit after API error");
      }
      continue; // Skip to next iteration of the while loop
    }

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

    std::string gift_output = convert_to_gift_format(quiz_data);

    if (interactive)
    {
      std::cout << "\n" << gift_output << std::endl;

      std::cout << "Is this output good enough? (y/n): ";
      std::string user_input;
      std::getline(std::cin, user_input);

      satisfied = (user_input == "y" || user_input == "Y" ||
                   user_input == "yes" || user_input == "Yes");
    }
    else
    {
      satisfied = true; // Exit after first generation
    }

    if (satisfied)
    {
      if (!output_file.empty())
      {
        std::ofstream file(output_file);
        if (!file.is_open())
        {
          throw std::runtime_error("Unable to open output file: " +
                                   output_file);
        }
        file << gift_output;
        file.close();
        if (!quiet)
          std::cout << "GIFT quiz saved to: " << output_file << std::endl;
      }
      else if (!interactive)
      {
        std::cout << gift_output << std::endl;
      }
    }
  }
}

void cleanupFiles(const std::vector<std::string> &file_ids,
                  const std::string &api_key, const bool quiet = false)
{
  if (file_ids.empty())
    return;

  if (!quiet)
    std::cout << "Starting parallel deletion of " << file_ids.size()
              << " files from Gemini..." << std::endl;

  CURLM *multi_handle = curl_multi_init();
  if (!multi_handle)
  {
    std::cerr << "Failed to initialize CURL multi handle for cleanup"
              << std::endl;
    return;
  }

  std::vector<CURL *> handles(file_ids.size());
  std::vector<WriteResult> results(file_ids.size());

  // Setup all deletion handles
  for (size_t i = 0; i < file_ids.size(); ++i)
  {
    handles[i] = curl_easy_init();
    if (!handles[i])
    {
      std::cerr << "Failed to initialize CURL handle for deleting "
                << file_ids[i] << std::endl;
      continue;
    }

    std::string url =
        "https://generativelanguage.googleapis.com/v1beta/files/" +
        file_ids[i] + "?key=" + api_key;

    curl_easy_setopt(handles[i], CURLOPT_URL, url.c_str());
    curl_easy_setopt(handles[i], CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(handles[i], CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(handles[i], CURLOPT_WRITEDATA, &results[i]);

    curl_multi_add_handle(multi_handle, handles[i]);
  }

  // Perform all deletions
  int running_handles;
  CURLMcode mc = curl_multi_perform(multi_handle, &running_handles);

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
      break;

    struct timeval timeout;
    timeout.tv_sec = curl_timeo / 1000;
    timeout.tv_usec = (curl_timeo % 1000) * 1000;

    int rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
    if (rc < 0)
      break;

    curl_multi_perform(multi_handle, &running_handles);
  }

  // Check results and cleanup
  for (size_t i = 0; i < handles.size(); ++i)
  {
    if (handles[i])
    {
      curl_multi_remove_handle(multi_handle, handles[i]);
      curl_easy_cleanup(handles[i]);
    }
  }
  curl_multi_cleanup(multi_handle);

  if (!quiet)
    std::cout << "All " << file_ids.size()
              << " files have been successfully deleted from online storage."
              << std::endl;
}

void print_usage(const char *program_name)
{
  std::cout
      << "Usage: " << program_name
      << " [OPTIONS]\n\n"
         "Options:\n"
         "  --help               Show this help message and exit\n"
         "  --gemini-api-key KEY Google Gemini API key\n"
         "  --interactive        Show GIFT output and ask for approval before "
         "saving\n"
         "  --num-questions N    Number of questions to generate (default: 5)\n"
         "  --output FILE        Write GIFT output to file instead of stdout\n"
         "  --files FILES...     Files to process (can be used multiple "
         "times)\n"
         "  --prompt \"TEXT\"      Custom query prompt (default: \"From both "
         "the text and \n"
         "                       images in these files, generate N "
         "multiple choice \n"
         "                       questions formatted according to the provided "
         "json \n"
         "                       schema. Ensure that any code excerpts in the "
         "generated \n"
         "                       questions or answers are surrounded by a pair "
         "of \n"
         "                       backticks. Also ensure each question includes "
         "a short \n"
         "                       title: if a question is based on content from "
         "a provided \n"
         "                       file, start the question title using a short "
         "version of \n"
         "                       the file title.\")\n"
         "  --quiet              Suppress non-error output (except interactive "
         "prompts \n"
         "                       and final GIFT output)\n\n"
         "Examples:\n"
         "  "
      << program_name
      << " --files file1.pdf file2.docx --num-questions 10\n"
         "  "
      << program_name
      << " --interactive --files a.pdf --num-questions 5 --files b.txt c.md\n"
         "  "
      << program_name
      << " --prompt \"Generate 7 C++ questions\" --num-questions 7\n"
         "  "
      << program_name
      << " --quiet --gemini-api-key abc123 --output quiz.gift --files "
         "../docs/*.pdf\n\n"
         "Environment:\n"
         "  GEMINI_API_KEY       API key for Google Gemini (if "
         "--gemini-api-key not used)\n\n"
         "Note: If --prompt and --num-questions specify different numbers, "
         "results may be unpredictable.\n";
}

struct CommandLineArgs
{
  int num_questions = 5;
  std::vector<std::string> files;
  std::string gemini_api_key;
  std::string output_file;
  std::string custom_prompt;
  bool interactive = false;
  bool quiet = false;
};

CommandLineArgs parseCommandLine(int argc, char *argv[])
{
  CommandLineArgs args;

  for (int i = 1; i < argc; ++i)
  {
    std::string arg = argv[i];

    if (arg == "--help")
    {
      print_usage(argv[0]);
      exit(0);
    }
    else if (arg == "--num-questions")
    {
      if (i + 1 >= argc)
      {
        throw std::runtime_error("--num-questions requires a value");
      }
      try
      {
        args.num_questions = std::stoi(argv[i + 1]);
        if (args.num_questions <= 0)
        {
          throw std::runtime_error("Number of questions must be positive");
        }
      }
      catch (const std::invalid_argument &)
      {
        throw std::runtime_error("Invalid number for --num-questions: " +
                                 std::string(argv[i + 1]));
      }
      ++i; // Skip the value
    }
    else if (arg == "--gemini-api-key")
    {
      if (i + 1 >= argc)
      {
        throw std::runtime_error("--gemini-api-key requires a value");
      }
      args.gemini_api_key = argv[i + 1];
      ++i; // Skip the value
    }
    else if (arg == "--output")
    {
      if (i + 1 >= argc)
      {
        throw std::runtime_error("--output requires a value");
      }
      args.output_file = argv[i + 1];
      ++i; // Skip the value
    }
    else if (arg == "--interactive")
    {
      args.interactive = true;
    }
    else if (arg == "--quiet")
    {
      args.quiet = true;
    }
    else if (arg == "--prompt")
    {
      if (i + 1 >= argc)
      {
        throw std::runtime_error("--prompt requires a value");
      }
      args.custom_prompt = argv[i + 1];
      ++i; // Skip the value
    }
    else if (arg == "--files")
    {
      // Collect all following arguments until next option or end
      ++i;
      while (i < argc && argv[i][0] != '-')
      {
        args.files.push_back(argv[i]);
        ++i;
      }
      --i; // Back up one since the loop will increment
    }
    else if (arg.substr(0, 2) == "--")
    {
      throw std::runtime_error("Unknown option: " + arg);
    }
    else
    {
      throw std::runtime_error("Unexpected argument: " + arg +
                               ". Use --files to specify files.");
    }
  }

  return args;
}

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    print_usage(argv[0]);
    return 1;
  }

  curl_global_init(CURL_GLOBAL_DEFAULT);

  try
  {
    CommandLineArgs args = parseCommandLine(argc, argv);

    if (args.files.empty() && args.custom_prompt.empty())
    {
      std::cerr
          << "Error: No files specified and no custom prompt provided. Use "
             "--files to specify files or --prompt for custom queries.\n"
          << std::endl;
      print_usage(argv[0]);
      return 1;
    }

    std::string api_key;
    if (!args.gemini_api_key.empty())
    {
      api_key = args.gemini_api_key;
    }
    else
    {
      const char *api_key_env = std::getenv("GEMINI_API_KEY");
      if (!api_key_env)
      {
        std::cerr << "Error: GEMINI_API_KEY environment variable not set and "
                     "--gemini-api-key not provided"
                  << std::endl;
        return 1;
      }
      api_key = api_key_env;
    }

    if (!args.quiet)
    {
      if (args.files.empty())
        std::cout << "Generating " << args.num_questions
                  << " questions using custom prompt." << std::endl;
      else
        std::cout << "Generating " << args.num_questions << " questions from "
                  << args.files.size() << " files." << std::endl;
    }

    std::vector<std::string> file_ids;
    if (!args.files.empty())
    {
      file_ids = upload_files(args.files, api_key, args.quiet);
    }

    runQuizGeneration(args.num_questions, file_ids, api_key, args.output_file,
                      args.interactive, args.quiet, args.custom_prompt);

    if (!file_ids.empty())
    {
      cleanupFiles(file_ids, api_key, args.quiet);
    }
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
