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


## Contributing

Contributions are welcome. Please create or comment on an issue; or submit a
pull request.

## License

MIT License
