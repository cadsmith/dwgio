add_library(dwgio_objects OBJECT
  appearance.cpp
  color.cpp
  create.cpp
  drawing.cpp
  edit.cpp
  handle.cpp
  index.cpp
  lineweight.cpp
  load.cpp
  object.cpp
  save.cpp
  symbol.cpp
  version.cpp
  view.cpp
)

add_subdirectory(acis)
add_subdirectory(audit)
add_subdirectory(bits)
add_subdirectory(classes)
add_subdirectory(edit)
add_subdirectory(encoding)
add_subdirectory(fields)
add_subdirectory(file)
add_subdirectory(geometry)
add_subdirectory(making)
add_subdirectory(objects)
add_subdirectory(releases)
add_subdirectory(saving)
add_subdirectory(store)
add_subdirectory(tables)
add_subdirectory(variables)

add_library(dwgio $<TARGET_OBJECTS:dwgio_objects>)
add_library(dwgio::dwgio ALIAS dwgio)

file(GLOB_RECURSE dwgio_public CONFIGURE_DEPENDS
  ${PROJECT_SOURCE_DIR}/include/dwgio/*.hpp)

configure_file(${PROJECT_SOURCE_DIR}/cmake/library.hpp.in
  ${PROJECT_BINARY_DIR}/include/dwgio/library.hpp @ONLY)

target_sources(dwgio
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include
    FILES ${dwgio_public} ${PROJECT_BINARY_DIR}/include/dwgio/library.hpp
)

target_compile_features(dwgio PUBLIC cxx_std_23)

find_package(Threads REQUIRED)
target_link_libraries(dwgio PRIVATE Threads::Threads)

set_target_properties(dwgio PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
  DEBUG_POSTFIX d
)

target_include_directories(dwgio_objects
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_BINARY_DIR}/include
  PRIVATE
    ${PROJECT_SOURCE_DIR}/source
)

target_compile_features(dwgio_objects PUBLIC cxx_std_23)

set_target_properties(dwgio_objects PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN YES
)

get_target_property(dwgio_type dwgio TYPE)
if (dwgio_type STREQUAL "STATIC_LIBRARY")
  target_compile_definitions(dwgio_objects PUBLIC DWGIO_STATIC)
  target_compile_definitions(dwgio PUBLIC DWGIO_STATIC)
else()
  target_compile_definitions(dwgio_objects PRIVATE dwgio_EXPORTS)
  set_target_properties(dwgio_objects PROPERTIES
    POSITION_INDEPENDENT_CODE ON)
endif()

if (dwgio_type STREQUAL "STATIC_LIBRARY")
  add_library(dwgio_internal INTERFACE)
  target_link_libraries(dwgio_internal INTERFACE dwgio)
else()
  add_library(dwgio_internal STATIC EXCLUDE_FROM_ALL
    $<TARGET_OBJECTS:dwgio_objects>)
  target_compile_definitions(dwgio_internal INTERFACE DWGIO_STATIC)
  target_link_libraries(dwgio_internal INTERFACE Threads::Threads)
  target_include_directories(dwgio_internal
    INTERFACE
      ${PROJECT_SOURCE_DIR}/include
      ${PROJECT_BINARY_DIR}/include
  )
  target_compile_features(dwgio_internal INTERFACE cxx_std_23)
endif()

target_include_directories(dwgio_internal
  INTERFACE
    ${PROJECT_SOURCE_DIR}/source
)

if (DWGIO_ENABLE_TIDY)
  find_program(DWGIO_CLANG_TIDY clang-tidy REQUIRED
    PATHS /opt/homebrew/opt/llvm/bin /usr/local/opt/llvm/bin)
  set_target_properties(dwgio_objects PROPERTIES
    CXX_CLANG_TIDY ${DWGIO_CLANG_TIDY})
endif()
