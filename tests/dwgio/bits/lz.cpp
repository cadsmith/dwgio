/***********************************************************************************************************************
 *  @file      tests/dwgio/bits/lz.cpp
 *  @brief     The R2004 LZ compressor and decompressor: opcodes, windows, far matches and hostile streams.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/lz.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <random>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace DwgIO::Bits
{
namespace
{
auto Literal(std::vector<std::byte> head, int count) -> std::vector<std::byte>
{
  for (int at = 0; at < count; ++at)
  {
    head.push_back(static_cast<std::byte>(at + 1));
  }
  head.push_back(std::byte{0x11});
  return head;
}

/**
 *  Four literals 1 to 4, then an opcode of the 0x40 family copying three bytes from `back` bytes back with one literal
 *  9 after it, then the end.
 */
auto Copying(std::uint8_t back) -> std::vector<std::byte>
{
  return {std::byte{0x01},
          std::byte{1},
          std::byte{2},
          std::byte{3},
          std::byte{4},
          std::byte{static_cast<std::uint8_t>(0x41U | (((back - 1U) & 3U) << 2U))},
          std::byte{static_cast<std::uint8_t>((back - 1U) >> 2U)},
          std::byte{9},
          std::byte{0x11}};
}
} // namespace

TEST_CASE("bits/lz: literal lengths are decoded and the 0x0F the text leaves out restored")
{
  for (const auto &[head, count] : {std::pair{std::vector<std::byte>{std::byte{0x05}}, 8},
                                    std::pair{std::vector<std::byte>{std::byte{0x00}, std::byte{0x02}}, 0x14},
                                    std::pair{std::vector<std::byte>{std::byte{0x0F}}, 18}})
  {
    CAPTURE(count);
    std::vector<std::byte> out;
    REQUIRE(Lz::Decompress(Literal(head, count), 0x7400, out));
    CHECK(out.size() == static_cast<std::size_t>(count));
    CHECK(out.back() == static_cast<std::byte>(count));
  }
}

TEST_CASE("bits/lz: a copy reaches into the window a page carries and not before it")
{
  std::vector<std::byte> out;
  CHECK_FALSE(Lz::Decompress(Copying(17), 64, out));
  std::vector<std::byte> with(16, std::byte{7});
  REQUIRE(Lz::Decompress(Copying(17), 64, with));
  CHECK(with.size() == 16 + 4 + 3 + 1);
  CHECK(with[20] == std::byte{7});
  CHECK(with[21] == std::byte{7});
  CHECK(with[22] == std::byte{7});
  CHECK(with[23] == std::byte{9});
  std::vector<std::byte> own;
  REQUIRE(Lz::Decompress(Copying(4), 64, own));
  CHECK(own == std::vector<std::byte>{std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, std::byte{1},
                                      std::byte{2}, std::byte{3}, std::byte{9}});
}

TEST_CASE("bits/lz: undecodable input is refused and output is capped at the limit")
{
  std::vector<std::byte> out;
  const std::vector<std::byte> four{std::byte{0x01}, std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}};
  std::vector<std::byte> odd = four;
  odd.push_back(std::byte{0x09});
  CHECK_FALSE(Lz::Decompress(odd, 64, out));
  CHECK_FALSE(Lz::Decompress(four, 64, out));
  std::vector<std::byte> ended = four;
  ended.push_back(std::byte{0x11});
  CHECK_FALSE(Lz::Decompress(ended, 3, out));
  std::vector<std::byte> most;
  CHECK(Lz::Decompress(ended, 4, most));
  CHECK(most.size() == 4);
  std::vector<std::byte> cut = Copying(4);
  cut.pop_back();
  cut.pop_back();
  CHECK_FALSE(Lz::Decompress(cut, 64, out));
  // A stream cut inside each token: the literal length, a long length, an offset, a close copy's second byte.
  for (const std::vector<std::uint8_t> &tail : {std::vector<std::uint8_t>{},
                                                {0x00},
                                                {0x20},
                                                {0x20, 0x00},
                                                {0x21},
                                                {0x21, 0x04},
                                                {0x40},
                                                {0x10},
                                                {0x10, 0x05},
                                                {0x12},
                                                {0x12, 0x04}})
  {
    std::vector<std::byte> stream = tail.empty() ? std::vector<std::byte>{} : four;
    for (const std::uint8_t one : tail)
    {
      stream.push_back(std::byte{one});
    }
    CAPTURE(tail.size());
    CHECK_FALSE(Lz::Decompress(stream, 64, out));
  }
  // A copy that would run past the most allowed, and a stream that ends right after a copy wanting literals.
  CHECK_FALSE(Lz::Decompress(Copying(4), 5, out));
  std::vector<std::byte> ending = four;
  ending.push_back(std::byte{0x40});
  ending.push_back(std::byte{0x00});
  CHECK_FALSE(Lz::Decompress(ending, 64, out));
  // A literal run after a copy that outruns the input.
  std::vector<std::byte> outrun = four;
  for (const std::uint8_t one : {std::uint8_t{0x40}, std::uint8_t{0x00}, std::uint8_t{0x05}})
  {
    outrun.push_back(std::byte{one});
  }
  CHECK_FALSE(Lz::Decompress(outrun, 64, out));
  // A long literal length with nothing after it, and a stream that is nothing but its end.
  CHECK_FALSE(Lz::Decompress(std::vector<std::byte>{std::byte{0x00}}, 64, out));
  std::vector<std::byte> none;
  CHECK(Lz::Decompress(std::vector<std::byte>{std::byte{0x11}}, 64, none));
  CHECK(none.empty());
}

