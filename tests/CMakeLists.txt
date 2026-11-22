add_executable(dwgio_tests)

target_link_libraries(dwgio_tests
  PRIVATE
    doctest::doctest
    dwgio_internal
)

target_compile_features(dwgio_tests PRIVATE cxx_std_23)

target_compile_definitions(dwgio_tests
  PRIVATE
    DWGIO_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
    DWGIO_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)

add_subdirectory(api)
add_subdirectory(dwgio)

get_target_property(dwgio_units dwgio_tests SOURCES)
foreach(unit IN LISTS dwgio_units)
  cmake_path(RELATIVE_PATH unit
    BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dwgio
    OUTPUT_VARIABLE name)
  if (name STREQUAL "main.cpp")
    continue()
  endif()
  string(REGEX REPLACE "\\.cpp$" "" name ${name})
  string(REPLACE "/" "?" pattern ${name})
  add_test(NAME dwgio/${name}
    COMMAND dwgio_tests --source-file=*dwgio?${pattern}.cpp
  )
  file(READ ${unit} text)
  if (text MATCHES "IsVerifying\\(\\)|HasSamples\\(\\)")
    set_tests_properties(dwgio/${name} PROPERTIES LABELS acad)
  endif()
endforeach()

find_package(Python3 COMPONENTS Interpreter)
if (Python3_Interpreter_FOUND)
  add_test(NAME dwgio_names
    COMMAND Python3::Interpreter -I
      ${PROJECT_SOURCE_DIR}/tools/tests/names.py
  )
endif()

if (DWGIO_INSTALL)
  add_test(NAME dwgio_package
    COMMAND ${CMAKE_COMMAND}
      -DBUILD=${PROJECT_BINARY_DIR}
      -DCONFIG=$<CONFIG>
      -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/package
      -DCOMPILER=${CMAKE_CXX_COMPILER}
      "-DGENERATOR=${CMAKE_GENERATOR}"
      -DFIXTURE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/acad/empty-r2018.dwg
      -P ${CMAKE_CURRENT_SOURCE_DIR}/package/check.cmake
    CONFIGURATIONS Release
  )
endif()

if (DWGIO_ENABLE_FUZZ)
  add_subdirectory(fuzz)
endif()

if (DWGIO_ENABLE_COVERAGE)
  if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    set(LLVM_PROFDATA xcrun llvm-profdata)
    set(LLVM_COV xcrun llvm-cov)
  elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    get_filename_component(COMPILER_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
    find_program(LLVM_PROFDATA llvm-profdata HINTS ${COMPILER_DIR})
    find_program(LLVM_COV llvm-cov HINTS ${COMPILER_DIR})
  endif()

  if (LLVM_PROFDATA AND LLVM_COV)
    set(COVERAGE_SOURCES
      ${PROJECT_SOURCE_DIR}/include
      ${PROJECT_SOURCE_DIR}/source
    )
    add_custom_target(coverage
      COMMAND
        ${CMAKE_COMMAND} -E env
        LLVM_PROFILE_FILE=dwgio_tests.profraw
        $<TARGET_FILE:dwgio_tests>
      COMMAND
        ${LLVM_PROFDATA} merge -sparse
        dwgio_tests.profraw -o dwgio_tests.profdata
      COMMAND
        ${LLVM_COV} report $<TARGET_FILE:dwgio_tests>
        -instr-profile=dwgio_tests.profdata
        -show-branch-summary
        ${COVERAGE_SOURCES}
      COMMAND
        ${LLVM_COV} show $<TARGET_FILE:dwgio_tests>
        -instr-profile=dwgio_tests.profdata
        -format=html -output-dir=coverage_html
        ${COVERAGE_SOURCES}
      COMMAND
        ${CMAKE_COMMAND}
        "-DCOVERAGE_TOOL=${LLVM_COV}"
        -DCOVERAGE_BINARY=$<TARGET_FILE:dwgio_tests>
        -DCOVERAGE_PROFILE=dwgio_tests.profdata
        "-DCOVERAGE_SOURCES=${COVERAGE_SOURCES}"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/coverage.cmake
      WORKING_DIRECTORY
        ${CMAKE_CURRENT_BINARY_DIR}
      DEPENDS
        dwgio_tests
      VERBATIM
    )
  else()
    message(FATAL_ERROR
      "DWGIO_ENABLE_COVERAGE needs llvm-profdata and llvm-cov")
  endif()
endif()
