/***********************************************************************************************************************
 *  @file      tests/fuzz/lz21.cpp
 *  @brief     Fuzzes the R2007 LZ decompressor and compressor.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/lz21.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <span>
#include <vector>

/**
 *  Decompresses whatever libFuzzer hands over as an R2007 page, and compresses it to check it comes back the same.
 */
extern "C" auto LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size) -> int
{
  constexpr std::size_t Page = 0xf800;
  const std::span<const std::byte> bytes = std::as_bytes(std::span{data, size});
  static_cast<void>(DwgIO::Bits::Lz21::Decompress(bytes, Page));
  const std::span<const std::byte> page = bytes.first(std::min(size, Page));
  if (page.empty())
  {
    return 0;
  }
  const std::optional<std::vector<std::byte>> back =
      DwgIO::Bits::Lz21::Decompress(DwgIO::Bits::Lz21::Compress(page), page.size());
  if (!back || !std::ranges::equal(*back, page))
  {
    std::abort();
  }
  return 0;
}
