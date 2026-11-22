/***********************************************************************************************************************
 *  @file      source/encoding/encoding.cpp
 *  @brief     How a file's text becomes UTF-8 and goes back.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "encoding/encoding.hpp"

#include "encoding/points.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <string_view>

namespace DwgIO::Encoding
{
namespace
{
constexpr char32_t Ascii = 0x80;

constexpr char Unknown = '?';

/**
 *  The last point of the basic plane, which AutoCAD writes as \U+ and four hex digits where the page lacks it.
 */
constexpr char32_t Plane = 0xFFFF;

struct Numbered
{
  int Number{};
  std::string_view Name;
};

constexpr int Ansi = 30;

constexpr std::array Numbers{
    Numbered{.Number = 28, .Name = "ANSI_1250"}, Numbered{.Number = 29, .Name = "ANSI_1251"},
    Numbered{.Number = 30, .Name = "ANSI_1252"}, Numbered{.Number = 32, .Name = "ANSI_1253"},
    Numbered{.Number = 33, .Name = "ANSI_1254"}, Numbered{.Number = 34, .Name = "ANSI_1255"},
    Numbered{.Number = 35, .Name = "ANSI_1256"}, Numbered{.Number = 36, .Name = "ANSI_1257"},
    Numbered{.Number = 37, .Name = "ANSI_874"},  Numbered{.Number = 38, .Name = "ANSI_932"},
    Numbered{.Number = 39, .Name = "ANSI_936"},  Numbered{.Number = 40, .Name = "ANSI_949"},
    Numbered{.Number = 41, .Name = "ANSI_950"},  Numbered{.Number = 42, .Name = "ANSI_1361"},
    Numbered{.Number = 44, .Name = "ANSI_1258"},
};

auto DecodeOne(std::string &out, std::string_view raw, const Codepage &page) -> std::size_t
{
  const auto lead = static_cast<unsigned char>(raw.front());
  if (raw.size() > 1)
  {
    const char32_t pair = DecodePair(page, lead, static_cast<unsigned char>(raw.at(1)));
    if (pair != 0)
    {
      Put(out, pair);
      return 2;
    }
  }
  Put(out, DecodeByte(page, lead));
  return 1;
}

/**
 *  The length of AutoCAD's escape of a point: \U+ and four hex digits.
 */
constexpr std::size_t Escape = 7;

constexpr std::size_t Digits = 3;

constexpr char32_t Hex = 16;

constexpr char32_t DigitA = 10; /**< The value of the hex digit A. */

/**
 *  The surrogates, which name no character alone.
 */
constexpr char32_t FirstSurrogate = 0xD800;
constexpr char32_t LastSurrogate = 0xDFFF;

/**
 *  The value of one hex digit in upper case, as AutoCAD writes it, or empty.
 */
auto Digit(char one) -> std::optional<char32_t>
{
  if (one >= '0' && one <= '9')
  {
    return static_cast<char32_t>(one - '0');
  }
  if (one >= 'A' && one <= 'F')
  {
    return static_cast<char32_t>(one - 'A') + DigitA;
  }
  return std::nullopt;
}

/**
 *  The character an escape at the text's start names, as AutoCAD writes one for a character the page lacks: \U+ and
 *  four hex digits in upper case, of a character of the basic plane the page holds neither as a byte nor as a pair;
 *  empty for anything else.
 */
auto ParseEscape(std::string_view text, const Codepage &page) -> std::optional<char32_t>
{
  if (text.size() < Escape || text[1] != 'U' || text[2] != '+')
  {
    return std::nullopt;
  }
  char32_t point = 0;
  for (const char one : text.substr(Digits, Escape - Digits))
  {
    const std::optional<char32_t> digit = Digit(one);
    if (!digit)
    {
      return std::nullopt;
    }
    point = (point * Hex) + *digit;
  }
  if (point < Ascii || (point >= FirstSurrogate && point <= LastSurrogate) || EncodeByte(page, point) ||
      EncodePair(page, point))
  {
    return std::nullopt;
  }
  return point;
}

