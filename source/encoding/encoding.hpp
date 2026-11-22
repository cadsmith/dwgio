/***********************************************************************************************************************
 *  @file      source/encoding/encoding.hpp
 *  @brief     How a file's text becomes UTF-8 and goes back.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENCODING_ENCODING_HPP
#define DWGIO_ENCODING_ENCODING_HPP

#include "encoding/codepage.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace DwgIO::Encoding
{
/**
 *  How the bytes of a file's text become UTF-8 and go back: a code page before R2007, UTF-8 checked from then on.
 */
struct Scheme
{
  const Codepage *Page{nullptr}; /**< The code page, or null for UTF-8. */
  /**
   *  Reads the file's bytes as UTF-8.
   */
  std::string (*In)(std::string_view raw, const Codepage *page){nullptr};
  /**
   *  Writes UTF-8 as the file's bytes.
   */
  std::string (*Out)(std::string_view text, const Codepage *page){nullptr};
};

/**
 *  The scheme of R2007 and later: UTF-8 either way, a damaged sequence read or written as U+FFFD.
 */
[[nodiscard]] auto Utf8() -> Scheme;

/**
 *  The scheme of a code page: a byte or a pair a character in, and the escape AutoCAD writes for a character the page
 *  lacks, \U+ and its four hex digits in upper case, that character, as AutoCAD reads it; out, a character the page
 *  lacks is that escape, or a question mark beyond the basic plane.
 */
[[nodiscard]] auto MakeScheme(const Codepage &page) -> Scheme;

/**
 *  The scheme of the code page a file header numbers, such as 30 for ANSI_1252; empty for a number not carried.
 */
[[nodiscard]] auto FindScheme(int codepage) -> std::optional<Scheme>;

/**
 *  The scheme of the code page a file header numbers, the standard page's for a number not carried.
 */
[[nodiscard]] auto SchemeOf(int codepage) -> Scheme;

/**
 *  The number a file header carries for the scheme's code page; 30, ANSI_1252, for a scheme with none.
 */
[[nodiscard]] auto NumberOf(const Scheme &with) -> int;

[[nodiscard]] auto Decode(const Scheme &with, std::string_view raw) -> std::string;

[[nodiscard]] auto Encode(const Scheme &with, std::string_view text) -> std::string;
} // namespace DwgIO::Encoding

#endif /* DWGIO_ENCODING_ENCODING_HPP */
