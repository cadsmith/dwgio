/***********************************************************************************************************************
 *  @file      source/objects/shapes/mlinestyle.hpp
 *  @brief     How an MLINESTYLE lies in the file, chapter 20.4.73.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_SHAPES_MLINESTYLE_HPP
#define DWGIO_OBJECTS_SHAPES_MLINESTYLE_HPP

#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/list.hpp"
#include "fields/since.hpp"
#include "objects/shape.hpp"

#include "dwgio/objects/mlinestyle.hpp"

#include <array>
#include <cstdint>
#include <string_view>
#include <utility>

namespace DwgIO::Objects::Shapes
{
/**
 *  The flag bits the file swaps against DXF's: 1 and 2, 32 and 64, 512 and 1024.
 */
constexpr std::array<std::pair<unsigned, unsigned>, 3> Swapped{{{1U, 2U}, {32U, 64U}, {512U, 1024U}}};

/**
 *  The same both ways.
 */
inline auto SwapFlags(int flags) -> int
{
  const auto bits = static_cast<unsigned>(flags);
  unsigned out = bits;
  for (const auto &[one, other] : Swapped)
  {
    out &= ~(one | other);
    out |= (bits & one) != 0 ? other : 0U;
    out |= (bits & other) != 0 ? one : 0U;
  }
  return static_cast<int>(out);
}

inline auto ReadFlags(MLineStyle &style, const Fields::Input &in) -> bool
{
  style.Flags = SwapFlags(in.In->BS());
  return !in.In->HasFailed();
}

inline auto WriteFlags(const MLineStyle &style, const Fields::Output &to) -> bool
{
  to.To->BS(SwapFlags(style.Flags));
  return true;
}
} // namespace DwgIO::Objects::Shapes

namespace DwgIO::Objects
{
/**
 *  Chapter 20.4.73; from R2018 each line names its linetype record instead of an index.
 */
template <>
struct Shape<Objects::MLineStyle>
{
  static constexpr std::array<std::uint16_t, 1> Types{73};
  static constexpr std::string_view Name = "MLINESTYLE";

  static auto Layout()
  {
    using Objects::MLineStyle;
    using Objects::MLineStyles::Element;
    return Fields::Layout(Fields::Text(&MLineStyle::Name), Fields::Text(&MLineStyle::Description),
                          Fields::Custom(Shapes::ReadFlags, Shapes::WriteFlags), Fields::CmColor(&MLineStyle::Fill),
                          Fields::Double(&MLineStyle::Start), Fields::Double(&MLineStyle::End),
                          Fields::List(&MLineStyle::Elements, Fields::Counted::Char, Fields::Double(&Element::Offset),
                                       Fields::CmColor(&Element::Color),
                                       Fields::Until(Version::R2013, Fields::Short(&Element::Linetype)),
                                       Fields::Since(Version::R2018, Fields::Link(&Element::Record, 5))));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_SHAPES_MLINESTYLE_HPP */
