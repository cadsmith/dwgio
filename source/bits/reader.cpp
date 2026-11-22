/***********************************************************************************************************************
 *  @file      source/bits/reader.cpp
 *  @brief     The reader of the bit codes of chapter 2.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/reader.hpp"

#include "bits/byte.hpp"
#include "encoding/encoding.hpp"
#include "encoding/units.hpp"

#include <algorithm>
#include <array>
#include <bit>

namespace DwgIO::Bits
{
namespace
{
/**
 *  The bits that give a BLL's length: three, plain, whatever chapter 2.4 says of the triplet.
 */
constexpr int LengthBits = 3;

constexpr unsigned Word = 64;

constexpr int Triplet = 3;

constexpr unsigned Low = 0xFF;

constexpr unsigned More = 0x80;

constexpr unsigned Minus = 0x40;

constexpr unsigned MoreShort = 0x8000;

constexpr unsigned HandleBytes = 8;

constexpr int Wide = 0x1F0;

constexpr unsigned TrueColor = 0x8000;

constexpr unsigned Transparent = 0x2000;

constexpr unsigned Referenced = 0x4000;

constexpr unsigned IndexBits = 0x1FFF;
/**
 *  The methods a CMC's long carries from R2004 in its top byte.
 */
constexpr std::uint32_t ByLayer = 0xC0;
constexpr std::uint32_t ByBlock = 0xC1;
constexpr std::uint32_t ByIndex = 0xC3;
constexpr int ByLayerIndex = 256;

constexpr std::array<unsigned, 4> FirstFour{0, 1, 2, 3};

constexpr std::array<unsigned, 6> SixBytes{4, 5, 0, 1, 2, 3};
} // namespace

Reader::Reader(std::span<const std::byte> bytes) : m_Bytes(bytes) {}

auto Reader::Position() const -> std::size_t
{
  return m_Position;
}

auto Reader::Size() const -> std::size_t
{
  return m_Bytes.size() * Byte;
}

auto Reader::Seek(std::size_t bit) -> void
{
  if (bit > Size())
  {
    Fail();
    return;
  }
  m_Position = bit;
}

auto Reader::HasFailed() const -> bool
{
  return m_Failed;
}

auto Reader::Fail() -> void
{
  m_Failed = true;
  m_Position = Size();
}

auto Reader::Take(int count) -> std::uint64_t
{
  if (m_Failed || m_Position + static_cast<std::size_t>(count) > Size())
  {
    Fail();
    return 0;
  }
  std::uint64_t value = 0;
  for (auto left = static_cast<unsigned>(count); left > 0;)
  {
    const auto current = static_cast<unsigned>(m_Bytes[m_Position / Byte]);
    const auto offset = static_cast<unsigned>(m_Position % Byte);
    const unsigned taken = std::min(left, Byte - offset);
    const unsigned piece = ((current << offset) & Low) >> (Byte - taken);
    value = (value << taken) | piece;
    m_Position += taken;
    left -= taken;
  }
  return value;
}

auto Reader::B() -> bool
{
  return Take(1) != 0;
}

auto Reader::BB() -> int
{
  return static_cast<int>(Take(2));
}

auto Reader::B3() -> int
{
  unsigned value = 0;
  for (int read = 0; read < Triplet; ++read)
  {
    const bool bit = B();
    value = (value << 1U) | (bit ? 1U : 0U);
    if (!bit)
    {
      break;
    }
  }
  return static_cast<int>(value);
}

auto Reader::Bits(int count) -> std::uint64_t
{
  return count == 0 ? 0 : Take(count);
}

auto Reader::RC() -> std::uint8_t
{
  return static_cast<std::uint8_t>(Take(Byte));
}

auto Reader::RS() -> std::uint16_t
{
  return std::byteswap(static_cast<std::uint16_t>(Take(2 * Byte)));
}

auto Reader::RL() -> std::uint32_t
{
  return std::byteswap(static_cast<std::uint32_t>(Take(4 * Byte)));
}

