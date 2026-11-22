/***********************************************************************************************************************
 *  @file      source/bits/lz21.cpp
 *  @brief     The compression and page checksum of R2007's coded container.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/lz21.hpp"

#include "bits/shuffle.hpp"

#include <algorithm>
#include <array>
#include <initializer_list>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <utility>

namespace DwgIO::Bits::Lz21
{
namespace
{
constexpr std::size_t Chunk = 32;

/**
 *  One block of a literal chunk: its size and where in the chunk's source it starts.
 */
class Block
{
public:
  Block(std::uint8_t count, std::uint8_t at) : Count{count}, At{at} {}

  std::uint8_t Count;
  std::uint8_t At;
};

using Blocks = std::initializer_list<Block>;

/**
 *  The blocks of chapter 5.10.1, in the order they land in the output, for a chunk of 1 to 32 bytes.
 */
auto Orders() -> const std::array<Blocks, Chunk + 1> &
{
  static const std::array<Blocks, Chunk + 1> orders{
      Blocks{},
      Blocks{{1, 0}},
      Blocks{{1, 1}, {1, 0}},
      Blocks{{1, 2}, {1, 1}, {1, 0}},
      Blocks{{1, 0}, {1, 1}, {1, 2}, {1, 3}},
      Blocks{{1, 4}, {4, 0}},
      Blocks{{1, 5}, {4, 1}, {1, 0}},
      Blocks{{2, 5}, {4, 1}, {1, 0}},
      Blocks{{4, 0}, {4, 4}},
      Blocks{{1, 8}, {8, 0}},
      Blocks{{1, 9}, {8, 1}, {1, 0}},
      Blocks{{2, 9}, {8, 1}, {1, 0}},
      Blocks{{4, 8}, {8, 0}},
      Blocks{{1, 12}, {4, 8}, {8, 0}},
      Blocks{{1, 13}, {4, 9}, {8, 1}, {1, 0}},
      Blocks{{2, 13}, {4, 9}, {8, 1}, {1, 0}},
      Blocks{{8, 8}, {8, 0}},
      Blocks{{8, 9}, {1, 8}, {8, 0}},
      Blocks{{1, 17}, {16, 1}, {1, 0}},
      Blocks{{3, 16}, {16, 0}},
      Blocks{{4, 16}, {16, 0}},
      Blocks{{1, 20}, {4, 16}, {16, 0}},
      Blocks{{2, 20}, {4, 16}, {16, 0}},
      Blocks{{3, 20}, {4, 16}, {16, 0}},
      Blocks{{8, 16}, {16, 0}},
      Blocks{{8, 17}, {1, 16}, {16, 0}},
      Blocks{{1, 25}, {8, 17}, {1, 16}, {16, 0}},
      Blocks{{2, 25}, {8, 17}, {1, 16}, {16, 0}},
      Blocks{{4, 24}, {8, 16}, {16, 0}},
      Blocks{{1, 28}, {4, 24}, {8, 16}, {16, 0}},
      Blocks{{2, 28}, {4, 24}, {8, 16}, {16, 0}},
      Blocks{{1, 30}, {4, 26}, {8, 18}, {16, 2}, {2, 0}},
      Blocks{{16, 16}, {16, 0}},
  };
  return orders;
}

constexpr std::size_t Half = 8;

/**
 *  Appends one block: two and three bytes reversed, sixteen with its halves swapped, the rest as they are.
 */
void Put(std::span<const std::byte> block, std::vector<std::byte> &out)
{
  if (block.size() == 2 || block.size() == 3)
  {
    out.insert(out.end(), block.rbegin(), block.rend());
    return;
  }
  if (block.size() == 2 * Half)
  {
    out.insert(out.end(), block.begin() + Half, block.end());
    out.insert(out.end(), block.begin(), block.begin() + Half);
    return;
  }
  out.insert(out.end(), block.begin(), block.end());
}

/**
 *  Reads the stream, failing at its end.
 */
struct Input
{
  std::span<const std::byte> Bytes;
  std::size_t At{};
  bool Failed{};

  [[nodiscard]] auto AtEnd() const -> bool { return At >= Bytes.size(); }

