set(API_ROOT ${PROJECT_SOURCE_DIR}/include)

file(GLOB_RECURSE headers RELATIVE ${API_ROOT}
  CONFIGURE_DEPENDS ${API_ROOT}/dwgio/*.hpp)
list(APPEND headers dwgio/library.hpp)

foreach(header IN LISTS headers)
  string(REGEX REPLACE "\\.hpp$" ".cpp" unit ${header})
  set(path ${CMAKE_CURRENT_BINARY_DIR}/alone/${unit})
  file(CONFIGURE OUTPUT ${path}
    CONTENT "#include \"${header}\"\n#include \"${header}\"\n")
  list(APPEND units ${path})
endforeach()

add_library(dwgio_headers OBJECT ${units})

target_link_libraries(dwgio_headers PRIVATE dwgio::dwgio)

target_compile_features(dwgio_headers PRIVATE cxx_std_23)

set(API_SCRIPT
  -DAPI_ROOT=${API_ROOT}
  -DAPI_LIBRARY=${PROJECT_SOURCE_DIR}/cmake/library.hpp.in
  -DAPI_SNAPSHOT=${CMAKE_CURRENT_SOURCE_DIR}/snapshot.txt
  -DAPI_CURRENT=${CMAKE_CURRENT_BINARY_DIR}/snapshot.txt
  -P ${CMAKE_CURRENT_SOURCE_DIR}/snapshot.cmake
)

add_test(NAME dwgio_api COMMAND ${CMAKE_COMMAND} ${API_SCRIPT})

add_custom_target(dwgio_api_update
  COMMAND ${CMAKE_COMMAND} -DAPI_UPDATE=ON ${API_SCRIPT}
  VERBATIM
)
