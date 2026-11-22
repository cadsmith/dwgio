/***********************************************************************************************************************
 *  @file      tests/dwgio/support/registered.hpp
 *  @brief     What AutoCAD remakes at each save, set aside before an entity is compared across releases.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TESTS_SUPPORT_REGISTERED_HPP
#define DWGIO_TESTS_SUPPORT_REGISTERED_HPP

#include "objects/shape.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/xdata.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <variant>
#include <vector>

namespace DwgIO::Entities
{
inline auto ApplicationOf(const Drawing &drawing, Handle appid) -> std::string
{
  for (const Object &object : drawing.Objects)
  {
    if (const auto *application = GetIf<Tables::AppId>(&object);
        application != nullptr && application->Record.Base.Self == appid)
    {
      return application->Record.Name;
    }
  }
  return {};
}

/**
 *  Gives the extended data of the object read back the application handles of its R2000 twin where the names agree,
 *  since AutoCAD registers an application again, under a handle of its own, in each save.
 */
inline void Reregister(Object &back, const Drawing &older, const Object &same, const Drawing &newer)
{
  const std::vector<Xdata> &modern = BaseOf(same).Extended;
  Visit(
      [&](auto &kind) -> void
      {
        std::vector<Xdata> &extended = Objects::BaseOfKind(kind).Extended;
        for (std::size_t at = 0; at < extended.size() && at < modern.size(); ++at)
        {
          if (ApplicationOf(older, extended[at].AppId) == ApplicationOf(newer, modern[at].AppId))
          {
            extended[at].AppId = modern[at].AppId;
          }
        }
      },
      back);
}

/**
 *  The block records of the dimensions' anonymous blocks, which AutoCAD redraws for each save.
 */
inline auto CollectDimensionBlocks(const Drawing &drawing) -> std::vector<Handle>
{
  std::vector<Handle> blocks;
  for (const Object &object : drawing.Objects)
  {
    if (const auto *record = GetIf<Tables::BlockRecord>(&object);
        record != nullptr && record->Record.Name.starts_with("*D"))
    {
      blocks.push_back(record->Record.Base.Self);
    }
  }
  return blocks;
}
} // namespace DwgIO::Entities

#endif /* DWGIO_TESTS_SUPPORT_REGISTERED_HPP */
