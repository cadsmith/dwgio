/***********************************************************************************************************************
 *  @file      source/file/sections/information.cpp
 *  @brief     The application information section, chapter 16.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/sections/information.hpp"

#include "encoding/encoding.hpp"
#include "file/sections/text.hpp"

#include "dwgio/library.hpp"
#include "dwgio/version.hpp"

#include <array>
#include <cstdint>
#include <string>

namespace DwgIO::File::Sections
{
namespace
{
/**
 *  The version of the layout AutoCAD 2026 writes.
 */
constexpr std::uint32_t Layout = 3;

/**
 *  The one entry, the product's.
 */
constexpr std::uint32_t Entries = 1;

/**
 *  The GUID AutoCAD gives the product's entry, as stored.
 */
constexpr std::array<std::uint8_t, 16> ProductGuid{0xe8, 0xe0, 0x96, 0x51, 0xc5, 0xce, 0xb2, 0x44,
                                                   0xa8, 0xbf, 0xf6, 0xe8, 0x3b, 0x85, 0x9d, 0x44};
} // namespace

auto WriteInformation() -> std::vector<std::byte>
{
  const Encoding::Scheme wide = Encoding::Utf8();
  TextWriter out{Version::R2018, wide};
  out.Long(Layout);
  out.Text("AppInfoDataList", TextCount::Ended);
  out.Long(Entries);
  for (const std::uint8_t one : ProductGuid)
  {
    out.Byte(one);
  }
  out.Text(std::string{R"(<ProductInformation name ="dwgio" build_version=")"} + std::string{LibraryVersion} + R"("/>)",
           TextCount::Ended);
  return out.Take();
}
} // namespace DwgIO::File::Sections
