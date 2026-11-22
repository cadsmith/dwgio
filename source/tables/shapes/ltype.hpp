/***********************************************************************************************************************
 *  @file      source/tables/shapes/ltype.hpp
 *  @brief     How an LTYPE record lies in the file, with its dashes and their text.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TABLES_SHAPES_LTYPE_HPP
#define DWGIO_TABLES_SHAPES_LTYPE_HPP

#include "encoding/encoding.hpp"
#include "encoding/units.hpp"
#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/fixed.hpp"
#include "fields/layout.hpp"
#include "fields/list.hpp"
#include "fields/nested.hpp"
#include "fields/since.hpp"
#include "fields/when.hpp"
#include "objects/shape.hpp"
#include "tables/shapes/record.hpp"

#include "dwgio/tables/ltype.hpp"
#include "dwgio/version.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace DwgIO::Tables::Shapes
{
/**
 *  Up to R2004.
 */
constexpr std::size_t StringsBytes = 256;

/**
 *  From R2007, UTF-16, only when a dash has text.
 */
constexpr std::size_t WideStringsBytes = 512;

/**
 *  2 in the files, whatever chapter 20.4.58 says.
 */
constexpr unsigned TextFlag = 2;

/**
 *  A text dash's shape number indexes the strings area; one beyond it is noted and left empty.
 */
inline auto ReadTexts(LType &ltype, const Fields::Input &in) -> bool
{
  for (Dash &dash : ltype.Dashes)
  {
    dash.Text.clear();
    if ((static_cast<unsigned>(dash.Flags) & TextFlag) == 0)
    {
      continue;
    }
    const auto at = static_cast<std::size_t>(dash.Shape);
    if (at >= ltype.Strings.size())
    {
      Note(in, IssueCode::Damaged, "dash text beyond the strings area");
      continue;
    }
    if (in.With.Release >= Version::R2007)
    {
      std::u16string units;
      for (std::size_t index = at; index + 1 < ltype.Strings.size(); index += 2)
      {
        const auto unit = static_cast<char16_t>(std::to_integer<unsigned>(ltype.Strings[index]) |
                                                (std::to_integer<unsigned>(ltype.Strings[index + 1]) << 8U));
        if (unit == 0)
        {
          break;
        }
        units.push_back(unit);
      }
      dash.Text = Encoding::FromUnits(units);
      continue;
    }
    std::string raw;
    for (std::size_t index = at; index < ltype.Strings.size() && ltype.Strings[index] != std::byte{0}; ++index)
    {
      raw.push_back(static_cast<char>(ltype.Strings[index]));
    }
    dash.Text = Encoding::Decode(Fields::TextCode::Scheme(in.With), raw);
  }
  return true;
}

inline auto HasText(const LType &ltype) -> bool
{
  return std::ranges::any_of(ltype.Dashes, [](const Dash &dash) -> bool
                             { return (static_cast<unsigned>(dash.Flags) & TextFlag) != 0; });
}

/**
 *  The strings area is written back as kept; the texts are read from it, never written.
 */
inline auto WriteTexts(const LType & /*ltype*/, const Fields::Output & /*to*/) -> bool
{
  return true;
}
} // namespace DwgIO::Tables::Shapes

namespace DwgIO::Objects
{
template <>
struct Shape<Tables::LType>
{
  static constexpr std::array<std::uint16_t, 1> Types{57};
  static constexpr std::string_view Name = "LTYPE";

  static auto Layout()
  {
    using Tables::Dash;
    using Tables::LType;
    using namespace Tables::Shapes;
    return Fields::Layout(
        Fields::Part(&LType::Record, RecordLayout()), Fields::Text(&LType::Description), Fields::Double(&LType::Length),
        Fields::Byte(&LType::Alignment),
        Fields::List(&LType::Dashes, Fields::Counted::Char, Fields::Double(&Dash::Length), Fields::Short(&Dash::Shape),
                     Fields::RawDouble(&Dash::Along), Fields::RawDouble(&Dash::Across), Fields::Double(&Dash::Scale),
                     Fields::Double(&Dash::Rotation), Fields::Short(&Dash::Flags),
                     Fields::Link(&Dash::Style, HardPointer)),
        Fields::Until(Version::R2004, Fields::FixedBytes(&LType::Strings, StringsBytes)),
        Fields::Since(Version::R2007, Fields::When(HasText, Fields::FixedBytes(&LType::Strings, WideStringsBytes))),
        Fields::Custom(ReadTexts, WriteTexts));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_TABLES_SHAPES_LTYPE_HPP */
