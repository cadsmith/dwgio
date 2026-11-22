/***********************************************************************************************************************
 *  @file      source/bits/lz.cpp
 *  @brief     The LZ77 variant of the paged container, chapter 4.7.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/lz.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>

namespace DwgIO::Bits::Lz
{
namespace
{
constexpr std::uint8_t End = 0x11;
constexpr std::uint8_t LongShort = 0x10;
constexpr std::uint8_t LongWide = 0x20;
constexpr std::uint8_t NearLow = 0x12;
constexpr std::uint8_t NearHigh = 0x1F;
constexpr std::uint8_t WideLow = 0x21;
/**
 *  The 0x21 opcode copies three bytes.
 */
constexpr std::uint8_t WideBase = 3;
constexpr std::uint8_t Close = 0x40;
constexpr std::size_t FarBase = 0x3FFF;
constexpr std::size_t LiteralBase = 3;
constexpr std::size_t LiteralRun = 0x0F;
constexpr std::size_t LiteralStep = 0xFF;
constexpr std::size_t Opening = 4;
constexpr std::size_t CloseReach = 0x400;
constexpr std::size_t WideReach = 0x4000;
constexpr std::size_t FarReach = 0x7FFF;
constexpr std::size_t CloseMost = 14;
constexpr std::size_t WideMost = 33;
/**
 *  The far family's short form stops at nine bytes: AutoCAD's decoder refuses 0x18 to 0x1F, so ten bytes and more take
 *  the long form.
 */
constexpr std::size_t NearMost = 9;
constexpr std::size_t LongShortLeast = 10;
/**
 *  The opcodes admit three bytes, but AutoCAD's decoder refuses a copy shorter than four.
 */
constexpr std::size_t Least = 4;
/**
 *  AutoCAD's decoder refuses a stream whose last copy reaches nearer the end than this.
 */
constexpr std::size_t Tail = 13;
constexpr std::size_t HashBits = 14;

class Source
{
public:
  explicit Source(std::span<const std::byte> bytes) : m_Bytes(bytes) {}

  [[nodiscard]] auto Next() -> std::optional<std::uint8_t>
  {
    if (m_At >= m_Bytes.size())
    {
      return std::nullopt;
    }
    return static_cast<std::uint8_t>(m_Bytes[m_At++]);
  }

  [[nodiscard]] auto Take(std::size_t count, std::vector<std::byte> &into) -> bool
  {
    if (count > m_Bytes.size() - m_At)
    {
      return false;
    }
    into.insert(into.end(), m_Bytes.begin() + static_cast<std::ptrdiff_t>(m_At),
                m_Bytes.begin() + static_cast<std::ptrdiff_t>(m_At + count));
    m_At += count;
    return true;
  }

private:
  std::span<const std::byte> m_Bytes;
  std::size_t m_At{};
};

/**
 *  A literal length read: the count, or the opcode the byte turned out to be.
 */
struct Literal
{
  std::size_t Count{};
  std::optional<std::uint8_t> Opcode{};
};

auto ReadLiteral(Source &in) -> std::optional<Literal>
{
  const std::optional<std::uint8_t> first = in.Next();
  if (!first)
  {
    return std::nullopt;
  }
  if ((*first & 0xF0U) != 0)
  {
    return Literal{.Count = 0, .Opcode = first};
  }
  if (*first != 0)
  {
    return Literal{.Count = *first + LiteralBase, .Opcode = std::nullopt};
  }
  std::size_t total = LiteralRun;
  while (true)
  {
    const std::optional<std::uint8_t> next = in.Next();
    if (!next)
    {
      return std::nullopt;
    }
    total += *next == 0 ? LiteralStep : *next;
    if (*next != 0)
    {
      return Literal{.Count = total + LiteralBase, .Opcode = std::nullopt};
    }
  }
}

auto ReadLong(Source &in) -> std::optional<std::size_t>
{
  const std::optional<std::uint8_t> first = in.Next();
  if (!first)
  {
    return std::nullopt;
  }
  if (*first != 0)
  {
    return *first;
  }
  std::size_t total = LiteralStep;
  while (true)
  {
    const std::optional<std::uint8_t> next = in.Next();
    if (!next)
    {
      return std::nullopt;
    }
    total += *next == 0 ? LiteralStep : *next;
    if (*next != 0)
    {
      return total;
    }
  }
}

/**
 *  A two-byte offset: the offset and the literal count in its low two bits.
 */
struct Offset
{
  std::size_t Back{};
  std::size_t Literals{};
};

auto ReadOffset(Source &in) -> std::optional<Offset>
{
  const std::optional<std::uint8_t> first = in.Next();
  const std::optional<std::uint8_t> second = in.Next();
  if (!first || !second)
  {
    return std::nullopt;
  }
  return Offset{.Back = (static_cast<std::size_t>(*first) >> 2U) | (static_cast<std::size_t>(*second) << 6U),
                .Literals = *first & 3U};
}

/**
 *  One copy: how many bytes, from how far back (one less than the distance), and the literals after it.
 */
struct Copy
{
  std::size_t Count{};
  std::size_t Back{};
  std::size_t Literals{};
};

