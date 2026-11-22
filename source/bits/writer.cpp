/***********************************************************************************************************************
 *  @file      source/bits/writer.cpp
 *  @brief     The writer of the bit codes of chapter 2.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/writer.hpp"

#include "bits/byte.hpp"
#include "encoding/encoding.hpp"
#include "encoding/units.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <optional>
#include <utility>

namespace DwgIO::Bits
{
namespace
{
constexpr unsigned Low = 0xFF;

constexpr unsigned More = 0x80;

constexpr unsigned Minus = 0x40;

constexpr unsigned MoreShort = 0x8000;

constexpr std::uint64_t Short = 0x7FFF;

constexpr int ZeroCode = 2;

constexpr int OneCode = 1;

constexpr int Whole = 0;

constexpr int ByteLong = 256;

constexpr int Wide = 0x1F0;

constexpr unsigned TrueColor = 0x8000;

constexpr unsigned Transparent = 0x2000;

constexpr unsigned Referenced = 0x4000;

constexpr unsigned IndexBits = 0x1FFF;

constexpr unsigned Triplet = 3;

constexpr std::array<unsigned, 4> Lengths{0, 2, 6, 7};

/**
 *  The most bytes a BLL's three length bits can say.
 */
constexpr unsigned MostLength = 7;

auto LengthOf(std::uint64_t value) -> unsigned
{
  unsigned count = 0;
  while (value != 0)
  {
    value >>= Byte;
    ++count;
  }
  return count;
}

auto Same(double one, double two) -> bool
{
  return std::bit_cast<std::uint64_t>(one) == std::bit_cast<std::uint64_t>(two);
}

/**
 *  The most a BS counts: the code units of a longer text are cut to it.
 */
constexpr std::size_t Most = 32767;

auto Fits(int value) -> bool
{
  return value > 0 && value < ByteLong;
}
} // namespace

auto Writer::Position() const -> std::size_t
{
  return m_Position;
}

auto Writer::Bytes() const -> std::span<const std::byte>
{
  return m_Bytes;
}

auto Writer::Take() -> std::vector<std::byte>
{
  m_Position = 0;
  return std::exchange(m_Bytes, {});
}

auto Writer::Put(bool bit) -> void
{
  const std::size_t offset = m_Position % Byte;
  if (offset == 0)
  {
    m_Bytes.emplace_back();
  }
  if (bit)
  {
    m_Bytes.back() |= static_cast<std::byte>(More >> offset);
  }
  ++m_Position;
}

auto Writer::B(bool bit) -> void
{
  Put(bit);
}

auto Writer::BB(int value) -> void
{
  Bits(static_cast<std::uint64_t>(value), 2);
}

auto Writer::B3(int value) -> void
{
  const unsigned wanted = value < 0 ? 0 : static_cast<unsigned>(value);
  unsigned said = Lengths.back();
  for (const unsigned one : Lengths)
  {
    if (wanted <= one)
    {
      said = one;
      break;
    }
  }
  unsigned bit = 0;
  for (; bit < Triplet && said > 0; ++bit)
  {
    Put(true);
    said = (said - 1) >> 1U;
  }
  if (bit < Triplet)
  {
    Put(false);
  }
}

auto Writer::Bits(std::uint64_t value, int count) -> void
{
  for (auto index = static_cast<unsigned>(count); index-- > 0;)
  {
    Put(((value >> index) & 1U) != 0);
  }
}

auto Writer::RC(std::uint8_t value) -> void
{
  Bits(value, Byte);
}

auto Writer::RS(std::uint16_t value) -> void
{
  RC(static_cast<std::uint8_t>(value & Low));
  RC(static_cast<std::uint8_t>(static_cast<unsigned>(value) >> Byte));
}

auto Writer::RL(std::uint32_t value) -> void
{
  RS(static_cast<std::uint16_t>(value));
  RS(static_cast<std::uint16_t>(static_cast<unsigned>(value) >> (2 * Byte)));
}

auto Writer::RLL(std::uint64_t value) -> void
{
  RL(static_cast<std::uint32_t>(value));
  RL(static_cast<std::uint32_t>(value >> (4 * Byte)));
}

auto Writer::RD(double value) -> void
{
  RLL(std::bit_cast<std::uint64_t>(value));
}

auto Writer::Pad() -> void
{
  const std::size_t over = m_Position % Byte;
  if (over != 0)
  {
    m_Position += Byte - over;
  }
}

auto Writer::Fill() -> void
{
  while (m_Position % Byte != 0)
  {
    Put(true);
  }
}

