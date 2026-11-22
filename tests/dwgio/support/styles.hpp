/***********************************************************************************************************************
 *  @file      tests/dwgio/support/styles.hpp
 *  @brief     Visual and view styles of a drawing dwgio saved, against AutoCAD's save of them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TESTS_SUPPORT_STYLES_HPP
#define DWGIO_TESTS_SUPPORT_STYLES_HPP

#include "encoding/codepage.hpp"
#include "encoding/encoding.hpp"
#include "releases/palette.hpp"
#include "releases/untyped.hpp"

#include "dwgio/base.hpp"
#include "dwgio/color.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/visualstyle.hpp"
#include "dwgio/objects/xrecord.hpp"
#include "dwgio/opaque.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/unknown.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO
{
/**
 *  The groups of the record a style keeps under ACAD_XREC_ROUNDTRIP in its extension dictionary, or none.
 */
inline auto Roundtrip(const Drawing &drawing, const Base &base) -> std::optional<std::vector<Tag>>
{
  const auto *dictionary = GetIf<Objects::Dictionary>(Find(drawing, base.Dictionary));
  if (dictionary == nullptr)
  {
    return std::nullopt;
  }
  const auto entry = std::ranges::find(dictionary->Entries, "ACAD_XREC_ROUNDTRIP", &Objects::Entry::Name);
  const auto *record =
      entry == dictionary->Entries.end() ? nullptr : GetIf<Objects::XRecord>(Find(drawing, entry->Object));
  return record != nullptr ? std::optional{record->Data} : std::nullopt;
}

/**
 *  Whether an object is a visual or view style.
 */
inline auto Style(const Object &object) -> bool
{
  const std::string_view name = TypeOf(object);
  return name == "VISUALSTYLE" || name == "ACDBDETAILVIEWSTYLE" || name == "ACDBSECTIONVIEWSTYLE";
}

/**
 *  Whether a save of a drawing left none of its visual and view styles out.
 */
inline auto KeepsStyles(const Drawing &source, const std::vector<Issue> &issues) -> bool
{
  return std::ranges::none_of(issues,
                              [&source](const Issue &issue) -> bool
                              {
                                const Object *object = Find(source, issue.Subject);
                                return object != nullptr && Style(*object) && issue.What.contains("left out");
                              });
}

/**
 *  A visual style's properties of R2010 or later but for what the release saved from cannot hold: below R2004 a true
 *  color, which becomes its nearest index, and below R2010 a lighting quality of 3, which becomes 2, and a line width's
 *  fraction.
 */
inline auto LowerProperties(std::vector<Objects::VisualStyles::Property> properties, Version from)
    -> std::vector<Objects::VisualStyles::Property>
{
  constexpr std::size_t Quality = 1;
  constexpr std::size_t Width = 26;
  constexpr std::size_t Shared = 28;
  for (std::size_t at = 0; at < std::min(properties.size(), Shared); ++at)
  {
    auto &value = properties[at].Value;
    if (auto *color = std::get_if<Color>(&value); color != nullptr && from < Version::R2004)
    {
      *color = Releases::LowerColor(*color);
    }
    if (auto *quality = std::get_if<std::int32_t>(&value); quality != nullptr && at == Quality && from < Version::R2010)
    {
      *quality = std::min(*quality, 2);
    }
    if (auto *width = std::get_if<double>(&value); width != nullptr && at == Width && from < Version::R2010)
    {
      *width = std::trunc(*width);
    }
  }
  return properties;
}

/**
 *  A view style's own bits of a release carried down to the release saved from and back, as AutoCAD's save of the
 *  drawing it was saved from would have them: what the older release cannot hold lost on the way.
 */
inline auto Through(const Unknown &theirs, Version from, Version to) -> std::optional<Opaque>
{
  const Encoding::Scheme scheme = Encoding::MakeScheme(Encoding::Standard());
  std::vector<Content> taken;
  const std::optional<Opaque> down = Releases::Carry(
      theirs, Releases::CarryContext{.From = to, .To = from, .Text = &scheme, .Given = {}, .Taken = &taken});
  if (!down)
  {
    return std::nullopt;
  }
  Unknown lower = theirs;
  lower.Body = down;
  return Releases::Carry(
      lower, Releases::CarryContext{.From = from, .To = to, .Text = &scheme, .Given = taken, .Taken = nullptr});
}

/**
 *  Checks the visual and view styles of a drawing dwgio saved from another release against AutoCAD's save of them to
 *  that release; upgraded, but for what the release saved from cannot hold. A style AutoCAD made at its save is not
 *  compared.
 */
inline void CheckStyles(const Drawing &ours, const Drawing &theirs, const Drawing &source)
{
  const Version from = source.Release;
  const Version to = theirs.Release;
  const bool upgrading = from < to;
  for (const Object &object : theirs.Objects)
  {
    const Base &base = BaseOf(object);
    const Object *mine = Find(ours, base.Self);
    if (Find(source, base.Self) == nullptr || TypeOf(object) != TypeOf(*Find(source, base.Self)))
    {
      continue;
    }
    if (const auto *their = GetIf<Objects::VisualStyle>(&object))
    {
      CAPTURE(std::format("{} {}", their->Name, HexOf(base.Self)));
      REQUIRE(mine != nullptr);
      const auto &style = Get<Objects::VisualStyle>(*mine);
      CHECK(style.Name == their->Name);
      CHECK(style.Kind == their->Kind);
      CHECK(LowerProperties(style.Properties, upgrading ? from : to) ==
            LowerProperties(their->Properties, upgrading ? from : to));
      CHECK(Roundtrip(ours, style.Base) == Roundtrip(theirs, base));
    }
    const std::string_view name = TypeOf(object);
    const auto *view = GetIf<Unknown>(&object);
    if (view != nullptr && (name == "ACDBDETAILVIEWSTYLE" || name == "ACDBSECTIONVIEWSTYLE"))
    {
      CAPTURE(std::format("{} {}", name, HexOf(base.Self)));
      REQUIRE(mine != nullptr);
      const auto &own = Get<Unknown>(*mine).Body;
      CHECK(own == (upgrading ? Through(*view, from, to) : view->Body));
      CHECK(Roundtrip(ours, BaseOf(*mine)) == Roundtrip(theirs, base));
    }
  }
}
} // namespace DwgIO

#endif /* DWGIO_TESTS_SUPPORT_STYLES_HPP */
