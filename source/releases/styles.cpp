/***********************************************************************************************************************
 *  @file      source/releases/styles.cpp
 *  @brief     The layers, linetypes and styles as another release holds them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/styles.hpp"

#include "encoding/codepage.hpp"
#include "encoding/encoding.hpp"
#include "encoding/units.hpp"
#include "objects/shapes/mleaderstyle.hpp"
#include "releases/lookup.hpp"
#include "releases/passage.hpp"
#include "releases/table/style.hpp"

#include "dwgio/base.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/header.hpp"
#include "dwgio/lineweight.hpp"
#include "dwgio/objects/mleaderstyle.hpp"
#include "dwgio/objects/mlinestyle.hpp"
#include "dwgio/objects/tablestyles/style.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/tables/control.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/tables/ltype.hpp"
#include "dwgio/tables/table.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/version.hpp"
#include "dwgio/view.hpp"
#include "dwgio/xdata.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <ranges>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
/**
 *  A dash text as a strings area holds it: UTF-16 and a NUL unit from R2007, the code page and a NUL before.
 */
auto EncodeDashText(const std::string &text, bool wide, const Encoding::Scheme &scheme) -> std::vector<std::byte>
{
  std::vector<std::byte> bytes;
  if (wide)
  {
    for (const char16_t unit : Encoding::ToUnits(text))
    {
      bytes.push_back(static_cast<std::byte>(unit & 0xFFU));
      bytes.push_back(static_cast<std::byte>(unit >> 8U));
    }
    bytes.insert(bytes.end(), 2, std::byte{0});
    return bytes;
  }
  for (const char one : Encoding::Encode(scheme, text))
  {
    bytes.push_back(static_cast<std::byte>(one));
  }
  bytes.push_back(std::byte{0});
  return bytes;
}

/**
 *  The linetype indices of a multiline style before R2018 that stand for ByLayer and ByBlock.
 */
constexpr int ByLayerIndex = 32767;
constexpr int ByBlockIndex = 32766;

/**
 *  The records a multiline style's linetype index counts among: the linetype table's entries, in order.
 */
auto CollectLinetypes(const Drawing &drawing) -> std::vector<Handle>
{
  const Tables::Control *control = View::Control(drawing, Tables::Table::LType);
  return control == nullptr ? std::vector<Handle>{} : control->Entries;
}

/**
 *  The record a linetype index before R2018 names: ByLayer's for one beyond the table, noted.
 */
auto FindLinetype(int index, const Passage &passage, Handle self) -> Handle
{
  const Header &header = passage.Drawing->Header;
  if (index == ByLayerIndex)
  {
    return header.LTypeByLayer;
  }
  if (index == ByBlockIndex)
  {
    return header.LTypeByBlock;
  }
  const std::vector<Handle> table = CollectLinetypes(*passage.Drawing);
  if (index >= 0 && static_cast<std::size_t>(index) < table.size())
  {
    return table[static_cast<std::size_t>(index)];
  }
  Note(passage, self, std::format("linetype {} of {} not in its table, ByLayer", index, passage.What));
  return header.LTypeByLayer;
}

/**
 *  The linetype index before R2018 a record goes by: ByLayer's for a record not in the table, noted.
 */
auto IndexOf(Handle record, const Passage &passage, Handle self) -> int
{
  const Header &header = passage.Drawing->Header;
  if (record == Handle{} || record == header.LTypeByLayer)
  {
    return ByLayerIndex;
  }
  if (record == header.LTypeByBlock)
  {
    return ByBlockIndex;
  }
  const std::vector<Handle> table = CollectLinetypes(*passage.Drawing);
  const auto at = std::ranges::find(table, record);
  if (at != table.end())
  {
    return static_cast<int>(at - table.begin());
  }
  Note(passage, self, std::format("linetype {} of {} not in its table, ByLayer", HexOf(record), passage.What));
  return ByLayerIndex;
}
} // namespace

void LowerLayer(Tables::Layer &layer, const Passage &passage)
{
  if (layer.Lineweight != Lineweight::Default)
  {
    Note(passage, layer.Record.Base.Self, std::format("lineweight of layer {} not kept by R14", layer.Record.Name));
  }
  layer.Lineweight = Lineweight::Default;
  layer.Style = {};
}

