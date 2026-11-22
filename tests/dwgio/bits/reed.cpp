/***********************************************************************************************************************
 *  @file      tests/dwgio/bits/reed.cpp
 *  @brief     The Reed-Solomon coding of R2007 files, against an R2007 fixture's header.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/reed.hpp"

#include "support/fixtures.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace DwgIO::Bits::Reed
{
namespace
{
auto Counting(std::size_t size) -> std::vector<std::byte>
{
  std::vector<std::byte> bytes(size);
  for (std::size_t at = 0; at < size; ++at)
  {
    bytes[at] = static_cast<std::byte>(at);
  }
  return bytes;
}

auto MakeBytes(std::initializer_list<unsigned> values) -> std::vector<std::byte>
{
  std::vector<std::byte> bytes;
  for (const unsigned one : values)
  {
    bytes.push_back(static_cast<std::byte>(one));
  }
  return bytes;
}
} // namespace

TEST_CASE("bits/reed: each code's codeword puts its parity after the data")
{
  const std::vector<std::byte> page = Interleave(Counting(Pages.Data), Pages);
  REQUIRE(page.size() == Codeword);
  CHECK(std::ranges::equal(std::span{page}.first(Pages.Data), Counting(Pages.Data)));
  CHECK(std::ranges::equal(std::span{page}.subspan(Pages.Data), MakeBytes({0x02, 0xa7, 0x81, 0xe8})));
  const std::vector<std::byte> system = Interleave(Counting(System.Data), System);
  CHECK(std::ranges::equal(
      std::span{system}.subspan(System.Data),
      MakeBytes({0xac, 0xd4, 0x64, 0xed, 0x89, 0x90, 0x06, 0xa1, 0xf7, 0xaf, 0xe8, 0xd3, 0x6a, 0x8d, 0xea, 0xe7})));
  CHECK(Interleave({}, Pages).empty());
}

TEST_CASE("bits/reed: an R2007 file header codes back to AutoCAD's bytes with three codewords interleaved")
{
  constexpr std::size_t At = 0x80;
  constexpr std::size_t Blocks = 3;
  const std::vector<std::byte> file = ReadFixture("empty-r2007.dwg");
  const std::span<const std::byte> coded = std::span{file}.subspan(At, Blocks * Codeword);
  std::vector<std::byte> data;
  for (std::size_t block = 0; block < Blocks; ++block)
  {
    for (std::size_t at = 0; at < System.Data; ++at)
    {
      data.push_back(coded[(Blocks * at) + block]);
    }
  }
  CHECK(std::ranges::equal(Interleave(data, System), coded));
  // A last codeword the data does not fill is padded with zeros.
  data.resize(data.size() - 10);
  const std::vector<std::byte> shorter = Interleave(data, System);
  CHECK(shorter.size() == Blocks * Codeword);
  CHECK(shorter[(Blocks * (System.Data - 1)) + 2] == std::byte{0});
}
} // namespace DwgIO::Bits::Reed
