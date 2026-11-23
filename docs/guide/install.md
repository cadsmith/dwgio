# Installing dwgio

dwgio is a CMake project that depends only on the standard library. Build it with your project from its source, through `add_subdirectory` or `FetchContent`, or install it once and find it with `find_package`. Each way gives you the target `dwgio::dwgio`.

## Requirements {#requirements}

- CMake 3.28 or later.
- A C++23 compiler whose standard library provides `std::expected`, `std::format`, `std::ranges::contains` and `std::from_chars` for `double`: GCC 13, Clang 19 with libstdc++ 13 or libc++ 20, Xcode 26 building for macOS 26, or MSVC 19.34 (Visual Studio 2022 17.4), or later. On FreeBSD 15, whose libc++ is 19, use GCC from the packages: `pkg install gcc15`, then build with `CXX=g++15`.

The target requires C++23 of whatever links to it, so a project that uses dwgio compiles as C++23 too. It needs no library beyond the system's threads.

## From the source tree {#subdirectory}

Put dwgio's source in your project, as a copy or a Git submodule, and add it:

```cmake [CMakeLists.txt]
cmake_minimum_required(VERSION 3.28)
project(app LANGUAGES CXX)

add_subdirectory(dwgio)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE dwgio::dwgio)
```

dwgio builds its tests, doctest and its tools, adds its warning flags and generates its install rules only when it is the top-level project, so adding it builds the library alone. Set `DWGIO_INSTALL` to `ON` to install dwgio with your project.

## With FetchContent {#fetchcontent}

`FetchContent` adds dwgio the same way, from a repository at a release tag or from a checkout you already have.

::: code-group

```cmake [From a directory]
include(FetchContent)
FetchContent_Declare(dwgio SOURCE_DIR ${CMAKE_SOURCE_DIR}/../dwgio)
FetchContent_MakeAvailable(dwgio)

target_link_libraries(app PRIVATE dwgio::dwgio)
```

```cmake [From a repository]
include(FetchContent)
FetchContent_Declare(dwgio
  GIT_REPOSITORY https://github.com/cadsmith/dwgio.git
  GIT_TAG v0.1.0
)
FetchContent_MakeAvailable(dwgio)

target_link_libraries(app PRIVATE dwgio::dwgio)
```

:::

## Installed, with find_package {#package}

Build and install dwgio once, then find it from any project. The install puts the headers under `include/dwgio`, and the library and its CMake package under `lib`, or the library directory `GNUInstallDirs` picks for the system.

```sh
cmake -S dwgio -B dwgio-build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build dwgio-build
cmake --install dwgio-build --prefix /opt/dwgio
```

A project then finds the package by name:

```cmake [CMakeLists.txt]
cmake_minimum_required(VERSION 3.28)
project(app LANGUAGES CXX)

find_package(dwgio CONFIG REQUIRED)

add_executable(app main.cpp)

target_link_libraries(app PRIVATE dwgio::dwgio)

target_compile_features(app PRIVATE cxx_std_23)
```

```sh
cmake -S app -B app-build -DCMAKE_PREFIX_PATH=/opt/dwgio
cmake --build app-build
```

The package checks its version as `SameMajorVersion`: `find_package(dwgio 0.1 CONFIG REQUIRED)` accepts any 0.x from 0.1 on. The [first program](/guide/start/#first) makes a good `main.cpp` to try it with.

## Static or shared {#shared}

dwgio is a static library unless `BUILD_SHARED_LIBS` is on, CMake's own switch for every library a project adds. Pass it when you configure, or set it in the project that adds dwgio:

```sh
cmake -S dwgio -B dwgio-build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=ON
```

There is nothing to define in your own code either way. A static build gives its users `DWGIO_STATIC` through the target, and the headers mark the public interface for export or import from that. The shared library is built with hidden visibility, so it exports the public interface and nothing else.

## Build and test dwgio {#tests}

Built on its own, dwgio builds its tests too. The presets build into `build/<preset>`; `release` also checks that an installed dwgio is found by a project outside the tree.

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

The other presets are `release`, `sanitize` (ASan and UBSan), `coverage`, `tidy` (clang-tidy, every finding an error) and `fuzz`. [Quality](/concepts/quality/#gates) lists what each requires.