/**
 *  The text with each escape AutoCAD writes for a character the page lacks read as that character, as AutoCAD reads it.
 *
 *  An escape of another form stays as it is, so that the text writes back to the same bytes; it shows the same, the
 *  escape being one of MTEXT's and TEXT's own.
 */
auto Unescape(std::string_view text, const Codepage &page) -> std::string
{
  std::string out;
  out.reserve(text.size());
  for (std::size_t at = text.find('\\'); at != std::string_view::npos; at = text.find('\\'))
  {
    out += text.substr(0, at);
    text.remove_prefix(at);
    if (const std::optional<char32_t> point = ParseEscape(text, page))
    {
      Put(out, *point);
      text.remove_prefix(Escape);
    }
    else
    {
      out.push_back('\\');
      text.remove_prefix(1);
    }
  }
  out += text;
  return out;
}

auto DecodePage(std::string_view raw, const Codepage *page) -> std::string
{
  std::string out;
  out.reserve(raw.size());
  while (!raw.empty())
  {
    raw.remove_prefix(DecodeOne(out, raw, *page));
  }
  // Decoded first: a pair's second byte may be a backslash.
  return Unescape(out, *page);
}

void EncodeOne(std::string &out, char32_t point, const Codepage &page)
{
  if (const std::optional<char> byte = EncodeByte(page, point))
  {
    out.push_back(*byte);
  }
  else if (const std::optional<Twin> pair = EncodePair(page, point))
  {
    out.append(pair->data(), pair->size());
  }
  else if (point <= Plane)
  {
    out += std::format("\\U+{:04X}", static_cast<std::uint32_t>(point));
  }
  else
  {
    out.push_back(Unknown);
  }
}

auto EncodePage(std::string_view text, const Codepage *page) -> std::string
{
  std::string out;
  out.reserve(text.size());
  while (!text.empty())
  {
    const Step one = First(text);
    if (one.Point < Ascii)
    {
      out.push_back(static_cast<char>(one.Point));
    }
    else
    {
      EncodeOne(out, one.Point, *page);
    }
    text.remove_prefix(one.Length);
  }
  return out;
}

auto Sound(std::string_view text) -> std::size_t
{
  std::size_t at = 0;
  while (at < text.size())
  {
    const bool ascii = static_cast<unsigned char>(text.at(at)) < Ascii;
    const std::size_t length = ascii ? 1 : First(text.substr(at)).Length;
    if (!ascii && length == 1)
    {
      return at;
    }
    at += length;
  }
  return at;
}

auto Whole(std::string_view text, const Codepage * /*unused*/) -> std::string
{
  const std::size_t sound = Sound(text);
  std::string out{text.substr(0, sound)};
  for (std::string_view left = text.substr(sound); !left.empty();)
  {
    const Step one = First(left);
    Put(out, one.Point);
    left.remove_prefix(one.Length);
  }
  return out;
}
} // namespace

auto Utf8() -> Scheme
{
  return Scheme{.Page = nullptr, .In = Whole, .Out = Whole};
}

auto MakeScheme(const Codepage &page) -> Scheme
{
  return Scheme{.Page = &page, .In = DecodePage, .Out = EncodePage};
}

auto FindScheme(int codepage) -> std::optional<Scheme>
{
  const auto *found =
      std::ranges::find_if(Numbers, [codepage](const Numbered &one) -> bool { return one.Number == codepage; });
  if (found == Numbers.end())
  {
    return std::nullopt;
  }
  return MakeScheme(*Find(found->Name));
}

auto SchemeOf(int codepage) -> Scheme
{
  return FindScheme(codepage).value_or(MakeScheme(Standard()));
}

auto NumberOf(const Scheme &with) -> int
{
  if (with.Page == nullptr)
  {
    return Ansi;
  }
  return std::ranges::find(Numbers, with.Page->Name, &Numbered::Name)->Number;
}

auto Decode(const Scheme &with, std::string_view raw) -> std::string
{
  return with.In(raw, with.Page);
}

auto Encode(const Scheme &with, std::string_view text) -> std::string
{
  return with.Out(text, with.Page);
}
} // namespace DwgIO::Encoding
