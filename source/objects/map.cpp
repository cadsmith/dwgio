/***********************************************************************************************************************
 *  @file      source/objects/map.cpp
 *  @brief     The reader and writer of the object map of chapter 23.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "objects/map.hpp"

#include "bits/byte.hpp"
#include "bits/crc.hpp"
#include "bits/reader.hpp"
#include "bits/writer.hpp"

#include <algorithm>
#include <cstdint>

namespace DwgIO::Objects
{
namespace
{
constexpr std::size_t Word = 2;

constexpr std::size_t Largest = 2032;

using Bits::Byte;

auto BigEndian(std::span<const std::byte> at) -> std::uint16_t
{
  return static_cast<std::uint16_t>((static_cast<unsigned>(at[0]) << Byte) | static_cast<unsigned>(at[1]));
}

void PutBigEndian(std::vector<std::byte> &out, std::uint16_t value)
{
  out.push_back(static_cast<std::byte>(static_cast<unsigned>(value) >> Byte));
  out.push_back(static_cast<std::byte>(value));
}

struct Walk
{
  std::uint64_t Handle{};
  std::int64_t Offset{};
};

void Append(std::vector<Mapped> &entries, std::span<const std::byte> section, Walk &walk)
{
  Bits::Reader in{section};
  while (in.Position() < in.Size())
  {
    const std::uint64_t handle = in.UMC();
    const std::int64_t offset = in.MC();
    if (in.HasFailed())
    {
      return;
    }
    walk.Handle += handle;
    walk.Offset += offset;
    if (walk.Offset >= 0)
    {
      entries.push_back(Mapped{.Self = DwgIO::Handle{walk.Handle}, .Offset = static_cast<std::size_t>(walk.Offset)});
    }
  }
}

/**
 *  Handle and offset as deltas from the section's last entry.
 */
auto EncodeEntry(const Mapped &one, const Walk &walk) -> std::vector<std::byte>
{
  Bits::Writer coded;
  coded.UMC(one.Self.Value - walk.Handle);
  coded.MC(static_cast<std::int64_t>(one.Offset) - walk.Offset);
  return coded.Take();
}

void Flush(std::vector<std::byte> &out, std::vector<std::byte> &section)
{
  std::vector<std::byte> framed;
  PutBigEndian(framed, static_cast<std::uint16_t>(section.size() + Word));
  framed.insert(framed.end(), section.begin(), section.end());
  PutBigEndian(framed, Bits::Crc(framed, Bits::Seed));
  out.insert(out.end(), framed.begin(), framed.end());
  section.clear();
}
} // namespace

auto ReadMap(std::span<const std::byte> map, std::size_t offset, std::vector<Issue> &issues) -> std::vector<Mapped>
{
  std::vector<Mapped> entries;
  for (std::size_t at = 0;;)
  {
    const bool whole = at + Word <= map.size() && BigEndian(map.subspan(at)) >= Word &&
                       at + BigEndian(map.subspan(at)) + Word <= map.size();
    if (!whole)
    {
      issues.push_back(
          Issue{.Code = IssueCode::Damaged, .Offset = offset + at, .What = "object map section runs past the end"});
      return entries;
    }
    const std::size_t size = BigEndian(map.subspan(at));
    const std::span<const std::byte> section = map.subspan(at, size);
    if (Bits::Crc(section, Bits::Seed) != BigEndian(map.subspan(at + size)))
    {
      issues.push_back(
          Issue{.Code = IssueCode::Checksum, .Offset = offset + at, .What = "object map section CRC mismatch"});
    }
    if (size == Word)
    {
      return entries;
    }
    Walk walk; // Each section counts its handles and offsets from zero, as AutoCAD writes them.
    Append(entries, section.subspan(Word), walk);
    at += size + Word;
  }
}

auto WriteMap(std::span<const Mapped> entries) -> std::vector<std::byte>
{
  std::vector<Mapped> sorted{entries.begin(), entries.end()};
  std::ranges::stable_sort(sorted, {}, &Mapped::Self);
  std::vector<std::byte> out;
  std::vector<std::byte> section;
  Walk walk;
  for (const Mapped &one : sorted)
  {
    const std::vector<std::byte> bytes = EncodeEntry(one, walk);
    section.insert(section.end(), bytes.begin(), bytes.end());
    walk.Handle = one.Self.Value;
    walk.Offset = static_cast<std::int64_t>(one.Offset);
    if (Word + section.size() >= Largest) // AutoCAD closes a section once it reaches the size, its last entry in it.
    {
      Flush(out, section);
      walk = {}; // A new section counts from zero again.
    }
  }
  if (!section.empty())
  {
    Flush(out, section);
  }
  Flush(out, section);
  return out;
}
} // namespace DwgIO::Objects
