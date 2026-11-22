file(GLOB_RECURSE headers RELATIVE ${API_ROOT} ${API_ROOT}/dwgio/*.hpp)
list(APPEND headers dwgio/library.hpp)
list(SORT headers)

set(block "[ \t]*/\\*\\*[^*]*\\*+([^/*][^*]*\\*+)*/")
set(listing "")
foreach(header IN LISTS headers)
  if (header STREQUAL "dwgio/library.hpp")
    file(READ ${API_LIBRARY} text)
  else()
    file(READ ${API_ROOT}/${header} text)
  endif()
  string(REGEX REPLACE "${block}" "" text "${text}")
  string(REGEX REPLACE "[ \t]*//[^\n]*" "" text "${text}")
  string(REGEX REPLACE "#(ifndef|define) DWGIO_[A-Z0-9_]+_HPP\n" ""
    text "${text}")
  string(REGEX REPLACE "#endif /\\* DWGIO_[A-Z0-9_]+_HPP \\*/\n" ""
    text "${text}")
  string(REGEX REPLACE "\n[ \t\n]*\n" "\n" text "\n${text}")
  string(APPEND listing "== ${header}${text}")
endforeach()

if (API_UPDATE)
  file(WRITE ${API_SNAPSHOT} "${listing}")
  return()
endif()

file(READ ${API_SNAPSHOT} kept)
if (NOT listing STREQUAL kept)
  file(WRITE ${API_CURRENT} "${listing}")
  find_program(DIFF diff)
  if (DIFF)
    execute_process(COMMAND ${DIFF} -u ${API_SNAPSHOT} ${API_CURRENT})
  endif()
  message(FATAL_ERROR
    "The public interface is not the one in ${API_SNAPSHOT}. "
    "If the change is meant, build the dwgio_api target to take it.")
endif()
