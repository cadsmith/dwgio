/***********************************************************************************************************************
 *  @file      source/releases/appid.cpp
 *  @brief     The applications a save to a release registers for the extended data it writes.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/appid.hpp"

#include "objects/shapes/mleaderstyle.hpp"
#include "releases/acis.hpp"
#include "releases/guid.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/multileader.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/mleaderstyle.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/version.hpp"
#include "dwgio/view.hpp"

#include <algorithm>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
/**
 *  An ACIS entity whose GUID moves into extended data.
 */
auto HasGuidData(const Object &object) -> bool
{
  const Entities::Geometry *geometry = GeometryOf(object);
  return geometry != nullptr && HasGuid(*geometry);
}

/**
 *  A multileader or its style, whose version moves into extended data.
 */
auto HasVersionData(const Object &object) -> bool
{
  return Holds<Objects::MLeaderStyle>(object) || Holds<Entities::MultiLeader>(object);
}
} // namespace

void RegisterApplications(Remake &remade, Version to)
{
  const Drawing &drawing = remade.Now();
  std::vector<std::string_view> needed;
  if (drawing.Release >= Version::R2010 && to < Version::R2010 && std::ranges::any_of(drawing.Objects, HasVersionData))
  {
    needed.push_back(Objects::MLeaderStyles::Versioned);
  }
  if (drawing.Release >= Version::R2013 && to < Version::R2013 && std::ranges::any_of(drawing.Objects, HasGuidData))
  {
    needed.push_back(GuidApplication);
  }
  std::erase_if(needed, [&drawing](std::string_view name) -> bool
                { return View::Record<Tables::AppId>(drawing, name) != nullptr; });
  if (needed.empty())
  {
    return;
  }
  if (View::Control(drawing, Tables::Table::AppId) == nullptr)
  {
    return; // No application table to list it in; what needs it is noted as it is converted.
  }
  Drawing &registered = remade.Change();
  for (const std::string_view name : needed)
  {
    Tables::AppId appid;
    appid.Record.Name = std::string{name};
    std::ignore = Edit::Add(registered, appid); // The table is there, and holds no record of the name.
  }
}

} // namespace DwgIO::Releases