TEST_CASE("bits/lz: a match further back than the far family reaches is skipped")
{
  // The hash remembers the last place of three bytes; a place 0x8000 or more bytes back is out of every family's
  // reach, so the bytes go as literals.
  std::vector<std::byte> input(4, std::byte{9}); // The opening literals, which the hash never sees.
  for (const char one : {'A', 'B', 'C', 'D', 'E'})
  {
    input.push_back(static_cast<std::byte>(one));
  }
  input.resize(input.size() + 0x8000, std::byte{0});
  for (const char one : {'A', 'B', 'C', 'D', 'E'})
  {
    input.push_back(static_cast<std::byte>(one));
  }
  input.resize(input.size() + 16, std::byte{7});
  const std::vector<std::byte> packed = Lz::Compress(input);
  std::vector<std::byte> out;
  REQUIRE(Lz::Decompress(packed, input.size(), out));
  CHECK(out == input);
}

TEST_CASE("bits/lz: everything compressed decompresses to itself")
{
  std::mt19937 random{7};
  std::vector<std::vector<std::byte>> inputs{
      std::vector<std::byte>(4, std::byte{1}), std::vector<std::byte>(5, std::byte{2}),
      std::vector<std::byte>(0x7400, std::byte{0}), std::vector<std::byte>(0x7400, std::byte{0xAB})};
  std::vector<std::byte> noise(0x7400);
  for (std::byte &one : noise)
  {
    one = static_cast<std::byte>(random() & 0xFFU);
  }
  inputs.push_back(noise);
  std::vector<std::byte> runs;
  for (int at = 0; at < 0x7400; ++at)
  {
    runs.push_back(static_cast<std::byte>((at / 700) & 0xFF));
  }
  inputs.push_back(runs);
  // Noise that repeats 0x4000 bytes later, so a match lies past the two-byte offsets' reach.
  std::vector<std::byte> far(0x7400);
  for (std::size_t at = 0; at < far.size(); ++at)
  {
    far[at] = at < 0x4000 ? static_cast<std::byte>(random() & 0xFFU) : far[at - 0x4000];
  }
  inputs.push_back(far);
  for (const std::vector<std::byte> &input : inputs)
  {
    CAPTURE(input.size());
    const std::vector<std::byte> packed = Lz::Compress(input);
    REQUIRE(packed.size() >= 3);
    CHECK(packed[packed.size() - 3] == std::byte{0x11});
    CHECK(packed[packed.size() - 2] == std::byte{0});
    CHECK(packed.back() == std::byte{0});
    std::vector<std::byte> out;
    REQUIRE(Lz::Decompress(packed, 0x7400, out));
    CHECK(out == input);
    if (input.size() == 0x7400 && input != noise && input != far)
    {
      CHECK(packed.size() < input.size() / 2);
    }
  }
  // Fewer than four bytes open no stream: they are padded with zeros to four.
  for (const std::vector<std::byte> &input :
       {std::vector<std::byte>{}, std::vector<std::byte>{std::byte{1}}, std::vector<std::byte>(3, std::byte{2})})
  {
    std::vector<std::byte> out;
    REQUIRE(Lz::Decompress(Lz::Compress(input), 4, out));
    CHECK(out.size() == 4);
    CHECK(std::ranges::equal(std::span{out}.first(input.size()), input));
    CHECK(std::ranges::all_of(std::span{out}.subspan(input.size()), [](std::byte one) { return one == std::byte{0}; }));
  }
  // The far input is coded with a long-distance opcode, its second half a few copies.
  const std::vector<std::byte> packedFar = Lz::Compress(far);
  CHECK(packedFar.size() < 0x4000 + 0x100);
  CHECK(std::ranges::any_of(packedFar, [](std::byte one) { return one >= std::byte{0x10} && one <= std::byte{0x1F}; }));
}
} // namespace DwgIO::Bits

