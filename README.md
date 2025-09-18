# Moodle Quiz GIFT Generator

This command-line tool can quickly generate Moodle Quiz GIFT multiple choice questions (MCQs) using a Large
Language Model (LLM). The Google Gemini 2.5 Flash LLM is configured as
the default provider, but Gemini 2.5 Pro can easily be used instead.

## Building

The Moodle Gift Generator relies on libcurl for network transfer via HTTPS; and
the Nlohmann JSON library for parsing and generation of JSO packets.

* [libcurl](https://curl.se/libcurl)
* [Nlohmann JSON](https://github.com/nlohmann/json)

Your favourite package manager can install these two dependencies.
Use CMake to configure; then build the `moodle-gift-gen` executable.

**Ubuntu (Debian)**

Three libcurl APT packages are available; providing different TLS/SSL backends:
OpenSSL, NSS, or GnuTLS; available in `libcurl4-openssl-dev`,
`libcurl4-nss-dev, and `libcurl4-gnutls-dev` respectively. They should all
work. The OpenSSL version is used in testing, and in the command invocation
below:

```
sudo apt-get install libcurl4-openssl-dev nlohmann-json3-dev
cmake ..
make
```

**Windows**

If VCPKG is used on Windows for package management, CMake's
`CMAKE_TOOLCHAIN_FILE` variable can be used to configure the VCPKG toolchain.
After the two commands below, the generated .sln file can be opened in
Visual Studio.

```
vcpkg install curl nlohmann-json
cmake .. -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake
```

**MacOS**

Brew can be used to install dependencies on Mac OS.

```
brew install curl nlohmann-json
cmake ..
```

## Example Usage

Before you run the Moodle Quiz GIFT Generator, you need a Gemini API key.
Assuming you have a Google account, this can be obtained very quickly as
described [here](https://ai.google.dev/gemini-api/docs/api-key). The
Moodle Quiz GIFT Generator can find your key if it is either defined
in an environment variable named `GEMINI_API_KEY`; or provided
as a command-line option.

The executable `moodle-gift-gen` is standalone, and may be moved from
the default CMake `build` directory; say to the project root (alongside
this README.md). Here are some example invocations:

```
moodle-gift-gen --files inputs/text-and-image-test*.pdf inputs/odd-file-name.md --num-questions 30 --output outputs/30.gift
```

For variety, the prompt below prints to standard output. This was copied
by hand into the `5.gift` file in the `output` directory.

```
moodle-gift-gen --prompt "Generate 5 questions on the topic of modern farming."
```

The usage information shown below is output if no arguments are provided to ``moodle-gift-gen`:

```
$ moodle-gift-gen
Usage: ./moodle-gift-gen [OPTIONS]

Options:
  --help               Show this help message and exit
  --gemini-api-key KEY Google Gemini API key
  --interactive        Show GIFT output and ask for approval before saving
  --num-questions N    Number of questions to generate (default: 5)
  --output FILE        Write GIFT output to file instead of stdout
  --files FILES...     Files to process (can be used multiple times)
  --prompt "TEXT"      Custom query prompt (default: "From both the text and
                       images in the provided files, generate N multiple choice
                       questions formatted according to the provided json
                       schema. Ensure that any code excerpts in the generated
                       questions or answers are surrounded by a pair of
                       backticks. Also ensure each question includes a short
                       title: if a question is based on content from a provided
                       file, start the question title using a short version of
                       the relevant file's title or overall theme. Do not refer
                       to the files provided by an ordinal word, such as
                       "first" or "second". When referring to an image, do this
                       only using one or two words which relate to the content
                       of the image itself; though vary (avoid) this if it
                       might help answer the question.")

  --quiet              Suppress non-error output (except interactive prompts
                       and final GIFT output)

Examples:
  ./moodle-gift-gen --files file1.pdf file2.docx --num-questions 10
  ./moodle-gift-gen --interactive --files a.pdf --num-questions 5 --files b.txt c.md
  ./moodle-gift-gen --prompt "Generate 7 C++ questions" --num-questions 7
  ./moodle-gift-gen --quiet --gemini-api-key abc123 --output quiz.gift --files ../docs/*.pdf

Environment:
  GEMINI_API_KEY       API key for Google Gemini (if --gemini-api-key not used)

Note: If --prompt is used, it should specify the number of questions to be
      generated. Providing --num-questions too is an error.
```

## Contributing

Contributions are welcome. Please create or comment on an issue; or submit a
pull request.

## License

MIT License