auto ReadCopy(std::uint8_t opcode, Source &in) -> std::optional<Copy>
{
  if (opcode >= Close)
  {
    const std::optional<std::uint8_t> second = in.Next();
    if (!second)
    {
      return std::nullopt;
    }
    return Copy{.Count = ((opcode & 0xF0U) >> 4U) - 1U,
                .Back = (static_cast<std::size_t>(*second) << 2U) | ((opcode & 0x0CU) >> 2U),
                .Literals = opcode & 3U};
  }
  std::size_t count = 0;
  std::size_t base = 0;
  if (opcode == LongShort)
  {
    const std::optional<std::size_t> length = ReadLong(in);
    if (!length)
    {
      return std::nullopt;
    }
    count = *length + LongShortLeast - 1;
    base = FarBase;
  }
  else if (opcode >= NearLow && opcode <= NearHigh)
  {
    count = (opcode & 0x0FU) + 2U;
    base = FarBase;
  }
  else if (opcode == LongWide)
  {
    const std::optional<std::size_t> length = ReadLong(in);
    if (!length)
    {
      return std::nullopt;
    }
    count = *length + WideMost;
  }
  else if (opcode >= WideLow) // Below Close, handled first.
  {
    count = opcode - (WideLow - WideBase);
  }
  else
  {
    return std::nullopt;
  }
  const std::optional<Offset> offset = ReadOffset(in);
  if (!offset)
  {
    return std::nullopt;
  }
  return Copy{.Count = count, .Back = offset->Back + base, .Literals = offset->Literals};
}

/**
 *  The output, which grows by copies and literals up to the most allowed.
 */
class Sink
{
public:
  Sink(std::size_t most, std::vector<std::byte> &out) : m_Limit(out.size() + most), m_Out(&out) {}

  /**
   *  Appends a copy's bytes, which may overlap what they copy.
   */
  [[nodiscard]] auto AppendCopy(const Copy &copy) const -> bool
  {
    const std::size_t distance = copy.Back + 1;
    if (distance > m_Out->size() || m_Out->size() + copy.Count > m_Limit)
    {
      return false;
    }
    for (std::size_t at = 0; at < copy.Count; ++at)
    {
      m_Out->push_back((*m_Out)[m_Out->size() - distance]);
    }
    return true;
  }

  [[nodiscard]] auto Literals(std::size_t count, Source &in) const -> bool
  {
    return m_Out->size() + count <= m_Limit && in.Take(count, *m_Out);
  }

private:
  std::size_t m_Limit;
  std::vector<std::byte> *m_Out;
};

/**
 *  The stream of a page: the opening literal, then copies, each with the literals after it.
 */
class Packer
{
public:
  explicit Packer(std::span<const std::byte> bytes) : m_Bytes(bytes) { m_Heads.fill(None); }

  auto Run() -> std::vector<std::byte>
  {
    std::size_t at = Opening;
    std::size_t literalStart = 0;
    while (at + Least + Tail <= m_Bytes.size())
    {
      const std::optional<Copy> match = Longest(at);
      if (!match)
      {
        Remember(at);
        ++at;
        continue;
      }
      Flush(literalStart, at - literalStart);
      m_Pending = *match;
      for (std::size_t inside = 0; inside < match->Count; ++inside)
      {
        Remember(at + inside);
      }
      at += match->Count;
      literalStart = at;
    }
    Flush(literalStart, m_Bytes.size() - literalStart); // Never empty: a copy leaves the tail, so this ends the copy.
    // AutoCAD's decoder reads two bytes past the end, so two zero bytes follow 0x11, counted in the compressed size.
    m_Out.push_back(std::byte{End});
    m_Out.push_back(std::byte{0});
    m_Out.push_back(std::byte{0});
    return std::move(m_Out);
  }

private:
  static constexpr std::size_t None = static_cast<std::size_t>(-1);

  [[nodiscard]] auto Key(std::size_t at) const -> std::size_t
  {
    const auto value = (static_cast<std::uint32_t>(m_Bytes[at]) << 16U) |
                       (static_cast<std::uint32_t>(m_Bytes[at + 1]) << 8U) |
                       static_cast<std::uint32_t>(m_Bytes[at + 2]);
    return (value * 2654435761U) >> (32U - HashBits);
  }

  void Remember(std::size_t at) // Called within the loop's bound, so three bytes always follow.
  {
    m_Heads.at(Key(at)) = at;
  }

  /**
   *  The longest match the last position of the same three bytes gives, coded as a copy, or none worth coding.
   */
  [[nodiscard]] auto Longest(std::size_t at) const -> std::optional<Copy>
  {
    const std::size_t candidate = m_Heads.at(Key(at));
    if (candidate == None)
    {
      return std::nullopt;
    }
    const std::size_t distance = at - candidate;
    if (distance > FarReach)
    {
      return std::nullopt;
    }
    std::size_t length = 0;
    const std::size_t room = m_Bytes.size() - at - Tail;
    while (length < room && m_Bytes[candidate + length] == m_Bytes[at + length])
    {
      ++length;
    }
    if (length < Least) // The far family's floor is the same four bytes.
    {
      return std::nullopt;
    }
    return Copy{.Count = length, .Back = distance - 1, .Literals = 0};
  }

