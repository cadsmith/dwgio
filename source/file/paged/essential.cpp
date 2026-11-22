/***********************************************************************************************************************
 *  @file      source/file/paged/essential.cpp
 *  @brief     The sections AutoCAD refuses a paged file without.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/paged/essential.hpp"

#include "bits/writer.hpp"

#include <array>
#include <cstdint>

namespace DwgIO::File::Paged
{
namespace
{
/**
 *  The version code of chapter 27 for each release written with an auxiliary header.
 */
auto Code(Version release) -> std::uint16_t
{
  constexpr std::uint16_t R2000Code = 23;
  constexpr std::uint16_t R2004Code = 25;
  constexpr std::uint16_t R2010Code = 29;
  constexpr std::uint16_t R2013Code = 31;
  constexpr std::uint16_t R2018Code = 33;
  switch (release)
  {
  case Version::R2000:
    return R2000Code;
  case Version::R2010:
    return R2010Code;
  case Version::R2013:
    return R2013Code;
  case Version::R2018:
    return R2018Code;
  default:
    return R2004Code;
  }
}

constexpr std::uint32_t Saves = 1;
constexpr std::uint16_t Unknown1 = 0x0005; /**< Unnamed in the specification; AutoCAD writes it twice. */
constexpr std::uint16_t Unknown2 = 0x0893; /**< Unnamed in the specification; after each Unknown1.     */
constexpr std::uint64_t SeedMost = 0x7FFFFFFF;
} // namespace

auto WriteRevHistory() -> std::vector<std::byte>
{
  Bits::Writer out;
  out.RL(0);
  out.RL(0);
  out.RL(1);
  out.RL(0);
  return out.Take();
}

auto WriteObjFreeSpace(const Header &header, Version release, std::size_t objects) -> std::vector<std::byte>
{
  constexpr std::array<std::uint32_t, 4> Values{0x32, 0x64, 0x200, 0xFFFFFFFF};
  Bits::Writer out;
  if (release >= Version::R2010)
  {
    // From R2010 a zero and the object count as 64-bit values, the update time, and the four pairs of 64 bits.
    out.RLL(0);
    out.RLL(objects);
    out.RL(static_cast<std::uint32_t>(header.TdupdateDays));
    out.RL(static_cast<std::uint32_t>(header.TdupdateMilliseconds));
    out.RC(4);
    for (const std::uint32_t value : Values)
    {
      out.RLL(value);
      out.RLL(0);
    }
    return out.Take();
  }
  // The text says a zero and an object count open it; AutoCAD writes 0xFFFF0000 twice. Then the update time, a
  // zero offset of the objects section, and four 64-bit values the text lists.
  out.RL(0xFFFF0000U);
  out.RL(0xFFFF0000U);
  out.RL(static_cast<std::uint32_t>(header.TdupdateDays));
  out.RL(static_cast<std::uint32_t>(header.TdupdateMilliseconds));
  out.RL(0);
  out.RC(4);
  for (const std::uint32_t value : Values)
  {
    out.RL(value);
    out.RL(0);
  }
  return out.Take();
}

auto WriteTemplate(Version release) -> std::vector<std::byte>
{
  // Up to R2004 an empty description and MEASUREMENT 1; from R2007 the description's count is 1, its one unit 0.
  Bits::Writer out;
  if (release >= Version::R2007)
  {
    out.RS(1);
    out.RS(0);
  }
  else
  {
    out.RS(0);
  }
  out.RS(1);
  return out.Take();
}

auto WriteAuxHeader(const Header &header, Version release, int maintenance) -> std::vector<std::byte>
{
  Bits::Writer out;
  out.RC(0xFF);
  out.RC(0x77);
  out.RC(0x01);
  const std::uint16_t code = Code(release);
  const auto maint = static_cast<std::uint16_t>(maintenance);
  // R2018 adds a zero short after each version and maintenance pair, where the text puts three at the end.
  const auto pair = [&out, code, maint, release] -> void
  {
    out.RS(code);
    out.RS(maint);
    if (release >= Version::R2018)
    {
      out.RS(0);
    }
  };
  pair();
  out.RL(Saves);
  out.RL(0xFFFFFFFFU);
  out.RS(static_cast<std::uint16_t>(Saves)); // Part 1: the saves less part 2.
  out.RS(0);                                 // Part 2: the saves over 0x7fff.
  out.RL(0);
  pair();
  pair();
  out.RS(Unknown1);
  out.RS(Unknown2);
  out.RS(Unknown1);
  out.RS(Unknown2);
  out.RS(0);
  out.RS(1);
  for (int at = 0; at < 5; ++at)
  {
    out.RL(0);
  }
  out.RL(static_cast<std::uint32_t>(header.TdcreateDays));
  out.RL(static_cast<std::uint32_t>(header.TdcreateMilliseconds));
  out.RL(static_cast<std::uint32_t>(header.TdupdateDays));
  out.RL(static_cast<std::uint32_t>(header.TdupdateMilliseconds));
  out.RL(header.Handseed.Value < SeedMost ? static_cast<std::uint32_t>(header.Handseed.Value) : 0xFFFFFFFFU);
  out.RL(0); // The educational plot stamp.
  out.RS(0);
  out.RS(static_cast<std::uint16_t>(Saves)); // Part 1 less part 2.
  out.RL(0);
  out.RL(0);
  out.RL(0);
  out.RL(Saves);
  for (int at = 0; at < 4; ++at)
  {
    out.RL(0);
  }
  return out.Take();
}
} // namespace DwgIO::File::Paged
