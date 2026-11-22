/***********************************************************************************************************************
 *  @file      tests/fuzz/load.cpp
 *  @brief     Fuzzes loading a drawing and saving it to its own release.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/load.hpp"

#include "dwgio/save.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

/**
 *  Loads whatever libFuzzer hands over and, when it is a drawing, saves it back to its own release.
 */
extern "C" auto LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size) -> int
{
  const std::span<const std::byte> bytes = std::as_bytes(std::span{data, size});
  if (const auto loaded = DwgIO::Load(bytes))
  {
    static_cast<void>(DwgIO::Save(loaded->Drawing));
  }
  return 0;
}
