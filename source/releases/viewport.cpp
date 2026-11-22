/***********************************************************************************************************************
 *  @file      source/releases/viewport.cpp
 *  @brief     A paper space viewport's view as R14 keeps it in extended data.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/viewport.hpp"

#include "releases/lookup.hpp"
#include "releases/passage.hpp"

#include "dwgio/base.hpp"
#include "dwgio/entities/viewport.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/vector.hpp"
#include "dwgio/version.hpp"
#include "dwgio/xdata.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <string>
#include <variant>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
/**
 *  R14 keeps a paper space viewport's view in ACAD's "MVIEW" extended data where R2000 has fields: a brace, the version
 *  16, the view's groups, the frozen layers by handle in a brace of their own, and a closing brace.
 */
namespace Mview
{
constexpr int String = 1000;
constexpr int Brace = 1002;
constexpr int Layer = 1003;
constexpr int Point = 1010;
constexpr int Real = 1040;
constexpr int Short = 1070;
constexpr std::int64_t Version = 16;
constexpr const char *Name = "MVIEW";
constexpr unsigned Mode = 0x1F;
constexpr unsigned IconShift = 5;
constexpr unsigned Icon = 3;
constexpr unsigned Snap = 256;
constexpr unsigned Grid = 512;
constexpr unsigned Isometric = 1024;
constexpr unsigned Hidden = 2048;
/**
 *  The isopair bits: top, right, neither for the left; 16384 above them locks the zoom, which R14 cannot say.
 */
constexpr unsigned Top = 4096;
constexpr unsigned Right = 8192;
/**
 *  The grid shown beyond the limits and adaptive, which R14 cannot say and AutoCAD's R2000 viewports all have.
 */
constexpr unsigned Beyond = 262144;
constexpr unsigned Adaptive = 524288;

auto IsMview(const Xdata &xdata) -> bool
{
  const std::string *name = xdata.Tags.empty() ? nullptr : std::get_if<std::string>(&xdata.Tags.front().Value);
  return name != nullptr && *name == Name;
}

/**
 *  The groups of a viewport's view, in R14's order.
 */
auto MakeGroups(const Entities::Viewport &viewport) -> std::vector<Tag>
{
  const auto flags = static_cast<unsigned>(viewport.Flags);
  const auto bit = [flags](unsigned mask) -> std::int64_t
  {
    return (flags & mask) != 0 ? 1 : 0;
  };
  std::int64_t isopair = 0;
  if ((flags & Top) != 0)
  {
    isopair = 1;
  }
  if ((flags & Right) != 0)
  {
    isopair = 2;
  }
  std::vector<Tag> groups{Tag{.Code = String, .Value = std::string{Name}}, Tag{.Code = Brace, .Value = "{"},
                          Tag{.Code = Short, .Value = Version}};
  for (const Vector &point : {viewport.Target, viewport.Direction})
  {
    groups.push_back(Tag{.Code = Point, .Value = point.X});
    groups.push_back(Tag{.Code = Point + 10, .Value = point.Y});
    groups.push_back(Tag{.Code = Point + 20, .Value = point.Z});
  }
  for (const double real : {viewport.Twist, viewport.ViewHeight, viewport.ViewCenter.X, viewport.ViewCenter.Y,
                            viewport.LensLength, viewport.Front, viewport.Back})
  {
    groups.push_back(Tag{.Code = Real, .Value = real});
  }
  // Fast zoom is always on in AutoCAD's R14 files, whatever the R2000 flag says.
  for (const std::int64_t value :
       {static_cast<std::int64_t>(flags & Mode), std::int64_t{viewport.Zoom}, std::int64_t{1},
        static_cast<std::int64_t>((flags >> IconShift) & Icon), bit(Snap), bit(Grid), bit(Isometric), isopair})
  {
    groups.push_back(Tag{.Code = Short, .Value = value});
  }
  for (const double real : {viewport.SnapAngle, viewport.SnapBase.X, viewport.SnapBase.Y, viewport.Snap.X,
                            viewport.Snap.Y, viewport.Grid.X, viewport.Grid.Y})
  {
    groups.push_back(Tag{.Code = Real, .Value = real});
  }
  groups.push_back(Tag{.Code = Short, .Value = bit(Hidden)});
  groups.push_back(Tag{.Code = Brace, .Value = "{"});
  for (const Handle layer : viewport.Frozen)
  {
    groups.push_back(Tag{.Code = Layer, .Value = layer});
  }
  groups.push_back(Tag{.Code = Brace, .Value = "}"});
  groups.push_back(Tag{.Code = Brace, .Value = "}"});
  return groups;
}

/**
 *  The view R14 keeps in a viewport's extended data, set into R2000's fields; false when the groups are not of that
 *  shape, which leaves the viewport as it was. R14 has no place for the grid's later bits.
 */
auto Read(const std::vector<Tag> &groups, Entities::Viewport &viewport) -> bool
{
  constexpr std::size_t Least = 35;
  constexpr std::size_t First = 3;
  constexpr std::size_t Last = 32;
  constexpr std::size_t Reals = 20;
  constexpr std::size_t Shorts = 9;
  if (groups.size() < Least)
  {
    return false;
  }
  std::vector<double> reals;
  std::vector<std::int64_t> shorts;
  for (std::size_t at = First; at < Last; ++at)
  {
    if (const auto *real = std::get_if<double>(&groups[at].Value))
    {
      reals.push_back(*real);
    }
    else if (const auto *value = std::get_if<std::int64_t>(&groups[at].Value))
    {
      shorts.push_back(*value);
    }
  }
  if (reals.size() != Reals || shorts.size() != Shorts)
  {
    return false;
  }
  std::size_t at = 0;
  const auto next = [&reals, &at] -> double
  {
    return reals.at(at++);
  };
  viewport.Target = Vector{.X = next(), .Y = next(), .Z = next()};
  viewport.Direction = Vector{.X = next(), .Y = next(), .Z = next()};
  viewport.Twist = next();
  viewport.ViewHeight = next();
  viewport.ViewCenter.X = next();
  viewport.ViewCenter.Y = next();
  viewport.LensLength = next();
  viewport.Front = next();
  viewport.Back = next();
  viewport.SnapAngle = next();
  viewport.SnapBase.X = next();
  viewport.SnapBase.Y = next();
  viewport.Snap.X = next();
  viewport.Snap.Y = next();
  viewport.Grid.X = next();
  viewport.Grid.Y = next();
  // The shorts: view mode, circle zoom, fast zoom, UCS icon, snap, grid, snap style, isopair, hidden lines.
  const auto when = [&shorts](std::size_t index, unsigned mask) -> unsigned
  {
    return shorts.at(index) != 0 ? mask : 0U;
  };
  constexpr std::int64_t TopPair = 1;
  constexpr std::int64_t RightPair = 2;
  unsigned flags = static_cast<unsigned>(32768) | Beyond | Adaptive | (static_cast<unsigned>(shorts[0]) & Mode) |
                   ((static_cast<unsigned>(shorts[3]) & Icon) << IconShift) | when(4, Snap) | when(5, Grid) |
                   when(6, Isometric) | when(8, Hidden);
  flags |= shorts[7] == TopPair ? Top : 0U;
  flags |= shorts[7] == RightPair ? Right : 0U;
  viewport.Flags = static_cast<int>(flags);
  viewport.Zoom = static_cast<int>(shorts[1]);
  viewport.Frozen.clear();
  for (const Tag &group : groups)
  {
    if (const auto *layer = std::get_if<Handle>(&group.Value); group.Code == Layer && layer != nullptr)
    {
      viewport.Frozen.push_back(*layer);
    }
  }
  return true;
}
} // namespace Mview
} // namespace

void ConvertView(Entities::Viewport &viewport, const Passage &passage, const Crossings &crossing)
{
  std::vector<Xdata> &extended = viewport.Base.Extended;
  if (crossing.R2000Up)
  {
    const auto mview = std::ranges::find_if(extended, Mview::IsMview);
    if (mview != extended.end() && Mview::Read(mview->Tags, viewport))
    {
      extended.erase(mview);
    }
  }
  if (!crossing.R2000Down)
  {
    return;
  }
  std::erase_if(extended, Mview::IsMview);
  if (viewport.Clip != Handle{})
  {
    Note(passage, viewport.Base.Self, std::format("clip of {} not kept by R14", passage.What));
  }
  const auto *acad = passage.Lookup->Record<Tables::AppId>("ACAD");
  if (acad == nullptr)
  {
    Note(passage, viewport.Base.Self, std::format("application ACAD missing for the view of {}", passage.What));
    return;
  }
  extended.push_back(Xdata{.Application = {}, .AppId = acad->Record.Base.Self, .Tags = Mview::MakeGroups(viewport)});
}
} // namespace DwgIO::Releases
