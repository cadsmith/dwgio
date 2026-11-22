/***********************************************************************************************************************
 *  @file      tests/fuzz/acis.cpp
 *  @brief     Fuzzes the conversion of a SAT or SAB body between ACIS versions.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "acis/convert.hpp"

#include "dwgio/entities/geometry.hpp"
#include "dwgio/version.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

/**
 *  First byte: the format (SAT or SAB) and the release to write; the rest the body.
 */
extern "C" auto LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size) -> int
{
  if (size < 1)
  {
    return 0;
  }
  const int format = (data[0] & 1) + 1;
  const auto to = static_cast<DwgIO::Version>((data[0] >> 1) % 7);
  const bool remade = (data[0] & 0x80) != 0;
  DwgIO::Entities::Acis acis{.Version = format,
                             .Textual = format == 1,
                             .Data = std::vector<std::byte>(std::as_bytes(std::span{data + 1, size - 1}).begin(),
                                                            std::as_bytes(std::span{data + 1, size - 1}).end())};
  const auto converted = DwgIO::Acis::Convert(acis, to, remade);
  if (converted)
  {
    for (int release = 0; release < 7; ++release)
    {
      static_cast<void>(DwgIO::Acis::Convert(*converted, static_cast<DwgIO::Version>(release), !remade));
    }
  }
  return 0;
}
