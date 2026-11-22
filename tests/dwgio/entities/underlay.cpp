/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/underlay.cpp
 *  @brief     The PDF, DWF and DGN underlays and their definitions against their DXF twin.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/underlay.hpp"

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "entities/shapes/underlay.hpp"
#include "fields/context.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "objects/context.hpp"
#include "objects/read.hpp"
#include "objects/write.hpp"
#include "releases/omission.hpp"
#include "support/acad.hpp"
#include "support/entities.hpp"
#include "support/fixtures.hpp"
#include "support/objects.hpp"
#include "support/reader.hpp"
#include "support/saves.hpp"

#include "dwgio/class.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/underlay.hpp"
#include "dwgio/view.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <expected>
#include <format>
#include <numbers>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Entities
{
namespace
{
/**
 *  A group's value as a number, or the fallback when the twin leaves the group out at its default.
 */
auto Or(const std::vector<Pair> &twin, std::size_t opened, int code, double fallback) -> double
{
  const std::string value = GroupOf(twin, opened, code);
  return value.empty() ? fallback : std::stod(value);
}

void CheckOverlaid(const std::vector<Pair> &twin, std::size_t opened, const UnderlayData &overlaid)
{
  constexpr double Degree = std::numbers::pi / 180;
  CHECK(overlaid.Definition == ParseHandle(GroupOf(twin, opened, 340)).value_or(Handle{}));
  CHECK(Near(overlaid.Position, ReadPoint(twin, opened, 10)));
  CHECK(Near(overlaid.Scale,
             Vector{.X = Or(twin, opened, 41, 1), .Y = Or(twin, opened, 42, 1), .Z = Or(twin, opened, 43, 1)}));
  CHECK(overlaid.Rotation == doctest::Approx(Or(twin, opened, 50, 0) * Degree));
  CHECK(Near(overlaid.Extrusion, Normal(twin, opened)));
  CHECK(overlaid.Flags == ReadWhole(twin, opened, 280));
  CHECK(overlaid.Contrast == ReadWhole(twin, opened, 281));
  CHECK(overlaid.Fade == ReadWhole(twin, opened, 282));
  CHECK(overlaid.Clip.size() == Every(twin, opened, 11).size());
}

/**
 *  Checks the underlay of the kind against the twin and its definition against the definition's record; the count of
 *  such underlays.
 */
template <UnderlayKind KindV>
auto CheckKind(const Drawing &drawing, const std::vector<Pair> &twin, std::string_view dictionary) -> std::size_t
{
  std::size_t count = 0;
  for (const Object &object : drawing.Objects)
  {
    const auto *underlay = GetIf<UnderlayOf<KindV>>(&object);
    if (underlay == nullptr)
    {
      continue;
    }
    ++count;
    CAPTURE(TypeOf(object));
    const std::optional<std::size_t> opened = Entity(twin, underlay->Base.Self);
    REQUIRE(opened);
    CheckOverlaid(twin, *opened, underlay->Data);
    CheckCommon(TwinPlace{.Twin = &twin, .At = *opened, .Drawing = &drawing}, underlay->Common, underlay->Base);
    const auto *definition = GetIf<Objects::UnderlayDefinition<KindV>>(Find(drawing, underlay->Data.Definition));
    REQUIRE(definition != nullptr);
    const std::optional<std::size_t> defined = Objects::FindHandled(twin, definition->Base.Self);
    REQUIRE(defined);
    CHECK(definition->File == GroupOf(twin, *defined, 1));
    CHECK(definition->Name == GroupOf(twin, *defined, 2));
    const Objects::Dictionary *book = View::Dictionary(drawing, dictionary);
    REQUIRE(book != nullptr);
    CHECK(definition->Base.Owner == book->Base.Self);
    // R14 keys the entry by the file's name in upper case, its dot an underscore: LEVE_PDF.
    CHECK(std::ranges::any_of(book->Entries, [definition](const Objects::Entry &entry) -> bool
                              { return entry.Object == definition->Base.Self; }));
  }
  return count;
}
} // namespace

TEST_CASE("entities/underlay: every release's three underlays and their definitions match the DXF twin")
{
  for (const std::string_view year : {"14", "2000", "2004", "2007", "2010", "2013", "2018"})
  {
    CAPTURE(year);
    const std::vector<Pair> twin = ReadTwin(std::format("kinds-r{}.dxf", year == "14" ? "2000" : year));
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(std::format("kinds-r{}.dwg", year)));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    const Drawing &drawing = loaded->Drawing;
    CHECK(CheckKind<UnderlayKind::Pdf>(drawing, twin, "ACAD_PDFDEFINITIONS") == 1);
    CHECK(CheckKind<UnderlayKind::Dwf>(drawing, twin, "ACAD_DWFDEFINITIONS") == 1);
    CHECK(CheckKind<UnderlayKind::Dgn>(drawing, twin, "ACAD_DGNDEFINITIONS") == 1);
  }
}