  /**
   *  The literals that follow the pending copy, or open the stream when there is none yet.
   */
  void Flush(std::size_t start, std::size_t count)
  {
    if (m_Pending)
    {
      Emit(*m_Pending, count);
      m_Pending.reset();
    }
    else
    {
      Length(count);
    }
    m_Out.insert(m_Out.end(), m_Bytes.begin() + static_cast<std::ptrdiff_t>(start),
                 m_Bytes.begin() + static_cast<std::ptrdiff_t>(start + count));
  }

  /**
   *  A literal length token for a run of four or more.
   */
  void Length(std::size_t count)
  {
    const std::size_t value = count - LiteralBase;
    if (value <= LiteralRun)
    {
      m_Out.push_back(static_cast<std::byte>(value));
      return;
    }
    m_Out.push_back(std::byte{0});
    std::size_t rest = value - LiteralRun;
    while (rest > LiteralStep)
    {
      m_Out.push_back(std::byte{0});
      rest -= LiteralStep;
    }
    m_Out.push_back(static_cast<std::byte>(rest));
  }

  void Long(std::size_t value)
  {
    if (value <= LiteralStep)
    {
      m_Out.push_back(static_cast<std::byte>(value));
      return;
    }
    m_Out.push_back(std::byte{0});
    std::size_t rest = value - LiteralStep;
    while (rest > LiteralStep)
    {
      m_Out.push_back(std::byte{0});
      rest -= LiteralStep;
    }
    m_Out.push_back(static_cast<std::byte>(rest));
  }

  void WriteOffset(std::size_t back, std::size_t literals)
  {
    m_Out.push_back(static_cast<std::byte>(((back & 0x3FU) << 2U) | literals));
    m_Out.push_back(static_cast<std::byte>(back >> 6U));
  }

  /**
   *  A copy with the count of literals after it: 1 to 3 in the opcode, else a literal length token follows.
   */
  void Emit(const Copy &copy, std::size_t literals)
  {
    const std::size_t inline3 = literals <= 3 ? literals : 0;
    const std::size_t distance = copy.Back + 1;
    if (distance <= CloseReach && copy.Count <= CloseMost)
    {
      m_Out.push_back(static_cast<std::byte>(((copy.Count + 1) << 4U) | ((copy.Back & 3U) << 2U) | inline3));
      m_Out.push_back(static_cast<std::byte>(copy.Back >> 2U));
    }
    else if (distance <= WideReach && copy.Count <= WideMost)
    {
      m_Out.push_back(static_cast<std::byte>(copy.Count + (WideLow - 3)));
      WriteOffset(copy.Back, inline3);
    }
    else if (distance <= WideReach)
    {
      m_Out.push_back(std::byte{LongWide});
      Long(copy.Count - WideMost);
      WriteOffset(copy.Back, inline3);
    }
    else if (copy.Count <= NearMost)
    {
      m_Out.push_back(static_cast<std::byte>(LongShort | (copy.Count - 2)));
      WriteOffset(copy.Back - FarBase, inline3);
    }
    else
    {
      m_Out.push_back(std::byte{LongShort});
      Long(copy.Count - (LongShortLeast - 1));
      WriteOffset(copy.Back - FarBase, inline3);
    }
    if (literals > 3)
    {
      Length(literals);
    }
  }

  std::span<const std::byte> m_Bytes;
  std::vector<std::byte> m_Out;
  std::array<std::size_t, std::size_t{1} << HashBits> m_Heads{};
  std::optional<Copy> m_Pending{};
};
} // namespace

auto Decompress(std::span<const std::byte> compressed, std::size_t most, std::vector<std::byte> &out) -> bool
{
  Source in{compressed};
  const Sink sink{most, out};
  const std::optional<Literal> opening = ReadLiteral(in);
  if (!opening || !sink.Literals(opening->Count, in))
  {
    return false;
  }
  std::optional<std::uint8_t> opcode = opening->Opcode ? opening->Opcode : in.Next();
  while (opcode && *opcode != End)
  {
    const std::optional<Copy> copy = ReadCopy(*opcode, in);
    if (!copy || !sink.AppendCopy(*copy))
    {
      return false;
    }
    if (copy->Literals != 0)
    {
      if (!sink.Literals(copy->Literals, in))
      {
        return false;
      }
      opcode = in.Next();
      continue;
    }
    const std::optional<Literal> literal = ReadLiteral(in);
    if (!literal || !sink.Literals(literal->Count, in))
    {
      return false;
    }
    opcode = literal->Opcode ? literal->Opcode : in.Next();
  }
  return opcode.has_value();
}

auto Compress(std::span<const std::byte> bytes) -> std::vector<std::byte>
{
  std::vector<std::byte> padded;
  if (bytes.size() < Opening)
  {
    padded.assign(bytes.begin(), bytes.end());
    padded.resize(Opening, std::byte{0});
    bytes = padded;
  }
  return Packer{bytes}.Run();
}
} // namespace DwgIO::Bits::Lz