  auto Next() -> unsigned
  {
    if (AtEnd())
    {
      Failed = true;
      return 0;
    }
    return std::to_integer<unsigned>(Bytes[At++]);
  }
};

/**
 *  A back reference: how far back and how many bytes, and the opcode after it.
 */
struct Copy
{
  std::size_t Offset{};
  std::size_t Length{};
  unsigned Opcode{};
};

constexpr unsigned Nibble = 4;
constexpr unsigned Low = 0x0F;
constexpr unsigned Low3 = 7;
constexpr unsigned High5 = 0xF8;

/**
 *  The instructions of chapter 5.10.2 for an opcode.
 */
auto Instruction(unsigned opcode, Input &in) -> Copy
{
  constexpr unsigned Short = 0x13;
  constexpr unsigned Near = 3;
  constexpr unsigned Far = 0x100;
  Copy copy;
  switch (opcode >> Nibble)
  {
  case 0:
  {
    copy.Length = (opcode & Low) + Short;
    const unsigned low = in.Next();
    copy.Opcode = in.Next();
    copy.Length += (copy.Opcode >> 3U) & 0x10U;
    copy.Offset = ((copy.Opcode & 0x78U) << 5U) + 1U + low;
  }
  break;
  case 1:
  {
    copy.Length = (opcode & Low) + Near;
    const unsigned low = in.Next();
    copy.Opcode = in.Next();
    copy.Offset = ((copy.Opcode & High5) << 5U) + 1U + low;
  }
  break;
  case 2:
  {
    std::size_t offset = in.Next();
    offset |= static_cast<std::size_t>(in.Next()) << 8U;
    std::size_t length = opcode & Low3;
    if ((opcode & 8U) == 0)
    {
      copy.Opcode = in.Next();
      length += copy.Opcode & High5;
    }
    else
    {
      ++offset;
      length += static_cast<std::size_t>(in.Next()) << 3U;
      copy.Opcode = in.Next();
      length += (static_cast<std::size_t>(copy.Opcode & High5) << 8U) + Far;
    }
    copy.Offset = offset;
    copy.Length = length;
  }
  break;
  default:
    copy.Length = opcode >> Nibble;
    copy.Opcode = in.Next();
    copy.Offset = ((copy.Opcode & High5) << 1U) + (opcode & Low) + 1U;
    break;
  }
  return copy;
}

/**
 *  A literal run's length from its opcode: the opcode plus eight, more bytes when that makes 0x17.
 */
auto LiteralLength(unsigned opcode, Input &in) -> std::size_t
{
  constexpr std::size_t Base = 8;
  constexpr std::size_t Extended = 0x17;
  constexpr unsigned Full = 0xFF;
  constexpr unsigned Wide = 0xFFFF;
  std::size_t length = opcode + Base;
  if (length != Extended)
  {
    return length;
  }
  unsigned more = in.Next();
  length += more;
  if (more != Full)
  {
    return length;
  }
  while (true) // A read past the end gives zero, which ends it.
  {
    more = in.Next() | (in.Next() << 8U);
    length += more;
    if (more != Wide)
    {
      return length;
    }
  }
}

constexpr std::size_t MinimumMatch = 3;
constexpr std::size_t Window = 0xFFFF;
constexpr std::size_t Tries = 16;

/**
 *  The shortest reference worth its bytes at a distance: two bytes of instruction near, three, then four or five far,
 *  and the run it breaks.
 */
auto Worth(std::size_t offset) -> std::size_t
{
  constexpr std::size_t Near = 0x200;
  constexpr std::size_t Middle = 0x2000;
  constexpr std::size_t Short = 4;
  constexpr std::size_t Medium = 5;
  constexpr std::size_t Long = 6;
  if (offset <= Near)
  {
    return Short;
  }
  return offset <= Middle ? Medium : Long;
}

/**
 *  A back reference found: how many bytes and how far back.
 */
struct Found
{
  std::size_t Length{};
  std::size_t Offset{};
};

/**
 *  Finds the longest earlier copy of the bytes at a position among the last few that share their first three.
 */
class Matcher
{
public:
  explicit Matcher(std::span<const std::byte> bytes) : m_Bytes{bytes}, m_Previous(bytes.size(), None) {}

