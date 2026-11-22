/***********************************************************************************************************************
 *  @file      tests/dwgio/tables/viewportheader.cpp
 *  @brief     The viewport entity headers of R14 and R2000, chained and written back.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/tables/viewportheader.hpp"

#include "fields/context.hpp"
#include "objects/read.hpp"
#include "objects/write.hpp"
#include "support/fixtures.hpp"
#include "support/objects.hpp"
#include "support/reader.hpp"

#include "dwgio/entities/viewport.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tables/control.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Tables
{
TEST_CASE("tables/viewportheader: every viewport header names a viewport and chains in its control")
{
  for (const std::string_view fixture : {"objects", "curves"})
  {
    for (const std::string_view release : {"-r2000.dwg", "-r14.dwg"})
    {
      const std::string name = std::string{fixture} + std::string{release};
      CAPTURE(name);
      const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
      REQUIRE(loaded);
      CHECK(loaded->Issues.empty());
      const Drawing &drawing = loaded->Drawing;
      const Control *control = nullptr;
      for (const Object &object : drawing.Objects)
      {
        if (const auto *one = GetIf<Control>(&object); one != nullptr && one->Table == Table::ViewportEntityHeader)
        {
          control = one;
        }
      }
      REQUIRE(control);
      CHECK(control->Base.Self == drawing.Header.ViewportEntityHeaderControlObject);
      std::vector<Handle> headers;
      std::vector<Handle> viewports;
      std::size_t roots = 0;
      for (const Object &object : drawing.Objects)
      {
        const auto *header = GetIf<ViewportEntityHeader>(&object);
        if (header == nullptr)
        {
          continue;
        }
        headers.push_back(header->Record.Base.Self);
        CAPTURE(HexOf(header->Record.Base.Self));
        CHECK(TypeOf(object) == "VP_ENT_HDR");
        CHECK(header->Record.Base.Owner == control->Base.Self);
        CHECK(header->Record.XrefIndex == -1);
        CHECK_FALSE(header->Record.Dependent);
        CHECK(header->Record.Source == Handle{});
        CHECK(std::ranges::contains(control->Entries, header->Record.Base.Self));
        // One header per drawing stands at the chain's root, with no viewport and its flag clear.
        if (header->Viewport == Handle{})
        {
          CHECK_FALSE(header->Flag);
          CHECK(header->Previous == Handle{});
          ++roots;
          continue;
        }
        CHECK(header->Flag);
        const auto *viewport = GetIf<Entities::Viewport>(Objects::FindObject(drawing, header->Viewport));
        REQUIRE(viewport);
        viewports.push_back(header->Viewport);
        if (header->Previous != Handle{})
        {
          CHECK(Holds<ViewportEntityHeader>(*Objects::FindObject(drawing, header->Previous)));
        }
      }
      CHECK(headers.size() == (fixture == "objects" ? 5 : 6));
      CHECK(headers.size() == control->Entries.size());
      CHECK(roots == 1);
      CHECK(viewports.size() == headers.size() - 1);
      std::ranges::sort(viewports);
      CHECK(std::ranges::adjacent_find(viewports) == viewports.end());
    }
  }
}

TEST_CASE("tables/viewportheader: a viewport header round-trips as R2000")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin, .Issues = &issues};
  const Objects::FileContext reading{
      .Release = Version::R2000, .Text = &Objects::Latin, .Classes = {}, .Issues = &issues};
  ViewportEntityHeader header;
  header.Record.Base.Self = Handle{0x100};
  header.Record.Base.Owner = Handle{0xF1};
  header.Record.Name = "";
  header.Flag = true;
  header.Viewport = Handle{0x101};
  header.Previous = Handle{0xF9};
  const std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(Object{header}, with), 0, reading);
  REQUIRE(back);
  CHECK(*back == Object{header});
  CHECK(TypeOf(*back) == "VP_ENT_HDR");
  CHECK(issues.empty());
}
} // namespace DwgIO::Tables
