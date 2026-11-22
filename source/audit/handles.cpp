/***********************************************************************************************************************
 *  @file      source/audit/handles.cpp
 *  @brief     The audit of handles: missing, repeated, or not below the seed.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "audit/checks.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/base.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"

#include <algorithm>
#include <cstdint>
#include <format>
#include <unordered_set>

namespace DwgIO::Audit
{
void Handles(Drawing &drawing, Report &report)
{
  Handle highest;
  for (const Object &object : drawing.Objects)
  {
    highest = std::max(highest, BaseOf(object).Self);
  }
  Handle &seed = drawing.Header.Handseed;
  if (seed <= highest)
  {
    NoteFixed(report, Finding{.Level = Severity::Error,
                              .What = std::format("handle seed {} not past {}", HexOf(seed), HexOf(highest)),
                              .Subject = {},
                              .Fixed = false});
  }
  std::unordered_set<std::uint64_t> seen;
  for (Object &object : drawing.Objects)
  {
    Base &base = BaseOf(object);
    if (!base.Self)
    {
      NoteFixed(report, Finding{.Level = Severity::Error,
                                .What = std::format("{} has no handle", TypeOf(object)),
                                .Subject = {},
                                .Fixed = false});
      base.Self = Handle{.Value = ++highest.Value};
    }
    else if (!seen.insert(base.Self.Value).second)
    {
      NoteFixed(report, Finding{.Level = Severity::Error,
                                .What = std::format("handle {} of a {} used twice", HexOf(base.Self), TypeOf(object)),
                                .Subject = base.Self,
                                .Fixed = false});
      base.Self = Handle{.Value = ++highest.Value};
    }
  }
  seed = std::max(seed, Handle{.Value = highest.Value + 1});
}
} // namespace DwgIO::Audit