  void Add(std::size_t at)
  {
    if (at + MinimumMatch > m_Bytes.size())
    {
      return;
    }
    const std::uint32_t key = Key(at);
    const auto head = m_Heads.find(key);
    m_Previous[at] = head == m_Heads.end() ? None : head->second;
    m_Heads[key] = at;
  }

  [[nodiscard]] auto Find(std::size_t at) const -> Found
  {
    Found best;
    if (at + MinimumMatch > m_Bytes.size())
    {
      return best;
    }
    const auto head = m_Heads.find(Key(at));
    std::size_t candidate = head == m_Heads.end() ? None : head->second;
    for (std::size_t tried = 0; candidate != None && tried < Tries && at - candidate <= Window; ++tried)
    {
      std::size_t length = 0;
      while (at + length < m_Bytes.size() && m_Bytes[candidate + length] == m_Bytes[at + length])
      {
        ++length;
      }
      if (length > best.Length)
      {
        best = Found{.Length = length, .Offset = at - candidate};
      }
      candidate = m_Previous[candidate];
    }
    return best;
  }

private:
  static constexpr std::size_t None = static_cast<std::size_t>(-1);

  [[nodiscard]] auto Key(std::size_t at) const -> std::uint32_t
  {
    return std::to_integer<std::uint32_t>(m_Bytes[at]) | (std::to_integer<std::uint32_t>(m_Bytes[at + 1]) << 8U) |
           (std::to_integer<std::uint32_t>(m_Bytes[at + 2]) << 16U);
  }

  std::span<const std::byte> m_Bytes;
  std::vector<std::size_t> m_Previous;
  std::unordered_map<std::uint32_t, std::size_t> m_Heads;
};

/**
 *  Writes the stream: a literal run's length goes in the byte that ends the reference before it when below eight, else
 *  in an opcode of its own; the bytes of a run in the chunk order Decompress undoes.
 */
class Emitter
{
public:
  explicit Emitter(std::vector<std::byte> &out) : m_Out{&out} {}

  void Literals(std::span<const std::byte> run)
  {
    constexpr std::size_t Base = 8;
    if (!m_Started)
    {
      if (run.size() < Base)
      {
        Put(0x20); // An empty reference, its last byte giving the run's length.
        Put(0);
        Put(0);
        Put(static_cast<unsigned>(run.size()));
      }
      else
      {
        Length(run.size());
      }
    }
    else if (run.size() < Base)
    {
      (*m_Out)[m_Last] |= static_cast<std::byte>(run.size());
    }
    else
    {
      Length(run.size());
    }
    m_Last = None;
    m_Started = true;
    for (std::size_t done = 0; done < run.size(); done += Chunk)
    {
      const std::span<const std::byte> chunk = run.subspan(done, std::min(Chunk, run.size() - done));
      const std::size_t base = m_Out->size();
      m_Out->resize(base + chunk.size());
      std::size_t taken = 0;
      for (const Block block : Orders().at(chunk.size()))
      {
        for (std::size_t index = 0; index < block.Count; ++index)
        {
          (*m_Out)[base + block.At + Source(block.Count, index)] = chunk[taken + index];
        }
        taken += block.Count;
      }
    }
  }

