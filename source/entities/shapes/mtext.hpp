/***********************************************************************************************************************
 *  @file      source/entities/shapes/mtext.hpp
 *  @brief     How an MTEXT lies in the file, with its background and R2018 columns.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_MTEXT_HPP
#define DWGIO_ENTITIES_SHAPES_MTEXT_HPP

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "entities/shapes/count.hpp"
#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/since.hpp"
#include "fields/when.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/mtext.hpp"
#include "dwgio/version.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace DwgIO::Entities::Shapes
{
constexpr unsigned Filled = 1;
constexpr unsigned Framed = 16;

/**
 *  The flags, then the margin, the color and the transparency when the flags have a fill or a frame.
 */
inline auto ReadBackground(MText &mtext, const Fields::Input &in) -> bool
{
  mtext.Background.Fill = in.In->BL();
  if ((static_cast<unsigned>(mtext.Background.Fill) & (Filled | Framed)) != 0)
  {
    mtext.Background.Margin = in.In->BD();
    mtext.Background.Color = in.In->CMC(in.With.Release, Fields::TextIn(in));
    mtext.Background.Transparency = in.In->BL();
  }
  return !in.In->HasFailed();
}

inline auto WriteBackground(const MText &mtext, const Fields::Output &to) -> bool
{
  to.To->BL(mtext.Background.Fill);
  if ((static_cast<unsigned>(mtext.Background.Fill) & (Filled | Framed)) != 0)
  {
    to.To->BD(mtext.Background.Margin);
    to.To->CMC(mtext.Background.Color, to.With.Release, Fields::TextOut(to));
    to.To->BL(mtext.Background.Transparency);
  }
  return true;
}

/**
 *  An R2018 text carries its frame when it is not annotative.
 */
inline auto IsFramed(const MText &mtext) -> bool
{
  return !mtext.Annotative;
}

/**
 *  From R2018: a bit set when the text is not annotative and then the version, a bit, the attachment, direction,
 *  position, rectangle and extents again, and the columns; the application is in the handle stream.
 */
inline auto ReadColumns(MText &mtext, const Fields::Input &in) -> bool
{
  mtext.Annotative = !in.In->B();
  if (mtext.Annotative)
  {
    return !in.In->HasFailed();
  }
  mtext.Version = in.In->BS();
  mtext.Defaulted = in.In->B();
  // The rest of the frame repeats what the entity already holds; AutoCAD writes the same values.
  static_cast<void>(in.In->BL());
  static_cast<void>(in.In->Point3());
  static_cast<void>(in.In->Point3());
  for (int repeated = 0; repeated < 4; ++repeated)
  {
    static_cast<void>(in.In->BD());
  }
  Columns &columns = mtext.Columns;
  columns.Kind = in.In->BS();
  if (columns.Kind != 0)
  {
    columns.Count = in.In->BL();
    columns.Breadth = in.In->BD();
    columns.Gutter = in.In->BD();
    columns.Automatic = in.In->B();
    columns.Reversed = in.In->B();
    if (!columns.Automatic && columns.Kind == 2)
    {
      const std::optional<std::size_t> heights = CheckCount(columns.Count, in);
      if (!heights)
      {
        return false;
      }
      columns.Heights.assign(*heights, 0.0);
      for (double &height : columns.Heights)
      {
        height = in.In->BD();
      }
    }
  }
  return !in.In->HasFailed();
}

inline auto WriteColumns(const MText &mtext, const Fields::Output &to) -> bool
{
  to.To->B(!mtext.Annotative);
  if (mtext.Annotative)
  {
    return true;
  }
  to.To->BS(mtext.Version);
  to.To->B(mtext.Defaulted);
  to.To->BL(mtext.Attachment);
  to.To->Point3(mtext.Direction);
  to.To->Point3(mtext.Position);
  to.To->BD(mtext.Width);
  to.To->BD(mtext.Defined);
  to.To->BD(mtext.ExtentWidth);
  to.To->BD(mtext.ExtentHeight);
  const Columns &columns = mtext.Columns;
  to.To->BS(columns.Kind);
  if (columns.Kind != 0)
  {
    to.To->BL(columns.Count);
    to.To->BD(columns.Breadth);
    to.To->BD(columns.Gutter);
    to.To->B(columns.Automatic);
    to.To->B(columns.Reversed);
    if (!columns.Automatic && columns.Kind == 2)
    {
      for (const double height : columns.Heights)
      {
        to.To->BD(height);
      }
    }
  }
  return true;
}
} // namespace DwgIO::Entities::Shapes

namespace DwgIO::Objects
{
template <>
struct Shape<Entities::MText>
{
  static constexpr std::array<std::uint16_t, 1> Types{44};
  static constexpr std::string_view Name = "MTEXT";

  static auto Layout()
  {
    using Entities::MText;
    return Fields::Layout(
        Fields::Point3(&MText::Position), Fields::Point3(&MText::Extrusion), Fields::Point3(&MText::Direction),
        Fields::Double(&MText::Width), Fields::Since(Version::R2007, Fields::Double(&MText::Defined)),
        Fields::Double(&MText::Height), Fields::Short(&MText::Attachment), Fields::Short(&MText::Flow),
        Fields::Double(&MText::ExtentHeight), Fields::Double(&MText::ExtentWidth), Fields::Text(&MText::Value),
        Fields::Link(&MText::Style, 5),
        Fields::Since(Version::R2000, Fields::Short(&MText::LineSpacingStyle),
                      Fields::Double(&MText::LineSpacingFactor), Fields::Bit(&MText::Unknown1)),
        Fields::Since(Version::R2004,
                      Fields::Custom(Entities::Shapes::ReadBackground, Entities::Shapes::WriteBackground)),
        Fields::Since(Version::R2018, Fields::Custom(Entities::Shapes::ReadColumns, Entities::Shapes::WriteColumns),
                      Fields::When(Entities::Shapes::IsFramed, Fields::Link(&MText::Application, 5))));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_MTEXT_HPP */
