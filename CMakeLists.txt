cmake_minimum_required(VERSION 3.28)
project(dwgio
  VERSION 0.1.0
  DESCRIPTION "Reads and writes AutoCAD DWG files from R14 to R2018"
  HOMEPAGE_URL "https://github.com/cadsmith/dwgio"
  LANGUAGES CXX
)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

if (PROJECT_IS_TOP_LEVEL)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
endif()

option(DWGIO_ENABLE_COVERAGE "Instrument for LLVM coverage" OFF)
if (DWGIO_ENABLE_COVERAGE)
  if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "DWGIO_ENABLE_COVERAGE needs Clang")
  endif()
  add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
  add_link_options(-fprofile-instr-generate)
endif()

option(DWGIO_ENABLE_FUZZ "Build the libFuzzer targets" OFF)
if (DWGIO_ENABLE_FUZZ)
  include(CheckCXXSourceCompiles)
  set(CMAKE_REQUIRED_LINK_OPTIONS -fsanitize=fuzzer)
  check_cxx_source_compiles([[
    #include <cstddef>
    #include <cstdint>
    extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *, std::size_t)
    {
      return 0;
    }
  ]] DWGIO_HAS_LIBFUZZER)
  unset(CMAKE_REQUIRED_LINK_OPTIONS)
  if (NOT DWGIO_HAS_LIBFUZZER)
    message(FATAL_ERROR
      "DWGIO_ENABLE_FUZZ needs a compiler with libFuzzer, such as LLVM "
      "clang; Apple Clang has none")
  endif()
  add_compile_options(-fsanitize=fuzzer-no-link)
endif()

option(DWGIO_ENABLE_SANITIZE "Instrument for ASan and UBSan" OFF)
if (DWGIO_ENABLE_SANITIZE)
  if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    message(FATAL_ERROR "DWGIO_ENABLE_SANITIZE needs Clang or GCC")
  endif()
  add_compile_options(
    -fsanitize=address,undefined
    -fno-sanitize-recover=all
    -fno-omit-frame-pointer
    $<$<CXX_COMPILER_ID:GNU>:-g1>
    $<$<CXX_COMPILER_ID:Clang,AppleClang>:-gline-tables-only>
  )
  add_link_options(-fsanitize=address,undefined)
endif()

option(DWGIO_ENABLE_TIDY "Run clang-tidy on the library as it builds" OFF)

option(DWGIO_ENABLE_CCACHE "Compile through ccache when it is installed" ON)
if (DWGIO_ENABLE_CCACHE AND PROJECT_IS_TOP_LEVEL)
  find_program(DWGIO_CCACHE_PROGRAM ccache)
  if (DWGIO_CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${DWGIO_CCACHE_PROGRAM})
  endif()
endif()

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8;/Zc:__cplusplus>")

if (PROJECT_IS_TOP_LEVEL AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  add_compile_options(
    -Wall -Wextra -Wpedantic -Wshadow
    -Wconversion -Wsign-conversion -Wold-style-cast
    -Wnon-virtual-dtor -Woverloaded-virtual -Wnull-dereference
  )
endif()

if (PROJECT_IS_TOP_LEVEL)
  include(CTest)
endif()

add_subdirectory(vendor SYSTEM)
add_subdirectory(source)

if (PROJECT_IS_TOP_LEVEL AND NOT DWGIO_ENABLE_FUZZ)
  add_subdirectory(tools)
endif()

option(DWGIO_INSTALL "Generate dwgio's install rules" ${PROJECT_IS_TOP_LEVEL})
if (DWGIO_INSTALL)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)

  install(TARGETS dwgio EXPORT dwgioTargets FILE_SET HEADERS)

  install(FILES ${PROJECT_SOURCE_DIR}/LICENSE
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/dwgio
  )

  install(EXPORT dwgioTargets
    NAMESPACE dwgio::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dwgio
  )

  write_basic_package_version_file(
    ${PROJECT_BINARY_DIR}/dwgioConfigVersion.cmake
    COMPATIBILITY SameMajorVersion
  )

  install(FILES ${PROJECT_SOURCE_DIR}/cmake/config.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dwgio
    RENAME dwgioConfig.cmake
  )

  install(FILES ${PROJECT_BINARY_DIR}/dwgioConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dwgio
  )
endif()

if (BUILD_TESTING AND PROJECT_IS_TOP_LEVEL)
  add_subdirectory(tests)
endif()