  void Reference(std::size_t length, std::size_t offset)
  {
    constexpr std::size_t ShortLongest = 14;
    constexpr std::size_t ShortFarthest = 0x200;
    constexpr std::size_t NearLongest = 18;
    constexpr std::size_t NearFarthest = 0x2000;
    constexpr std::size_t Longest = 0xFF;
    constexpr std::size_t Far = 0x100;
    constexpr std::size_t Wide = 0xFFFF;
    while (length > 0)
    {
      const std::size_t back = offset - 1;
      if (length <= ShortLongest && offset <= ShortFarthest)
      {
        Put(static_cast<unsigned>((length << Nibble) | (back & Low)));
        End(static_cast<unsigned>((back >> Nibble) << 3U));
        return;
      }
      if (length <= NearLongest && offset <= NearFarthest)
      {
        Put(static_cast<unsigned>(0x10U | (length - 3)));
        Put(static_cast<unsigned>(back & 0xFFU));
        End(static_cast<unsigned>((back >> 8U) << 3U));
        return;
      }
      const std::size_t most = length > Longest ? Far + Wide : Longest;
      std::size_t part = std::min(length, most);
      if (length - part > 0 && length - part < MinimumMatch)
      {
        part = length - MinimumMatch;
      }
      if (part > Longest) // The long form: the length less 0x100 in three pieces, the distance less one.
      {
        const std::size_t rest = part - Far;
        Put(static_cast<unsigned>(0x28U | (rest & Low3)));
        Put(static_cast<unsigned>(back & 0xFFU));
        Put(static_cast<unsigned>(back >> 8U));
        Put(static_cast<unsigned>((rest >> 3U) & 0xFFU));
        End(static_cast<unsigned>((rest >> 8U) & High5));
      }
      else
      {
        Put(static_cast<unsigned>(0x20U | (part & Low3)));
        Put(static_cast<unsigned>(offset & 0xFFU));
        Put(static_cast<unsigned>(offset >> 8U));
        End(static_cast<unsigned>(part & High5));
      }
      length -= part;
    }
  }

private:
  static constexpr std::size_t None = static_cast<std::size_t>(-1);

  /**
   *  Where in a block of `count` the byte `index` of its output comes from.
   */
  static auto Source(std::size_t count, std::size_t index) -> std::size_t
  {
    if (count == 2 || count == 3)
    {
      return count - 1 - index;
    }
    return count == 2 * Half ? (index + Half) % (2 * Half) : index;
  }

  void Put(unsigned value) { m_Out->push_back(static_cast<std::byte>(value)); }

  /**
   *  The byte that ends a reference, its low three bits left for the run after it.
   */
  void End(unsigned value)
  {
    m_Last = m_Out->size();
    Put(value);
  }

  /**
   *  A run's length of eight bytes or more as an opcode below 0x10 and the bytes after it.
   */
  void Length(std::size_t length)
  {
    constexpr std::size_t Base = 8;
    constexpr std::size_t Extended = 0x17;
    constexpr std::size_t Full = 0xFF;
    constexpr std::size_t Wide = 0xFFFF;
    if (length < Extended)
    {
      Put(static_cast<unsigned>(length - Base));
      return;
    }
    Put(Low);
    std::size_t rest = length - Extended;
    if (rest < Full)
    {
      Put(static_cast<unsigned>(rest));
      return;
    }
    Put(static_cast<unsigned>(Full));
    rest -= Full;
    while (true)
    {
      const std::size_t part = std::min(rest, Wide);
      Put(static_cast<unsigned>(part & 0xFFU));
      Put(static_cast<unsigned>(part >> 8U));
      rest -= part;
      if (part != Wide)
      {
        return;
      }
    }
  }

  std::vector<std::byte> *m_Out;
  std::size_t m_Last{None};
  bool m_Started{};
};

/**
 *  The output growing from the input, to at most `Most` bytes.
 */
struct Expansion
{
  Input In;
  std::vector<std::byte> Out;
  std::size_t Most{};

  /**
   *  Copies a run of `length` bytes in its chunk order; false when the input or the room runs short.
   */
  [[nodiscard]] auto Literals(std::size_t length) -> bool
  {
    if (length > In.Bytes.size() - std::min(In.At, In.Bytes.size()) || Out.size() + length > Most)
    {
      return false;
    }
    for (std::size_t done = 0; done < length;)
    {
      const std::size_t count = std::min(Chunk, length - done);
      const std::span<const std::byte> chunk = In.Bytes.subspan(In.At + done, count);
      for (const Block block : Orders().at(count))
      {
        Put(chunk.subspan(block.At, block.Count), Out);
      }
      done += count;
    }
    In.At += length;
    return true;
  }