TEST_CASE("entities/underlay: a transformed and clipped underlay of each kind round-trips")
{
  const std::vector<Class> classes{Class{.Number = 500, .DxfName = "PDFUNDERLAY", .ItemClass = 0x1F2},
                                   Class{.Number = 501, .DxfName = "DWFUNDERLAY", .ItemClass = 0x1F2},
                                   Class{.Number = 502, .DxfName = "DGNUNDERLAY", .ItemClass = 0x1F2}};
  UnderlayData overlaid;
  overlaid.Definition = Handle{0x50};
  overlaid.Position = Vector{.X = 300, .Y = 10, .Z = 5};
  overlaid.Scale = Vector{.X = 2, .Y = 3, .Z = 4};
  overlaid.Rotation = std::numbers::pi / 6;
  overlaid.Flags = 3;
  overlaid.Contrast = 70;
  overlaid.Fade = 20;
  overlaid.Clip = {Bound{.Position = {.X = 1, .Y = 9, .Z = 0}}, Bound{.Position = {.X = 7, .Y = 2, .Z = 0}}};
  for (const Version release : FixtureReleases)
  {
    CAPTURE(NameOf(release));
    std::vector<Issue> issues;
    const Fields::Context with{.Release = release, .Text = &Objects::Latin, .Issues = &issues, .Classes = &classes};
    const Objects::FileContext reading{
        .Release = release, .Text = &Objects::Latin, .Classes = classes, .Issues = &issues};
    const auto again = [&with, &reading](const Object &object) -> Object
    {
      const std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(object, with), 0, reading);
      REQUIRE(back);
      return *back;
    };
    const Common common{.Space = Space::Model, .Layer = Handle{0x10}};
    CHECK(Get<PdfUnderlay>(again(PdfUnderlay{.Base = {.Self = Handle{0x100}}, .Common = common, .Data = overlaid}))
              .Data == overlaid);
    CHECK(Get<DwfUnderlay>(again(DwfUnderlay{.Base = {.Self = Handle{0x101}}, .Common = common, .Data = overlaid}))
              .Data == overlaid);
    CHECK(Get<DgnUnderlay>(again(DgnUnderlay{.Base = {.Self = Handle{0x102}}, .Common = common, .Data = overlaid}))
              .Data == overlaid);
    CHECK(issues.empty());
  }
}

TEST_CASE("entities/underlay: a negative or oversized clip count or a missing clip stops the read")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin, .Issues = &issues};
  const auto read = [&with](const Bits::Writer &data) -> bool
  {
    Bits::Reader in{data.Bytes()};
    Fields::Input reading{.In = &in, .With = with};
    UnderlayData overlaid;
    return Fields::Read(Fields::Layout(Fields::Custom(Shapes::ReadClip, Shapes::WriteClip)), overlaid, reading);
  };
  Bits::Writer big;
  big.BL(1000);
  CHECK_FALSE(read(big));
  Bits::Writer negative;
  negative.BL(-1);
  CHECK_FALSE(read(negative));
  CHECK_FALSE(read(Bits::Writer{}));
  REQUIRE(issues.size() == 2);
  CHECK(issues[0].What == "underlay clip count 1000 out of range");
  CHECK(issues[1].What == "underlay clip count -1 out of range");
}

TEST_CASE("entities/underlay: a definition dropped from a save is cleared from its underlay and noted")
{
  const Drawing drawing = Load(ReadFixture("kinds-r2018.dwg")).value().Drawing;
  for (const Object &object : drawing.Objects)
  {
    if (const auto *underlay = GetIf<PdfUnderlay>(&object))
    {
      Object detached = object;
      std::vector<Issue> issues;
      const std::vector<Handle> leftOut{underlay->Data.Definition};
      DwgIO::Releases::ClearPointers(detached, leftOut, issues);
      CHECK(Get<PdfUnderlay>(detached).Data.Definition == Handle{});
      REQUIRE(issues.size() == 1);
      CHECK(issues.front().What == std::format("underlay definition of PDFUNDERLAY {} cleared, {} left out",
                                               HexOf(underlay->Base.Self), HexOf(underlay->Data.Definition)));
    }
  }
}

TEST_CASE("entities/underlay: every kinds fixture saves to its own release byte for byte")
{
  for (const std::string_view year : {"14", "2000", "2004", "2007", "2010", "2013", "2018"})
  {
    CheckSameSave(std::format("kinds-r{}.dwg", year));
  }
}

TEST_CASE("entities/underlay: the kinds fixtures saved to other releases keep their underlays and definitions")
{
  struct Crossing
  {
    std::string_view From;
    Version To;
  };

  for (const Crossing crossing : {Crossing{.From = "kinds-r2018.dwg", .To = Version::R2000},
                                  Crossing{.From = "kinds-r2000.dwg", .To = Version::R2018},
                                  Crossing{.From = "kinds-r2007.dwg", .To = Version::R2013},
                                  Crossing{.From = "kinds-r2004.dwg", .To = Version::R14},
                                  Crossing{.From = "kinds-r14.dwg", .To = Version::R2010},
                                  Crossing{.From = "kinds-r2013.dwg", .To = Version::R2007}})
  {
    const Drawing back = CheckSaveAs(crossing.From, crossing.To);
    std::size_t underlays = 0;
    std::size_t definitions = 0;
    for (const Object &object : back.Objects)
    {
      underlays += Holds<PdfUnderlay>(object) || Holds<DwfUnderlay>(object) || Holds<DgnUnderlay>(object) ? 1U : 0U;
      definitions += Holds<Objects::PdfDefinition>(object) || Holds<Objects::DwfDefinition>(object) ||
                             Holds<Objects::DgnDefinition>(object)
                         ? 1U
                         : 0U;
    }
    CHECK(underlays == 3);
    CHECK(definitions == 3);
  }
}

TEST_CASE("entities/underlay: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO::Entities
