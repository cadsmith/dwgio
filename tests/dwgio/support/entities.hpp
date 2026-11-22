/***********************************************************************************************************************
 *  @file      tests/dwgio/support/entities.hpp
 *  @brief     What every entity test checks against the DXF twin: records, owners, lettering and common data.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TESTS_SUPPORT_ENTITIES_HPP
#define DWGIO_TESTS_SUPPORT_ENTITIES_HPP

#include "support/twin.hpp"
#include "support/vector.hpp"

#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/entities/lettering.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/vector.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace DwgIO::Entities
{
/**
 *  A lineweight's width in hundredths of a millimeter, as DXF's group 370 gives it.
 */
inline auto HundredthsOf(Lineweight lineweight) -> long
{
  const std::optional<double> millimeters = MillimetersOf(lineweight);
  REQUIRE(millimeters);
  return std::lround(*millimeters * 100);
}

/**
 *  The ENTITIES or BLOCKS record of the twin with that handle.
 */
inline auto Entity(const std::vector<Pair> &twin, Handle self) -> std::optional<std::size_t>
{
  bool inside = false;
  for (std::size_t at = 1; at < twin.size(); ++at)
  {
    if (twin[at].Code == 2 && (twin[at].Value == "ENTITIES" || twin[at].Value == "BLOCKS") &&
        twin[at - 1].Value == "SECTION")
    {
      inside = true;
    }
    else if (twin[at].Code == 0 && twin[at].Value == "ENDSEC")
    {
      inside = false;
    }
    else if (inside && twin[at].Code == 0 && Self(twin, at) == self)
    {
      return at;
    }
  }
  return std::nullopt;
}

/**
 *  The extrusion, the Z axis when the twin leaves it out.
 */
inline auto Normal(const std::vector<Pair> &twin, std::size_t opened) -> Vector
{
  return GroupOf(twin, opened, 210).empty() ? Up : ReadPoint(twin, opened, 210);
}

/**
 *  A direction group, the X axis when the twin leaves it out.
 */
inline auto Along(const std::vector<Pair> &twin, std::size_t opened, int code) -> Vector
{
  return GroupOf(twin, opened, code).empty() ? Right : ReadPoint(twin, opened, code);
}

/**
 *  The owner's 330 group: the last one before the first subclass marker, since a 102 brace of reactors holds 330 groups
 *  before it and a hatch's boundary sources hold 330 groups after it.
 */
inline auto Owner(const std::vector<Pair> &twin, std::size_t opened) -> std::string
{
  std::string owner;
  for (std::size_t at = opened + 1; at < twin.size() && twin[at].Code != 0 && twin[at].Code != 100; ++at)
  {
    if (twin[at].Code == 330)
    {
      owner = twin[at].Value;
    }
  }
  return owner;
}

/**
 *  Checks a text's lettering against the twin's groups, whose angles are in degrees; where the twin keeps no alignment
 *  point, the alignment is the position.
 */
inline void CheckLettering(const std::vector<Pair> &twin, std::size_t at, const Lettering &lettering)
{
  CHECK(Near(lettering.Position, ReadPoint(twin, at, 10)));
  if (!GroupOf(twin, at, 11).empty())
  {
    CHECK(Near(lettering.Alignment, ReadPoint(twin, at, 11)));
  }
  else
  {
    CHECK(Near(lettering.Alignment, lettering.Position));
  }
  CHECK(lettering.Height == doctest::Approx(ReadReal(twin, at, 40)));
  CHECK(lettering.Value == GroupOf(twin, at, 1));
  CHECK(lettering.Rotation == doctest::Approx(ReadReal(twin, at, 50) * std::numbers::pi / 180.0));
  CHECK(lettering.Width == doctest::Approx(GroupOf(twin, at, 41).empty() ? 1.0 : ReadReal(twin, at, 41)));
  CHECK(lettering.Oblique == doctest::Approx(ReadReal(twin, at, 51) * std::numbers::pi / 180.0));
  CHECK(lettering.Thickness == doctest::Approx(ReadReal(twin, at, 39)));
  CHECK(Near(lettering.Extrusion, Normal(twin, at)));
  const std::string style = GroupOf(twin, at, 7);
  const std::optional<std::size_t> named = FindOpeningNoCase(twin, "STYLE", style.empty() ? "Standard" : style);
  REQUIRE(named);
  CHECK(lettering.Style == Self(twin, *named));
  CHECK(lettering.Generation == ReadWhole(twin, at, 71));
  CHECK(std::to_underlying(lettering.Horizontal) == ReadWhole(twin, at, 72));
  CHECK(std::to_underlying(lettering.Vertical) == ReadWhole(twin, at, 73));
}

