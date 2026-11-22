/***********************************************************************************************************************
 *  @file      tests/fuzz/lz.cpp
 *  @brief     Fuzzes the R2004 LZ decompressor and compressor.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/lz.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <span>
#include <vector>

/**
 *  Decompresses whatever libFuzzer hands over as a page, and compresses it to check it comes back the same.
 */
extern "C" auto LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size) -> int
{
  const std::span<const std::byte> bytes = std::as_bytes(std::span{data, size});
  std::vector<std::byte> out;
  static_cast<void>(DwgIO::Bits::Lz::Decompress(bytes, 0x7400, out));
  std::vector<std::byte> back;
  const std::span<const std::byte> page = bytes.first(std::min<std::size_t>(size, 0x7400));
  if (!DwgIO::Bits::Lz::Decompress(DwgIO::Bits::Lz::Compress(page), 0x7400, back))
  {
    std::abort();
  }
  const std::size_t least = page.size() < 4 ? 4 : page.size();
  if (back.size() != least || !std::equal(page.begin(), page.end(), back.begin()))
  {
    std::abort();
  }
  return 0;
}
