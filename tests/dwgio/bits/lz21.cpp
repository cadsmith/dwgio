/***********************************************************************************************************************
 *  @file      tests/dwgio/bits/lz21.cpp
 *  @brief     The R2007 LZ compressor and decompressor: runs, near, far and long references, hostile streams.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/lz21.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace DwgIO::Bits::Lz21
{
namespace
{
/**
 *  Bytes that do not repeat, from a linear congruential generator.
 */
auto Noise(std::size_t size, std::uint32_t seed) -> std::vector<std::byte>
{
  std::vector<std::byte> bytes(size);
  for (std::byte &one : bytes)
  {
    seed = (seed * 1103515245U) + 12345U;
    one = static_cast<std::byte>(seed >> 16U);
  }
  return bytes;
}

auto Concatenate(std::initializer_list<std::vector<std::byte>> parts) -> std::vector<std::byte>
{
  std::vector<std::byte> joined;
  for (const std::vector<std::byte> &part : parts)
  {
    joined.insert(joined.end(), part.begin(), part.end());
  }
  return joined;
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

/**
 *  The bytes compressed and back.
 */
auto RoundTrip(std::span<const std::byte> bytes) -> std::optional<std::vector<std::byte>>
{
  return Decompress(Compress(bytes), bytes.size());
}
} // namespace

TEST_CASE("bits/lz21: runs of every length class read back from one byte to more than 0xFFFF")
{
  for (const std::size_t size :
       {std::size_t{1}, std::size_t{7}, std::size_t{8}, std::size_t{0x16}, std::size_t{0x17}, std::size_t{0x17 + 0xFE},
        std::size_t{0x17 + 0xFF}, std::size_t{0x17 + 0xFF + 0xFFFF}, std::size_t{0x17 + 0xFF + 0xFFFF + 5}})
  {
    CAPTURE(size);
    const std::vector<std::byte> noise = Noise(size, static_cast<std::uint32_t>(size));
    CHECK(RoundTrip(noise) == noise);
  }
  CHECK(Compress({}).empty());
}

TEST_CASE("bits/lz21: near far and long references read back with short and long runs after them")
{
  const std::vector<std::byte> word = Noise(40, 1);
  const std::vector<std::byte> gap = Noise(9000, 2);
  for (const std::vector<std::byte> &bytes : {
           Concatenate({word, word}),                                        // A short reference.
           Concatenate({Noise(300, 3), Noise(16, 4), Noise(300, 5)}),        // None at all.
           Concatenate({Noise(600, 6), Noise(600, 6)}),                      // A long one, cut in parts.
           Concatenate({Noise(20, 7), Noise(1000, 8), Noise(20, 7)}),        // A near one.
           Concatenate({word, gap, word, Noise(3, 9), word, Noise(30, 10)}), // A far one, runs of three and thirty.
           Concatenate({Noise(257, 11), Noise(257, 11)}),                    // 257 bytes: a part of 254, then three.
           Concatenate({Noise(5, 12), std::vector<std::byte>(300, std::byte{0})}),     // A reference of distance one.
           Concatenate({Noise(5, 13), std::vector<std::byte>(0x20000, std::byte{0})}), // Long ones, cut in parts.
           Concatenate({Noise(40, 14), Noise(0x10000, 15), Noise(40, 14)}),            // Too far back to refer to.
           // A reference longer than the long form holds, by two bytes: a part three short, then three.
           Concatenate({Noise(5, 16), std::vector<std::byte>(0x100 + 0xFFFF + 3, std::byte{0})}),
       })
  {
    CAPTURE(bytes.size());
    CHECK(RoundTrip(bytes) == bytes);
    CHECK(Compress(bytes).size() <= bytes.size() + 8);
  }
  CHECK(Compress(std::vector<std::byte>(0x10000, std::byte{0})).size() < 0x10);
}

TEST_CASE("bits/lz21: a stream that refers before its start or outgrows its size or ends early fails")
{
  CHECK_FALSE(Decompress({}, 10));
  // A run of eight, then a reference 0x200 back.
  const std::vector<std::byte> eight = Concatenate({MakeBytes({0x00}), Noise(8, 1)});
  CHECK(Decompress(eight, 8) == std::optional{Noise(8, 1)});
  CHECK_FALSE(Decompress(eight, 7));
  CHECK_FALSE(Decompress(Concatenate({eight, MakeBytes({0x3F, 0xF8})}), 100));
  CHECK_FALSE(Decompress(Concatenate({eight, MakeBytes({0x10})}), 100));
  CHECK_FALSE(Decompress(Concatenate({eight, MakeBytes({0x30, 0x00})}), 10));
  CHECK(Decompress(MakeBytes({0x0F}), 100) == std::optional{std::vector<std::byte>{}}); // An opcode with no run after.
  CHECK_FALSE(Decompress(MakeBytes({0x0F, 0xFF, 0x01}), 100));
  // A run of eight, then a reference of five bytes and no distance, which would read past the output.
  CHECK_FALSE(Decompress(Concatenate({eight, MakeBytes({0x25, 0x00, 0x00, 0x00})}), 100));
  // A reference of no length may have no distance: it copies nothing.
  CHECK(Decompress(Concatenate({eight, MakeBytes({0x20, 0x00, 0x00, 0x00})}), 100) == std::optional{Noise(8, 1)});
  CHECK_FALSE(Decompress(MakeBytes({0x05, 1, 2}), 100));
}

TEST_CASE("bits/lz21: the page checksum of a few bytes and of more than one chunk matches")
{
  const std::vector<std::byte> digits = MakeBytes({'1', '2', '3', '4', '5', '6', '7', '8', '9'});
  CHECK(Checksum(0, digits) == 0x21910485U);
  std::vector<std::byte> counting;
  for (int round = 0; round < 30; ++round)
  {
    for (unsigned one = 0; one < 256; ++one)
    {
      counting.push_back(static_cast<std::byte>(one));
    }
  }
  CHECK(Checksum(0, counting) == 0x599a36a4U);
}
} // namespace DwgIO::Bits::Lz21
