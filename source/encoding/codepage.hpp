/***********************************************************************************************************************
 *  @file      source/encoding/codepage.hpp
 *  @brief     The code pages a file's text may be in before R2007.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENCODING_CODEPAGE_HPP
#define DWGIO_ENCODING_CODEPAGE_HPP

#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <string_view>

namespace DwgIO::Encoding
{
inline constexpr std::size_t Half = 128;

struct Codepage
{
  std::string_view Name;
  const std::array<char16_t, Half> *High{nullptr};
  std::span<const char16_t> Pairs;
  std::size_t Index{};
};

using Twin = std::array<char, 2>;

[[nodiscard]] auto Pages() -> std::span<const Codepage>;

[[nodiscard]] auto Find(std::string_view name) -> const Codepage *;

[[nodiscard]] auto Standard() -> const Codepage &;

[[nodiscard]] auto DecodeByte(const Codepage &page, unsigned char byte) -> char32_t;

[[nodiscard]] auto EncodeByte(const Codepage &page, char32_t point) -> std::optional<char>;

[[nodiscard]] auto DecodePair(const Codepage &page, unsigned char lead, unsigned char trail) -> char32_t;

[[nodiscard]] auto EncodePair(const Codepage &page, char32_t point) -> std::optional<Twin>;
} // namespace DwgIO::Encoding

#endif /* DWGIO_ENCODING_CODEPAGE_HPP */
