/***********************************************************************************************************************
 *  @file      tests/dwgio/releases/appid.cpp
 *  @brief     The applications a save below R2010 or R2013 registers for what it moves to extended data.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/appid.hpp"

#include "entities/shapes/geometry.hpp"
#include "objects/shapes/mleaderstyle.hpp"
#include "releases/acis.hpp"
#include "releases/guid.hpp"
#include "releases/roundtrip.hpp"
#include "support/acad.hpp"
#include "support/fixtures.hpp"
#include "support/pass.hpp"

#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/entities/modeler.hpp"
#include "dwgio/entities/multileader.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/mleaderstyle.hpp"
#include "dwgio/save.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/tables/control.hpp"
#include "dwgio/tables/table.hpp"
#include "dwgio/version.hpp"
#include "dwgio/view.hpp"
#include "dwgio/xdata.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <expected>
#include <format>
#include <optional>
#include <variant>

namespace DwgIO::Releases
{
namespace
{
/**
 *  The mleaders fixture of R2018 as AutoCAD's samples are: no ACAD_MLEADERVER application, its record renamed.
 */
auto LoadWithoutApplication() -> Drawing
{
  Drawing drawing = Load(ReadFixture("mleaders-r2018.dwg")).value().Drawing;
  for (Object &object : drawing.Objects)
  {
    if (auto *appid = GetIf<Tables::AppId>(&object);
        appid != nullptr && appid->Record.Name == Objects::MLeaderStyles::Versioned)
    {
      appid->Record.Name = "DWGIO_RENAMED";
    }
  }
  return drawing;
}

/**
 *  Whether an object is a multileader or its style.
 */
auto Leading(const Object &object) -> bool
{
  return Holds<Objects::MLeaderStyle>(object) || Holds<Entities::MultiLeader>(object);
}
} // namespace

TEST_CASE("releases/appid: a save below R2010 registers ACAD_MLEADERVER for the multileader version")
{
  const Drawing drawing = LoadWithoutApplication();
  REQUIRE(View::Record<Tables::AppId>(drawing, Objects::MLeaderStyles::Versioned) == nullptr);
  REQUIRE(std::ranges::count_if(drawing.Objects, Leading) > 1);
  for (const Version release : {Version::R14, Version::R2004, Version::R2007})
  {
    CAPTURE(NameOf(release));
    const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = release});
    REQUIRE(saved);
    CHECK_FALSE(std::ranges::any_of(saved->Issues, [](const Issue &issue) -> bool
                                    { return issue.What.starts_with("application ACAD_MLEADERVER missing"); }));
    const Drawing down = Load(saved->Bytes).value().Drawing;
    const auto *appid = View::Record<Tables::AppId>(down, Objects::MLeaderStyles::Versioned);
    REQUIRE(appid != nullptr);
    // Under the seed: a new handle, as AutoCAD gives it.
    CHECK(appid->Record.Base.Self == drawing.Header.Handseed);
    for (const Object &object : down.Objects)
    {
      if (Leading(object))
      {
        CAPTURE(HexOf(BaseOf(object).Self));
        CHECK(std::ranges::any_of(
            BaseOf(object).Extended, [appid](const Xdata &xdata) -> bool
            { return xdata.AppId == appid->Record.Base.Self && Objects::MLeaderStyles::IsVersion(xdata); }));
      }
    }
    QueueAudit(saved->Bytes, std::format("dwgio-mleaders-unregistered-as-{}.dwg", NameOf(release)));
  }
}

TEST_CASE("releases/appid: nothing is registered when no version moves or no table can list it")
{
  const Drawing drawing = LoadWithoutApplication();
  CHECK(Pass(RegisterApplications, drawing, Version::R2007).has_value());
  CHECK_FALSE(Pass(RegisterApplications, drawing, Version::R2010).has_value());
  CHECK_FALSE(
      Pass(RegisterApplications, Load(ReadFixture("mleaders-r2018.dwg")).value().Drawing, Version::R2007).has_value());
  CHECK_FALSE(
      Pass(RegisterApplications, Load(ReadFixture("mleaders-r2007.dwg")).value().Drawing, Version::R2000).has_value());
  Drawing plain = drawing;
  std::erase_if(plain.Objects, Leading);
  CHECK_FALSE(Pass(RegisterApplications, plain, Version::R2007).has_value());
  // A multileader alone needs it too.
  Drawing leaders = drawing;
  std::erase_if(leaders.Objects, [](const Object &object) -> bool { return Holds<Objects::MLeaderStyle>(object); });
  CHECK(Pass(RegisterApplications, leaders, Version::R2004).has_value());
  Drawing tableless = Create(Version::R2018);
  std::erase_if(tableless.Objects,
                [](const Object &object) -> bool
                {
                  const auto *control = GetIf<Tables::Control>(&object);
                  return Holds<Tables::AppId>(object) || (control != nullptr && control->Table == Tables::Table::AppId);
                });
  tableless.Objects.emplace_back(Objects::MLeaderStyle{});
  CHECK_FALSE(Pass(RegisterApplications, tableless, Version::R2007).has_value());
}

TEST_CASE("releases/appid: a save below R2013 registers ACAD_OBJECT_CHANGE_GUID for the ACIS GUIDs")
{
  const Drawing drawing = Load(ReadFixture("kinds-r2013.dwg")).value().Drawing;
  REQUIRE(View::Record<Tables::AppId>(drawing, GuidApplication) == nullptr);
  const std::optional<Drawing> registered = Pass(RegisterApplications, drawing, Version::R2010);
  REQUIRE(registered);
  CHECK(View::Record<Tables::AppId>(*registered, GuidApplication) != nullptr);
  CHECK_FALSE(Pass(RegisterApplications, *registered, Version::R2010));
  CHECK_FALSE(Pass(RegisterApplications, drawing, Version::R2018));
  CHECK_FALSE(Pass(RegisterApplications, Load(ReadFixture("kinds-r2010.dwg")).value().Drawing, Version::R2004));
  // Saved below R2010 too, the multileaders' version needs its application as well.
  const std::optional<Drawing> both = Pass(RegisterApplications, drawing, Version::R2007);
  REQUIRE(both);
  CHECK(View::Record<Tables::AppId>(*both, GuidApplication) != nullptr);
  CHECK(View::Record<Tables::AppId>(*both, Objects::MLeaderStyles::Versioned) != nullptr);
  // No GUID to keep, nothing to register.
  Drawing unguided = drawing;
  for (Object &object : unguided.Objects)
  {
    if (Entities::Geometry *geometry = GeometryOf(object))
    {
      geometry->Guid = std::string{Entities::Shapes::NoGuid};
    }
  }
  CHECK_FALSE(Pass(RegisterApplications, unguided, Version::R2010));
}

TEST_CASE("releases/appid: a NURBS surface's record is read only on upgrading a SURFACE to R2013")
{
  Drawing drawing = Create(Version::R2013);
  drawing.Objects.emplace_back(Entities::Surface{});
  CHECK_FALSE(Pass(KeepRoundTrips, drawing, Version::R2018));
}

TEST_CASE("releases/appid: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO::Releases
