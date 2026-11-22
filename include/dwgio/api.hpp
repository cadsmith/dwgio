/***********************************************************************************************************************
 *  @file      dwgio/api.hpp
 *  @brief     DWGIO_API, the mark on what dwgio exports.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_API_HPP
#define DWGIO_API_HPP

/**
 *  @def DWGIO_API
 *  The mark on every declaration dwgio exports.
 *
 *  CMake defines DWGIO_STATIC when it builds dwgio as a static library, which exports nothing, and dwgio_EXPORTS while
 *  it builds dwgio as a shared one. The shared library is built with hidden visibility; only what is marked is seen.
 */

#ifdef DWGIO_STATIC
#  define DWGIO_API
#else
#  if defined(_WIN32) || defined(__CYGWIN__)
#    ifdef dwgio_EXPORTS
#      define DWGIO_API __declspec(dllexport)
#    else
#      define DWGIO_API __declspec(dllimport)
#    endif
#  else
#    if defined(__GNUC__) && __GNUC__ >= 4
#      define DWGIO_API __attribute__((visibility("default")))
#    else
#      define DWGIO_API
#    endif
#  endif
#endif

#endif /* DWGIO_API_HPP */