namespace DwgIO::Bits
{
TEST_CASE("bits/lz: a copy stops thirteen bytes before the end and a literal run closes the stream")
{
  const std::vector<std::byte> zeros(64);
  const std::vector<std::byte> packed = Lz::Compress(zeros);
  std::vector<std::byte> tail(13, std::byte{0});
  tail.insert(tail.begin(), std::byte{0x0A});
  tail.insert(tail.end(), {std::byte{0x11}, std::byte{0}, std::byte{0}});
  REQUIRE(packed.size() > tail.size());
  CHECK(std::equal(tail.begin(), tail.end(), packed.end() - static_cast<std::ptrdiff_t>(tail.size())));
}

TEST_CASE("bits/lz: a far copy of ten bytes or more takes the long form")
{
  // AutoCAD's streams never carry the opcodes 0x18 to 0x1F: beyond 0x3FFF a copy of 4 to 9 bytes is 0x12 to 0x17
  // and a longer one is 0x10 with its length counted from 9.
  std::vector<std::byte> input;
  for (int at = 1; at <= 20; ++at)
  {
    input.push_back(static_cast<std::byte>(at));
  }
  input.resize(input.size() + 0x4000, std::byte{0});
  for (int at = 1; at <= 20; ++at)
  {
    input.push_back(static_cast<std::byte>(at));
  }
  for (int at = 100; at < 120; ++at)
  {
    input.push_back(static_cast<std::byte>(at));
  }
  const std::vector<std::byte> packed = Lz::Compress(input);
  // Sixteen bytes at a distance of 0x4000 + 20: the long form, length 16 - 9, offset 20 and no inline literals.
  const std::vector<std::byte> wanted{std::byte{0x10}, std::byte{0x07}, std::byte{0x50}, std::byte{0x00}};
  CHECK(std::ranges::search(packed, wanted).begin() != packed.end());
  std::vector<std::byte> out;
  REQUIRE(Lz::Decompress(packed, input.size(), out));
  CHECK(out == input);
}

TEST_CASE("bits/lz: a copy follows a copy directly when no literal lies between")
{
  // AutoCAD's streams put an opcode straight after a copy with no inline literals; the text's decoder reads it.
  std::vector<std::byte> input;
  const auto run = [&input](int from, int count)
  {
    for (int at = from; at < from + count; ++at)
    {
      input.push_back(static_cast<std::byte>(at));
    }
  };
  run(0xF0, 4); // Before the first hashed position.
  run(1, 8);    // X at 4.
  run(11, 8);   // Y at 12.
  run(200, 16); // Filler at 20.
  run(11, 8);   // Y again at 36: a copy of 8 at a distance of 24.
  run(1, 8);    // X again at 44: a copy of 8 at a distance of 40, directly after.
  run(100, 16); // The literals that close the stream.
  const std::vector<std::byte> packed = Lz::Compress(input);
  const std::vector<std::byte> wanted{std::byte{0x9C}, std::byte{0x05}, std::byte{0x9C}, std::byte{0x09},
                                      std::byte{0x0D}};
  CHECK(std::ranges::search(packed, wanted).begin() != packed.end());
  std::vector<std::byte> out;
  REQUIRE(Lz::Decompress(packed, input.size(), out));
  CHECK(out == input);
}
} // namespace DwgIO::Bits
