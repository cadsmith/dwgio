/***********************************************************************************************************************
 *  @file      source/releases/binding.cpp
 *  @brief     What binds an object to its drawing's release, so a save to another leaves it out.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/binding.hpp"

#include "encoding/codepage.hpp"
#include "encoding/encoding.hpp"
#include "releases/untyped.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/entities/mesh.hpp"
#include "dwgio/object.hpp"
#include "dwgio/unknown.hpp"
#include "dwgio/version.hpp"

#include <string_view>
#include <variant>

namespace DwgIO::Releases
{
auto BindingOf(const Object &object, const Drawing &drawing, Version to) -> std::string_view
{
  const Version from = drawing.Release;
  if (from == to)
  {
    return {};
  }
  if (const auto *unknown = GetIf<Unknown>(&object))
  {
    // In the drawing's code page, as the save recasts them: a text it writes longer than a count holds, its
    // characters escaped, does not cross where another page's would.
    const Encoding::Scheme scheme = Encoding::SchemeOf(drawing.Codepage);
    return Carry(*unknown, CarryContext{.From = from, .To = to, .Text = &scheme}) ? std::string_view{} : "bytes";
  }
  if (const auto *mesh = GetIf<Entities::Mesh>(&object); mesh != nullptr && mesh->Overridden != 0)
  {
    return "overrides";
  }
  return {};
}
} // namespace DwgIO::Releases
