# Moodle Gift Generator

Generate Moodle GIFT Multiple Choice Questions (MCQs) using a Large Language
Model (LLM).

## Building

The Moodle Gift Generator relies on libcurl for network transfer via HTTPS; and
the Nlohmann JSON library for parsing and generation of JSO packets.

* [libcurl](https://curl.se/libcurl)
* [Nlohmann JSON](https://github.com/nlohmann/json)

Your favourite package manager can install these two dependencies.
Use CMake to configure; then build the `gift-gen` executable.

**Linux**

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

## License

MIT License