auto Reader::RLL() -> std::uint64_t
{
  return std::byteswap(Take(8 * Byte));
}

auto Reader::RD() -> double
{
  return std::bit_cast<double>(RLL());
}

auto Reader::Pad() -> void
{
  const std::size_t over = m_Position % Byte;
  if (over != 0)
  {
    m_Position += Byte - over;
  }
}

auto Reader::Bytes(std::size_t count) -> std::vector<std::byte>
{
  std::vector<std::byte> bytes;
  if (m_Failed || count > (Size() - m_Position) / Byte) // No product to wrap past the size for a huge count.
  {
    Fail();
    return bytes;
  }
  bytes.reserve(count);
  for (std::size_t read = 0; read < count; ++read)
  {
    bytes.push_back(static_cast<std::byte>(RC() & Low));
  }
  return bytes;
}

auto Reader::BS() -> int
{
  switch (BB())
  {
  case 0:
    return static_cast<std::int16_t>(RS());
  case 1:
    return RC();
  case 2:
    return 0;
  default:
    return 256;
  }
}

auto Reader::BL() -> std::int32_t
{
  switch (BB())
  {
  case 0:
    return static_cast<std::int32_t>(RL());
  case 1:
    return RC();
  default:
    return 0;
  }
}

auto Reader::BLL() -> std::int64_t
{
  const auto count = static_cast<unsigned>(Bits(LengthBits));
  std::uint64_t value = 0;
  for (unsigned index = 0; index < count; ++index)
  {
    value |= std::uint64_t{RC()} << (Byte * index);
  }
  return static_cast<std::int64_t>(value);
}

auto Reader::BD() -> double
{
  switch (BB())
  {
  case 0:
    return RD();
  case 1:
    return 1.0;
  default:
    return 0.0;
  }
}

auto Reader::Modular(bool withSign) -> std::uint64_t
{
  std::uint64_t value = 0;
  for (unsigned shift = 0; shift < Word; shift += Byte - 1)
  {
    const unsigned byte = RC();
    if (m_Failed)
    {
      return 0;
    }
    if ((byte & More) == 0)
    {
      const bool negative = withSign && (byte & Minus) != 0;
      const unsigned kept = withSign ? byte & (Minus - 1) : byte;
      value |= std::uint64_t{kept} << shift;
      return negative ? ~value + 1 : value;
    }
    value |= std::uint64_t{byte & (More - 1)} << shift;
  }
  Fail();
  return 0;
}

auto Reader::MC() -> std::int64_t
{
  return static_cast<std::int64_t>(Modular(true));
}

auto Reader::UMC() -> std::uint64_t
{
  return Modular(false);
}

auto Reader::MS() -> std::uint64_t
{
  std::uint64_t value = 0;
  for (unsigned shift = 0; shift < Word; shift += (2 * Byte) - 1)
  {
    const unsigned word = RS();
    if (m_Failed)
    {
      return 0;
    }
    value |= std::uint64_t{word & (MoreShort - 1)} << shift;
    if ((word & MoreShort) == 0)
    {
      return value;
    }
  }
  Fail();
  return 0;
}

auto Reader::H() -> Reference
{
  const unsigned head = RC();
  const unsigned counter = head & 0x0FU;
  if (counter > HandleBytes)
  {
    Fail();
    return {};
  }
  std::uint64_t value = 0;
  for (unsigned index = 0; index < counter; ++index)
  {
    value = (value << Byte) | RC();
  }
  if (m_Failed)
  {
    return {};
  }
  return Reference{.Code = static_cast<int>(head >> 4U), .Value = Handle{value}};
}

auto Reader::PatchBytes(double other, std::span<const unsigned> order) -> double
{
  auto bits = std::bit_cast<std::uint64_t>(other);
  for (const unsigned index : order)
  {
    const unsigned shift = Byte * index;
    bits = (bits & ~(std::uint64_t{Low} << shift)) | (std::uint64_t{RC()} << shift);
  }
  return std::bit_cast<double>(bits);
}