void ConvertLType(Tables::LType &ltype, const Passage &passage, const Crossings &crossing)
{
  if (!crossing.R2007Up && !crossing.R2007Down)
  {
    return;
  }
  const bool upgrading = crossing.R2007Up;
  constexpr std::size_t Narrow = 256;
  constexpr std::size_t Wide = 512;
  constexpr unsigned TextFlag = 2;
  const auto texted = [](const Tables::Dash &dash) -> bool
  {
    return (static_cast<unsigned>(dash.Flags) & TextFlag) != 0;
  };
  const Encoding::Scheme scheme = Encoding::SchemeOf(passage.Drawing->Codepage);
  std::size_t size = Narrow;
  if (upgrading)
  {
    size = std::ranges::any_of(ltype.Dashes, texted) ? Wide : 0;
  }
  std::vector<std::byte> area(size, std::byte{0});
  std::size_t at = 0;
  for (Tables::Dash &dash : ltype.Dashes | std::views::filter(texted))
  {
    const std::vector<std::byte> bytes = EncodeDashText(dash.Text, upgrading, scheme);
    if (at + bytes.size() > area.size())
    {
      Note(passage, ltype.Record.Base.Self, std::format("text of a dash of {} beyond its strings area", passage.What));
      continue;
    }
    std::ranges::copy(bytes, area.begin() + static_cast<std::ptrdiff_t>(at));
    dash.Shape = static_cast<int>(at);
    at += bytes.size();
  }
  ltype.Strings = std::move(area);
}

void ConvertMLineStyle(Objects::MLineStyle &style, const Passage &passage, const Crossings &crossing)
{
  for (Objects::MLineStyles::Element &element : style.Elements)
  {
    if (crossing.R2018Up)
    {
      element.Record = FindLinetype(element.Linetype, passage, style.Base.Self);
      element.Linetype = ByLayerIndex; // What R2018's reader leaves, the file having no index.
    }
    if (crossing.R2018Down)
    {
      element.Linetype = IndexOf(element.Record, passage, style.Base.Self);
      element.Record = {};
    }
  }
}

void ConvertVersion(const VersionPlace &versioned, const Passage &passage, const Crossings &crossing)
{
  Base &base = *versioned.Base;
  int &version = *versioned.Version;
  const auto record = std::ranges::find_if(base.Extended, Objects::MLeaderStyles::IsVersion);
  if (crossing.R2010Up && record != base.Extended.end())
  {
    const auto *value = std::get_if<std::int64_t>(&record->Tags.front().Value);
    version = value == nullptr ? version : static_cast<int>(*value);
    base.Extended.erase(record);
  }
  if (!crossing.R2010Down || record != base.Extended.end())
  {
    return;
  }
  const auto *appid = passage.Lookup->Record<Tables::AppId>(Objects::MLeaderStyles::Versioned);
  if (appid == nullptr)
  {
    Note(passage, base.Self,
         std::format("application {} missing for the version of {}", Objects::MLeaderStyles::Versioned, passage.What));
    return;
  }
  base.Extended.push_back(
      Xdata{.Application = {},
            .AppId = appid->Record.Base.Self,
            .Tags = {Tag{.Code = Objects::MLeaderStyles::VersionCode, .Value = std::int64_t{version}}}});
}

void ConvertTableStyle(Objects::TableStyle &style, const Passage &passage, const Crossings &crossing)
{
  if (crossing.R2010Up)
  {
    if (style.TitleSuppressed || style.HeaderSuppressed)
    {
      Note(passage, style.Base.Self, std::format("suppressed rows of {} not kept by R2010", passage.What));
    }
    Table::Raise(style, passage.Drawing->Header.LTypeByBlock);
    if (crossing.R2018Up)
    {
      style.Flags = static_cast<int>(static_cast<unsigned>(style.Flags) | Table::UpgradeBit);
    }
  }
  if (crossing.R2010Down)
  {
    Table::Lower(style);
  }
  if (crossing.R2018Down)
  {
    style.Flags = static_cast<int>(static_cast<unsigned>(style.Flags) & ~Table::UpgradeBit);
  }
}
} // namespace DwgIO::Releases
