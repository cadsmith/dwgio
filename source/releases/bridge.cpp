/***********************************************************************************************************************
 *  @file      source/releases/bridge.cpp
 *  @brief     The links of a chain carried past the entities a save leaves out.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/bridge.hpp"

#include "releases/lookup.hpp"
#include "releases/object.hpp"

#include "dwgio/common.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/version.hpp"

#include <algorithm>
#include <cstddef>

namespace DwgIO::Releases
{
namespace
{
struct BridgeContext
{
  const DwgIO::Drawing *Drawing{};
  const Releases::Omission *Omission{};
};

/**
 *  As the release holds it; null for an entity the drawing lacks.
 */
auto NextLink(Handle self, bool forward, const BridgeContext &walking) -> Handle
{
  const Object *passed = walking.Omission->Lookup->Of(self);
  if (passed == nullptr)
  {
    return {};
  }
  const Translation translated = ConvertObject(*passed, *walking.Omission->Lookup, walking.Omission->To);
  const Common *common = CommonOf(translated.Object);
  if (common == nullptr)
  {
    return {};
  }
  return forward ? common->Next : common->Previous;
}

/**
 *  The nearest entity a save writes from a link, following the links of those it leaves out, as the release holds them;
 *  null at the chain's end or where it breaks.
 */
auto NearestWritten(Handle link, bool forward, const BridgeContext &walking) -> Handle
{
  for (std::size_t steps = 0; link && steps <= walking.Drawing->Objects.size(); ++steps)
  {
    if (!std::ranges::binary_search(walking.Omission->LeftOut, link.Value, {}, &Handle::Value))
    {
      return link;
    }
    link = NextLink(link, forward, walking);
  }
  return {};
}
} // namespace

void Bridge(DwgIO::Object &object, const Drawing &drawing, const Omission &leaving)
{
  if (leaving.To >= Version::R2004 || leaving.LeftOut.empty())
  {
    return;
  }
  const BridgeContext walking{.Drawing = &drawing, .Omission = &leaving};
  if (Common *common = CommonOf(object))
  {
    common->Previous = NearestWritten(common->Previous, false, walking);
    common->Next = NearestWritten(common->Next, true, walking);
  }
  Visit(
      [&walking](auto &kind) -> void
      {
        if constexpr (requires {
                        kind.First;
                        kind.Last;
                      })
        {
          kind.First = NearestWritten(kind.First, true, walking);
          kind.Last = NearestWritten(kind.Last, false, walking);
        }
      },
      object);
}
} // namespace DwgIO::Releases
