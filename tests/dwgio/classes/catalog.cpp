/***********************************************************************************************************************
 *  @file      tests/dwgio/classes/catalog.cpp
 *  @brief     The class records a save declares, stamped for R2004 as AutoCAD stamps them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "classes/catalog.hpp"

#include "objects/kinds.hpp"

#include "dwgio/class.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/symbol.hpp"
#include "dwgio/unknown.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

namespace DwgIO::Classes
{
TEST_CASE("classes/catalog: an unknown object of a class AutoCAD's table lacks gets no declaration")
{
  Drawing drawing = Create();
  Unknown unknown;
  unknown.Type = 500;
  unknown.Class = Symbol{"ACME_WIDGET"};
  unknown.Base.Self = Edit::MakeHandle(drawing);
  unknown.Base.Owner = drawing.Header.DictionaryNamedObjects;
  drawing.Objects.emplace_back(unknown);
  CHECK(CollectNeeded(drawing, Version::R2000).empty());
  CHECK(std::ranges::none_of(CollectNeeded(drawing, Version::R14),
                             [](const Class &one) -> bool { return one.DxfName == "ACME_WIDGET"; }));
}

TEST_CASE("classes/catalog: every kind stored under a class is in AutoCAD's table")
{
  const auto known = []<typename KindT>(std::type_identity<KindT>) -> void
  {
    for (const std::string_view name : Objects::ClassNames<KindT>())
    {
      CAPTURE(name);
      CHECK(FindDeclared(name));
    }
  };
  []<typename... KindsT>(std::type_identity<std::tuple<KindsT...>>, const auto &each) -> void
  {
    (each(std::type_identity<KindsT>{}), ...);
  }(std::type_identity<Objects::Varying>{}, known);
}

TEST_CASE("classes/catalog: an R2004 class record carries the ObjectDBX version and its instance count")
{
  // AutoCAD refuses an R2004 file whose class records carry no version: its own say version 22, maintenance 42,
  // and count the objects of the class, the layouts among them though R2004 stores those under a fixed type.
  const Drawing drawing = Create(Version::R2004);
  std::vector<Class> classes = drawing.Classes;
  classes.push_back(Class{.Number = 503,
                          .ProxyFlags = 0,
                          .Application = "ObjectDBX Classes",
                          .CppName = "AcDbScale",
                          .DxfName = "SCALE",
                          .WasZombie = false,
                          .ItemClass = 499,
                          .Instances = 5,
                          .DwgVersion = 1,
                          .Maintenance = 2,
                          .Unknown1 = 0,
                          .Unknown2 = 0});
  FillCounts(classes, drawing);
  const auto named = [&classes](std::string_view dxf) -> const Class &
  {
    return *std::ranges::find(classes, dxf, &Class::DxfName);
  };
  CHECK(named("LAYOUT").Instances == 2);
  CHECK(named("LAYOUT").DwgVersion == 22);
  CHECK(named("LAYOUT").Maintenance == 42);
  CHECK(named("ACDBPLACEHOLDER").Instances == 1);
  CHECK(named("ACDBDICTIONARYWDFLT").Instances == 1);
  CHECK(named("SCALE").Instances == 5);
  CHECK(named("SCALE").DwgVersion == 1);
  CHECK(named("SCALE").Maintenance == 2);
  // A class in use with a maintenance but no version keeps both as they are.
  classes.push_back(Class{.Number = 504,
                          .ProxyFlags = 0,
                          .Application = "ObjectDBX Classes",
                          .CppName = "AcDbLayout",
                          .DxfName = "LAYOUT",
                          .WasZombie = false,
                          .ItemClass = 499,
                          .Instances = 0,
                          .DwgVersion = 0,
                          .Maintenance = 7,
                          .Unknown1 = 0,
                          .Unknown2 = 0});
  FillCounts(classes, drawing);
  CHECK(classes.back().Instances == 2);
  CHECK(classes.back().DwgVersion == 0);
  CHECK(classes.back().Maintenance == 7);
}
} // namespace DwgIO::Classes