auto Writer::Raw(std::span<const std::byte> bytes) -> void
{
  for (const std::byte one : bytes)
  {
    RC(static_cast<std::uint8_t>(one));
  }
}

auto Writer::Append(const Writer &tail) -> void
{
  const std::span<const std::byte> bytes = tail.Bytes();
  const std::size_t whole = tail.Position() / Byte;
  for (std::size_t at = 0; at < whole; ++at)
  {
    RC(static_cast<std::uint8_t>(bytes[at]));
  }
  const auto rest = static_cast<unsigned>(tail.Position() % Byte);
  if (rest > 0)
  {
    Bits(static_cast<std::uint64_t>(bytes[whole]) >> (Byte - rest), static_cast<int>(rest));
  }
}

auto Writer::BS(int value) -> void
{
  if (value == 0)
  {
    BB(ZeroCode);
  }
  else if (value == ByteLong)
  {
    BB(3);
  }
  else if (Fits(value))
  {
    BB(OneCode);
    RC(static_cast<std::uint8_t>(value));
  }
  else
  {
    BB(Whole);
    RS(static_cast<std::uint16_t>(value));
  }
}

auto Writer::BL(std::int32_t value) -> void
{
  if (value == 0)
  {
    BB(ZeroCode);
  }
  else if (Fits(value))
  {
    BB(OneCode);
    RC(static_cast<std::uint8_t>(value));
  }
  else
  {
    BB(Whole);
    RL(static_cast<std::uint32_t>(value));
  }
}

auto Writer::BLL(std::int64_t value) -> void
{
  const auto bits = static_cast<std::uint64_t>(value);
  const unsigned length = std::min(LengthOf(bits), MostLength);
  Bits(length, Triplet);
  for (unsigned index = 0; index < length; ++index)
  {
    RC(static_cast<std::uint8_t>(bits >> (Byte * index)));
  }
}

auto Writer::BD(double value) -> void
{
  if (Same(value, 0.0))
  {
    BB(ZeroCode);
  }
  else if (Same(value, 1.0))
  {
    BB(OneCode);
  }
  else
  {
    BB(Whole);
    RD(value);
  }
}

auto Writer::Modular(std::uint64_t magnitude, std::uint64_t last, std::uint8_t sign) -> void
{
  while (magnitude > last)
  {
    RC(static_cast<std::uint8_t>((magnitude & (More - 1)) | More));
    magnitude >>= Byte - 1;
  }
  RC(static_cast<std::uint8_t>(magnitude | sign));
}

auto Writer::MC(std::int64_t value) -> void
{
  const bool negative = value < 0;
  const auto magnitude = negative ? ~static_cast<std::uint64_t>(value) + 1 : static_cast<std::uint64_t>(value);
  Modular(magnitude, Minus - 1, negative ? Minus : 0);
}

auto Writer::UMC(std::uint64_t value) -> void
{
  Modular(value, More - 1, 0);
}

auto Writer::MS(std::uint64_t value) -> void
{
  while (value > Short)
  {
    RS(static_cast<std::uint16_t>((value & Short) | MoreShort));
    value >>= (2 * Byte) - 1;
  }
  RS(static_cast<std::uint16_t>(value));
}

auto Writer::H(Reference reference) -> void
{
  const unsigned count = LengthOf(reference.Value.Value);
  RC(static_cast<std::uint8_t>((static_cast<unsigned>(reference.Code) << 4U) | count));
  for (unsigned index = count; index-- > 0;)
  {
    RC(static_cast<std::uint8_t>(reference.Value.Value >> (Byte * index)));
  }
}

auto Writer::DD(double value, double other) -> void
{
  const auto wanted = std::bit_cast<std::uint64_t>(value);
  const auto given = std::bit_cast<std::uint64_t>(other);
  if (wanted == given)
  {
    BB(0);
  }
  else if ((wanted >> (4 * Byte)) == (given >> (4 * Byte)))
  {
    BB(1);
    RL(static_cast<std::uint32_t>(wanted));
  }
  else if ((wanted >> (6 * Byte)) == (given >> (6 * Byte)))
  {
    BB(2);
    RS(static_cast<std::uint16_t>(wanted >> (4 * Byte)));
    RL(static_cast<std::uint32_t>(wanted));
  }
  else
  {
    BB(3);
    RD(value);
  }
}

auto Writer::BT(double value, Version release) -> void
{
  if (release >= Version::R2000)
  {
    const bool usual = Same(value, 0.0);
    B(usual);
    if (usual)
    {
      return;
    }
  }
  BD(value);
}