auto Reader::DD(double other) -> double
{
  switch (BB())
  {
  case 0:
    return other;
  case 1:
    return PatchBytes(other, FirstFour);
  case 2:
    return PatchBytes(other, SixBytes);
  default:
    return RD();
  }
}

auto Reader::BT(Version release) -> double
{
  if (release >= Version::R2000 && B())
  {
    return 0.0;
  }
  return BD();
}

auto Reader::BE(Version release) -> Vector
{
  if (release >= Version::R2000 && B())
  {
    return Up;
  }
  return Point3();
}

auto Reader::Point2() -> Vector
{
  Vector point;
  point.X = BD();
  point.Y = BD();
  return point;
}

auto Reader::Point3() -> Vector
{
  Vector point = Point2();
  point.Z = BD();
  return point;
}

auto Reader::Raw() -> std::string
{
  const int length = BS();
  if (length < 0)
  {
    Fail();
    return {};
  }
  std::string text;
  for (int index = 0; index < length && !m_Failed; ++index)
  {
    text.push_back(static_cast<char>(RC()));
  }
  return text;
}

auto Reader::CMC(Version release) -> Color
{
  return CMC(release, *this);
}

auto Reader::CMC(Version release, Reader &names) -> Color
{
  Color color;
  color.Index = BS();
  if (release < Version::R2004)
  {
    return color;
  }
  // The long's top byte is the method: 0xC3 an index in the low byte, 0xC0 by layer, 0xC1 by block, else a true
  // color (0xC2) kept whole; AutoCAD writes the index 0 before it.
  const auto value = static_cast<std::uint32_t>(BL());
  switch (value >> 24U)
  {
  case ByIndex:
    color.Index = static_cast<int>(value & 0xFFU);
    break;
  case ByLayer:
    color.Index = ByLayerIndex;
    break;
  case ByBlock:
    color.Index = 0;
    break;
  default:
    color.Rgb = static_cast<std::int32_t>(value);
    break;
  }
  const unsigned flags = RC();
  const bool unicode = release >= Version::R2007;
  if ((flags & 3U) == 0)
  {
    return color;
  }
  ColorNames named;
  if ((flags & 1U) != 0)
  {
    named.Name = unicode ? names.TU() : names.Raw();
  }
  if ((flags & 2U) != 0)
  {
    named.Book = unicode ? names.TU() : names.Raw();
  }
  color.Names = std::move(named);
  return color;
}

auto Reader::ENC(Version release) -> Color
{
  Color color;
  const auto value = static_cast<unsigned>(static_cast<std::uint16_t>(BS()));
  if (release < Version::R2004)
  {
    color.Index = static_cast<std::int16_t>(value);
    return color;
  }
  color.Index = static_cast<int>(value & IndexBits);
  if ((value & TrueColor) != 0)
  {
    color.Rgb = BL();
  }
  if ((value & Referenced) != 0)
  {
    color.Reference = Handle{};
  }
  if ((value & Transparent) != 0)
  {
    color.Transparency = BL();
  }
  return color;
}

auto Reader::OT(Version release) -> int
{
  if (release < Version::R2010)
  {
    return BS();
  }
  switch (BB())
  {
  case 0:
    return RC();
  case 1:
    return RC() + Wide;
  default:
    return RS();
  }
}

auto Reader::T(const Encoding::Scheme &with) -> std::string
{
  std::string raw = Raw();
  if (!raw.empty() && raw.back() == '\0')
  {
    raw.pop_back();
  }
  return Encoding::Decode(with, raw);
}

auto Reader::TU() -> std::string
{
  const int count = BS();
  if (count < 0)
  {
    Fail();
    return {};
  }
  std::u16string units;
  for (int index = 0; index < count; ++index)
  {
    const std::uint16_t unit = RS();
    if (m_Failed)
    {
      return {};
    }
    units.push_back(unit);
  }
  return Encoding::FromUnits(units);
}

auto Reader::TV(Version release, const Encoding::Scheme &with) -> std::string
{
  return release >= Version::R2007 ? TU() : T(with);
}
} // namespace DwgIO::Bits