/**
 *  An entity's record in the twin, with the drawing it came from.
 */
struct TwinPlace
{
  const std::vector<Pair> *Twin{};
  std::size_t At{};
  const DwgIO::Drawing *Drawing{};
};

/**
 *  Checks the common data against the twin's groups: the layer and linetype through their tables, the color, scale,
 *  invisibility, lineweight, space and owner.
 */
inline void CheckCommon(const TwinPlace &place, const Common &common, const Base &base)
{
  const std::vector<Pair> &twin = *place.Twin;
  const std::size_t at = place.At;
  const std::optional<std::size_t> layer = FindOpeningNoCase(twin, "LAYER", GroupOf(twin, at, 8));
  REQUIRE(layer);
  CHECK(common.Layer == Self(twin, *layer));
  // An R14 file names ByBlock and Continuous by their records; R2000 says them in the flags.
  const bool modern = place.Drawing->Release >= Version::R2000;
  const std::string linetype = Upper(GroupOf(twin, at, 6));
  if (linetype.empty() || linetype == "BYLAYER")
  {
    CHECK(common.LinetypeOrigin == Origin::ByLayer);
    CHECK(common.Linetype == Handle{});
  }
  else if (modern && linetype == "BYBLOCK")
  {
    CHECK(common.LinetypeOrigin == Origin::ByBlock);
  }
  else if (modern && linetype == "CONTINUOUS")
  {
    CHECK(common.LinetypeOrigin == Origin::Continuous);
  }
  else
  {
    const std::optional<std::size_t> named = FindOpeningNoCase(twin, "LTYPE", linetype);
    REQUIRE(named);
    CHECK(common.LinetypeOrigin == Origin::Record);
    CHECK(common.Linetype == Self(twin, *named));
  }
  const std::string color = GroupOf(twin, at, 62);
  CHECK(common.Color.Index == (color.empty() ? 256 : std::stoi(color)));
  const std::string scale = GroupOf(twin, at, 48);
  CHECK(common.LinetypeScale == (scale.empty() ? 1.0 : std::stod(scale)));
  CHECK(common.Invisible == (ReadWhole(twin, at, 60) == 1));
  const std::string weight = GroupOf(twin, at, 370);
  if (weight.empty())
  {
    CHECK(common.Lineweight == (modern ? std::optional{Lineweight::ByLayer} : std::nullopt));
  }
  else if (modern)
  {
    REQUIRE(common.Lineweight);
    CHECK(HundredthsOf(*common.Lineweight) == std::stoi(weight));
  }
  // DXF flags every entity of a paper space layout with 67; the file names only the first layout's space by mode.
  const bool paper = ReadWhole(twin, at, 67) == 1;
  if (common.Space == Space::Paper)
  {
    CHECK(paper);
  }
  else if (common.Space == Space::Model)
  {
    CHECK_FALSE(paper);
  }
  // AutoCAD gives a SEQEND a plot style of its own that is null.
  CHECK((common.PlotstyleOrigin == Origin::ByLayer ||
         (common.PlotstyleOrigin == Origin::Record && common.Plotstyle == Handle{})));
  CHECK(base.Owner == ParseHandle(Owner(twin, at)));
}
} // namespace DwgIO::Entities

#endif /* DWGIO_TESTS_SUPPORT_ENTITIES_HPP */
