/***********************************************************************************************************************
 *  @file      tests/fuzz/cross.cpp
 *  @brief     Fuzzes loading a drawing and saving it to another release.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/load.hpp"
#include "dwgio/save.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

/**
 *  Byte 0: the release to save to; the rest a drawing.
 */
extern "C" auto LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size) -> int
{
  if (size < 2)
  {
    return 0;
  }
  const std::span<const std::byte> bytes = std::as_bytes(std::span{data + 1, size - 1});
  if (const auto loaded = DwgIO::Load(bytes))
  {
    DwgIO::SaveOptions options;
    options.Release = static_cast<DwgIO::Version>(data[0] % 7);
    static_cast<void>(DwgIO::Save(loaded->Drawing, options));
  }
  return 0;
}
