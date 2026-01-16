
# Third-party Dependencies

This directory contains third-party libraries used by the kiwiLang compiler and runtime.

## Structure

```
third_party/
├── README.md          # This file
├── catch2/            # Catch2 testing framework
├── llvm/              # LLVM libraries (optional, for JIT)
├── fmt/               {fmt} formatting library
├── spdlog/            spdlog logging library
├── json/              nlohmann/json library
├── argparse/          CLI argument parsing
├── re2/               RE2 regular expressions
└── CMakeLists.txt     # CMake configuration for dependencies
```

## Dependencies

### Required
1. **Catch2** - Testing framework
   - Version: 3.0+
   - URL: https://github.com/catchorg/Catch2
   - Usage: Unit testing infrastructure

2. **{fmt}** - Modern formatting library
   - Version: 9.0+
   - URL: https://github.com/fmtlib/fmt
   - Usage: String formatting throughout the codebase

3. **spdlog** - Fast logging library
   - Version: 1.11+
   - URL: https://github.com/gabime/spdlog
   - Usage: Debug and diagnostic logging

### Optional
4. **LLVM** - Compiler infrastructure (optional)
   - Version: 15.0+
   - URL: https://llvm.org/
   - Usage: JIT compilation, optimization passes
   - Note: Can be disabled with `-DkiwiLang_USE_LLVM=OFF`

5. **nlohmann/json** - JSON parsing
   - Version: 3.11+
   - URL: https://github.com/nlohmann/json
   - Usage: Configuration files, serialization

6. **argparse** - Argument parsing
   - Version: 2.8+
   - URL: https://github.com/p-ranav/argparse
   - Usage: Command-line interface

7. **RE2** - Regular expressions
   - Version: 2022+
   - URL: https://github.com/google/re2
   - Usage: Pattern matching in lexer/parser

## Installation Methods

### 1. Git Submodules (Recommended)
```bash
# Initialize submodules
git submodule update --init --recursive

# Add a new dependency
git submodule add https://github.com/catchorg/Catch2.git third_party/catch2
```

### 2. CMake FetchContent
Dependencies can be automatically downloaded via CMake's FetchContent.
See `cmake/Dependencies.cmake` for configuration.

### 3. System Packages
Some dependencies can be installed via system package managers:

#### Ubuntu/Debian
```bash
sudo apt-get install libfmt-dev libspdlog-dev catch2
```

#### macOS (Homebrew)
```bash
brew install fmt spdlog catch2
```

#### Windows (vcpkg)
```bash
vcpkg install fmt:x64-windows spdlog:x64-windows catch2:x64-windows
```

## CMake Integration

The main `CMakeLists.txt` includes dependencies via:

```cmake
add_subdirectory(third_party)
```

Each dependency directory should contain its own `CMakeLists.txt` or be configured
via `FetchContent` in `cmake/Dependencies.cmake`.

## Version Pinning

For reproducible builds, dependencies should be pinned to specific versions:

```cmake
# In cmake/Dependencies.cmake
FetchContent_Declare(
  catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.3.2
)
```

## License Compliance

All third-party libraries must be compatible with kiwiLang's MIT license:

| Library | License | Notes |
|---------|---------|-------|
| Catch2 | BSL-1.0 | Business Source License |
| {fmt} | MIT | |
| spdlog | MIT | |
| LLVM | Apache 2.0 with LLVM Exceptions | |
| nlohmann/json | MIT | |
| argparse | MIT | |
| RE2 | BSD-3-Clause | |

## Adding New Dependencies

1. Add the library to `third_party/` as a submodule
2. Update `CMakeLists.txt` to include the dependency
3. Add configuration to `cmake/Dependencies.cmake` if using FetchContent
4. Update this README with library details
5. Ensure license compatibility

## Development Notes

- Always use the `third_party` prefix when including headers:
  ```cpp
  #include <fmt/format.h>
  #include <spdlog/spdlog.h>
  #include <catch2/catch_test_macros.hpp>
  ```

- Prefer header-only libraries when possible
- Keep dependencies minimal to reduce build times
- Regularly update dependencies for security fixes

## Troubleshooting

### Missing Dependencies
```bash
# Check if submodules are initialized
git submodule status

# Initialize all submodules
git submodule update --init --recursive
```

### Build Issues
1. Clear CMake cache: `rm -rf build/`
2. Ensure CMake version >= 3.15
3. Check compiler supports C++20

### License Issues
If adding a new dependency:
1. Verify license compatibility with MIT
2. Update LICENSE-THIRD-PARTY file
3. Include proper attribution

## Contributing

When modifying dependencies:
1. Update version pins in `cmake/Dependencies.cmake`
2. Test with both submodule and FetchContent methods
3. Update this documentation
