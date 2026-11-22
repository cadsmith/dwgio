/***********************************************************************************************************************
 *  @file      tests/dwgio/file/sections/information.cpp
 *  @brief     The application information section, naming dwgio as AutoCAD 2026 lays it out.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/sections/information.hpp"

#include "encoding/encoding.hpp"
#include "file/sections/text.hpp"
#include "support/sections.hpp"

#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <string>
#include <vector>

namespace DwgIO::File::Sections
{
TEST_CASE("file/sections/information: the application information names dwgio in AutoCAD 2026's layout")
{
  const std::vector<std::byte> made = WriteInformation();
  const Encoding::Scheme wide = Encoding::Utf8();
  TextReader in{made, Version::R2018, wide};
  CHECK(in.Long() == 3);
  CHECK(in.Text(TextCount::Ended) == "AppInfoDataList");
  CHECK(in.Long() == 1);
  // The GUID AutoCAD gives the product's entry, its own as it writes it after a file another application saved.
  const std::vector<std::byte> product =
      Octets({0xe8, 0xe0, 0x96, 0x51, 0xc5, 0xce, 0xb2, 0x44, 0xa8, 0xbf, 0xf6, 0xe8, 0x3b, 0x85, 0x9d, 0x44});
  const std::vector<std::byte> guid(made.begin() + 42, made.begin() + 58);
  CHECK(guid == product);
  const std::vector<std::byte> after(made.begin() + 58, made.end());
  TextReader rest{after, Version::R2018, wide};
  const std::string text = rest.Text(TextCount::Ended);
  CHECK(text.starts_with(R"(<ProductInformation name ="dwgio" build_version=")"));
  CHECK_FALSE(rest.HasFailed());
  CHECK(WriteInformation() == made);
}
} // namespace DwgIO::File::Sections