  /**
   *  The back references after a run, until one gives the next run's length or an opcode starts one: that length and
   *  opcode; nothing for a reference before the start or beyond the room.
   */
  [[nodiscard]] auto Copies() -> std::optional<std::pair<std::size_t, unsigned>>
  {
    constexpr unsigned Fifteen = 15;
    unsigned opcode = In.Next();
    Copy copy = Instruction(opcode, In);
    while (!In.Failed)
    {
      // A reference of no distance would copy the byte not yet written.
      if (copy.Offset > Out.size() || (copy.Offset == 0 && copy.Length != 0) || Out.size() + copy.Length > Most)
      {
        return std::nullopt;
      }
      for (std::size_t at = 0; at < copy.Length; ++at)
      {
        Out.push_back(Out[Out.size() - copy.Offset]);
      }
      const std::size_t length = copy.Opcode & Low3;
      if (length != 0 || In.AtEnd())
      {
        return std::pair{length, opcode};
      }
      opcode = In.Next();
      if ((opcode >> Nibble) == 0)
      {
        return std::pair{std::size_t{0}, opcode};
      }
      if ((opcode >> Nibble) == Fifteen)
      {
        opcode &= Low;
      }
      copy = Instruction(opcode, In);
    }
    return std::pair{std::size_t{0}, opcode};
  }
};
} // namespace

auto Decompress(std::span<const std::byte> compressed, std::size_t most) -> std::optional<std::vector<std::byte>>
{
  constexpr unsigned Prelude = 2;
  Expansion expanding{.In = Input{.Bytes = compressed, .At = 0, .Failed = false}, .Out = {}, .Most = most};
  Input &in = expanding.In;
  std::size_t length = 0;
  unsigned opcode = in.Next();
  if ((opcode >> Nibble) == Prelude)
  {
    in.At = 3;
    length = in.Next() & Low3;
  }
  while (!in.AtEnd()) // A read past the end fails and leaves the input done.
  {
    if (length == 0)
    {
      length = LiteralLength(opcode, in);
    }
    if (!expanding.Literals(length))
    {
      return std::nullopt;
    }
    if (in.AtEnd())
    {
      break;
    }
    const std::optional<std::pair<std::size_t, unsigned>> next = expanding.Copies();
    if (!next)
    {
      return std::nullopt;
    }
    std::tie(length, opcode) = *next;
  }
  if (in.Failed)
  {
    return std::nullopt;
  }
  return std::move(expanding.Out);
}

auto Checksum(std::uint64_t seed, std::span<const std::byte> bytes) -> std::uint32_t
{
  constexpr std::uint64_t Multiplier = 0x343FD;
  constexpr std::uint64_t Increment = 0x269EC3;
  constexpr std::uint32_t Mask = 0xFFFF;
  constexpr std::uint32_t Modulus = 0xFFF1;
  constexpr std::size_t Big = 0x15B0;
  const std::uint64_t mixed = ((seed + bytes.size()) * Multiplier) + Increment;
  std::uint32_t first = static_cast<std::uint32_t>(mixed) & Mask;
  std::uint32_t second = static_cast<std::uint32_t>(mixed >> 16U) & Mask;
  for (std::size_t start = 0; start < bytes.size(); start += Big)
  {
    Shuffle(bytes.subspan(start, std::min(Big, bytes.size() - start)),
            [&first, &second](std::byte one) -> void
            {
              first += std::to_integer<std::uint32_t>(one);
              second += first;
            });
    first %= Modulus;
    second %= Modulus;
  }
  return (second << 16U) | (first & Mask);
}

auto Compress(std::span<const std::byte> bytes) -> std::vector<std::byte>
{
  std::vector<std::byte> out;
  if (bytes.empty())
  {
    return out;
  }
  Matcher matcher{bytes};
  Emitter emit{out};
  std::size_t literal = 0; // The first byte starts a run: there is nothing before it to copy.
  std::size_t at = 1;
  matcher.Add(0);
  while (at < bytes.size())
  {
    const Found found = matcher.Find(at);
    if (found.Length < Worth(found.Offset))
    {
      matcher.Add(at);
      ++at;
      continue;
    }
    emit.Literals(bytes.subspan(literal, at - literal));
    emit.Reference(found.Length, found.Offset);
    for (std::size_t added = at; added < at + found.Length; ++added)
    {
      matcher.Add(added);
    }
    at += found.Length;
    literal = at;
  }
  if (literal < bytes.size())
  {
    emit.Literals(bytes.subspan(literal));
  }
  return out;
}
} // namespace DwgIO::Bits::Lz21