auto Writer::BE(const Vector &value, Version release) -> void
{
  if (release >= Version::R2000)
  {
    const bool usual = value == Up;
    B(usual);
    if (usual)
    {
      return;
    }
  }
  Point3(value);
}

auto Writer::Point2(const Vector &value) -> void
{
  BD(value.X);
  BD(value.Y);
}

auto Writer::Point3(const Vector &value) -> void
{
  Point2(value);
  BD(value.Z);
}

auto Writer::Text(const std::string &text) -> void
{
  BS(static_cast<int>(text.size()));
  for (const char one : text)
  {
    RC(static_cast<std::uint8_t>(one));
  }
}

namespace
{
/**
 *  The long of a CMC by index from R2004: by layer, by block or the index in the low byte under its method.
 */
auto Method(int index) -> std::uint32_t
{
  constexpr std::uint32_t ByLayer = 0xC0000000;
  constexpr std::uint32_t ByBlock = 0xC1000000;
  constexpr std::uint32_t ByIndex = 0xC3000000;
  constexpr int ByLayerIndex = 256;
  if (index == ByLayerIndex)
  {
    return ByLayer;
  }
  return index == 0 ? ByBlock : ByIndex | (static_cast<std::uint32_t>(index) & 0xFFU);
}
} // namespace

auto Writer::CMC(const Color &value, Version release) -> void
{
  CMC(value, release, *this);
}

auto Writer::CMC(const Color &value, Version release, Writer &names) -> void
{
  if (release < Version::R2004)
  {
    BS(value.Index);
    return;
  }
  BS(0);
  // AutoCAD writes the index 0 before the long.
  BL(value.Rgb ? *value.Rgb : static_cast<std::int32_t>(Method(value.Index)));
  const ColorNames none;
  const ColorNames &named = value.Names ? **value.Names : none;
  const auto flags = static_cast<std::uint8_t>((named.Name ? 1U : 0U) | (named.Book ? 2U : 0U));
  RC(flags);
  const auto name = [release, &names](const std::string &text) -> void
  {
    if (release >= Version::R2007)
    {
      static_cast<void>(names.TU(text));
    }
    else
    {
      names.Text(text);
    }
  };
  if (named.Name)
  {
    name(*named.Name);
  }
  if (named.Book)
  {
    name(*named.Book);
  }
}

auto Writer::ENC(const Color &value, Version release) -> void
{
  if (release < Version::R2004)
  {
    BS(value.Index);
    return;
  }
  const bool colored = value.Rgb || value.Reference;
  const unsigned flags = (static_cast<unsigned>(value.Index) & IndexBits) | (colored ? TrueColor : 0U) |
                         (value.Reference ? Referenced : 0U) | (value.Transparency ? Transparent : 0U);
  BS(static_cast<int>(flags));
  if (colored)
  {
    BL(value.Rgb.value_or(0));
  }
  if (value.Transparency)
  {
    BL(*value.Transparency);
  }
}

auto Writer::OT(int value, Version release) -> void
{
  if (release < Version::R2010)
  {
    BS(value);
  }
  else if (value >= 0 && value < ByteLong)
  {
    BB(0);
    RC(static_cast<std::uint8_t>(value));
  }
  else if (value >= Wide && value < Wide + ByteLong)
  {
    BB(1);
    RC(static_cast<std::uint8_t>(value - Wide));
  }
  else
  {
    BB(2);
    RS(static_cast<std::uint16_t>(value));
  }
}

auto Writer::T(const std::string &text, const Encoding::Scheme &with) -> std::optional<std::size_t>
{
  std::string raw = Encoding::Encode(with, text);
  std::optional<std::size_t> cut;
  if (raw.size() >= Most)
  {
    raw.resize(Most - 1);
    cut = raw.size();
  }
  if (!raw.empty())
  {
    raw.push_back('\0');
  }
  Text(raw);
  return cut;
}

auto Writer::TU(const std::string &text) -> std::optional<std::size_t>
{
  std::u16string units = Encoding::ToUnits(text);
  std::optional<std::size_t> cut;
  if (units.size() > Most)
  {
    units.resize(Most);
    cut = units.size();
  }
  BS(static_cast<int>(units.size()));
  for (const char16_t unit : units)
  {
    RS(unit);
  }
  return cut;
}

auto Writer::TV(const std::string &text, Version release, const Encoding::Scheme &with) -> std::optional<std::size_t>
{
  if (release >= Version::R2007)
  {
    return TU(text);
  }
  return T(text, with);
}
} // namespace DwgIO::Bits
