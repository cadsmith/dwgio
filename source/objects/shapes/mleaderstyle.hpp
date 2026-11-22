/***********************************************************************************************************************
 *  @file      source/objects/shapes/mleaderstyle.hpp
 *  @brief     How an MLEADERSTYLE lies in the file, and which format it is in.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_SHAPES_MLEADERSTYLE_HPP
#define DWGIO_OBJECTS_SHAPES_MLEADERSTYLE_HPP

#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/nested.hpp"
#include "fields/since.hpp"
#include "objects/shape.hpp"

#include "dwgio/base.hpp"
#include "dwgio/objects/mleaderstyle.hpp"
#include "dwgio/version.hpp"
#include "dwgio/xdata.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects::MLeaderStyles
{
/**
 *  Before R2010 this application's extended data carries a multileader's or its style's version, as a 1070.
 */
inline constexpr std::string_view Versioned = "ACAD_MLEADERVER";

inline constexpr int VersionCode = 1070;

/**
 *  A file names the application by its APPID record only, which the objects may not have reached yet, so the record is
 *  told by holding the version alone.
 */
inline auto IsVersion(const Xdata &xdata) -> bool
{
  return xdata.Tags.size() == 1 && xdata.Tags.front().Code == VersionCode;
}

/**
 *  The new format: any from R2010, one with ACAD_MLEADERVER extended data before.
 */
inline auto IsNewFormat(const Base &base, Version release) -> bool
{
  return release >= Version::R2010 || std::ranges::any_of(base.Extended, IsVersion);
}
} // namespace DwgIO::Objects::MLeaderStyles

namespace DwgIO::Objects::Shapes
{
/**
 *  Only in the new format.
 */
inline auto ReadAngle(MLeaderStyle &style, const Fields::Input &in) -> bool
{
  if (MLeaderStyles::IsNewFormat(style.Base, in.With.Release))
  {
    style.Text.AngleType = in.In->BS();
  }
  return !in.In->HasFailed();
}

inline auto WriteAngle(const MLeaderStyle &style, const Fields::Output &to) -> bool
{
  if (MLeaderStyles::IsNewFormat(style.Base, to.With.Release))
  {
    to.To->BS(style.Text.AngleType);
  }
  return true;
}

/**
 *  Only in the new format.
 */
inline auto ReadFlush(MLeaderStyle &style, const Fields::Input &in) -> bool
{
  if (MLeaderStyles::IsNewFormat(style.Base, in.With.Release))
  {
    style.Text.AlwaysLeft = in.In->B();
  }
  return !in.In->HasFailed();
}

inline auto WriteFlush(const MLeaderStyle &style, const Fields::Output &to) -> bool
{
  if (MLeaderStyles::IsNewFormat(style.Base, to.With.Release))
  {
    to.To->B(style.Text.AlwaysLeft);
  }
  return true;
}
} // namespace DwgIO::Objects::Shapes

namespace DwgIO::Objects
{
/**
 *  Chapter 20.4.87; stored under its class.
 *
 *  The annotative bit comes before the bit the files set for a changed style.
 */
template <>
struct Shape<Objects::MLeaderStyle>
{
  static constexpr std::array<std::uint16_t, 0> Types{};
  static constexpr std::string_view Name = "MLEADERSTYLE";

  static auto Layout()
  {
    using Objects::MLeaderStyle;
    using Objects::MLeaderStyles::BlockFormat;
    using Objects::MLeaderStyles::LeaderFormat;
    using Objects::MLeaderStyles::TextFormat;
    return Fields::Layout(
        Fields::Since(Version::R2010, Fields::Short(&MLeaderStyle::Revision)),
        Fields::Short(&MLeaderStyle::ContentType), Fields::Short(&MLeaderStyle::DrawOrder),
        Fields::Part(&MLeaderStyle::Leader,
                     Fields::Layout(Fields::Short(&LeaderFormat::DrawOrder), Fields::Long(&LeaderFormat::MaxPoints),
                                    Fields::Double(&LeaderFormat::FirstSegmentAngle),
                                    Fields::Double(&LeaderFormat::SecondSegmentAngle),
                                    Fields::Short(&LeaderFormat::LeaderType), Fields::CmColor(&LeaderFormat::Color),
                                    Fields::Link(&LeaderFormat::Linetype, 5), Fields::Long(&LeaderFormat::Lineweight),
                                    Fields::Bit(&LeaderFormat::EnableLanding),
                                    Fields::Double(&LeaderFormat::LandingGap), Fields::Bit(&LeaderFormat::EnableDogleg),
                                    Fields::Double(&LeaderFormat::DoglegLength))),
        Fields::Text(&MLeaderStyle::Description),
        Fields::Part(&MLeaderStyle::Leader, Fields::Layout(Fields::Link(&LeaderFormat::Arrowhead, 5),
                                                           Fields::Double(&LeaderFormat::ArrowheadSize))),
        Fields::Part(&MLeaderStyle::Text,
                     Fields::Layout(Fields::Text(&TextFormat::DefaultText), Fields::Link(&TextFormat::Style, 5),
                                    Fields::Short(&TextFormat::LeftAttachment),
                                    Fields::Short(&TextFormat::RightAttachment))),
        Fields::Custom(Shapes::ReadAngle, Shapes::WriteAngle),
        Fields::Part(&MLeaderStyle::Text,
                     Fields::Layout(Fields::Short(&TextFormat::Alignment), Fields::CmColor(&TextFormat::Color),
                                    Fields::Double(&TextFormat::Height), Fields::Bit(&TextFormat::Frame))),
        Fields::Custom(Shapes::ReadFlush, Shapes::WriteFlush),
        Fields::Part(&MLeaderStyle::Text, Fields::Layout(Fields::Double(&TextFormat::AlignSpace))),
        Fields::Part(&MLeaderStyle::Block,
                     Fields::Layout(Fields::Link(&BlockFormat::Content, 5), Fields::CmColor(&BlockFormat::Color),
                                    Fields::Point3(&BlockFormat::Scale), Fields::Bit(&BlockFormat::EnableScale),
                                    Fields::Double(&BlockFormat::Rotation), Fields::Bit(&BlockFormat::EnableRotation),
                                    Fields::Short(&BlockFormat::Connection))),
        Fields::Double(&MLeaderStyle::Scale), Fields::Bit(&MLeaderStyle::Annotative),
        Fields::Bit(&MLeaderStyle::OverwriteProperty), Fields::Double(&MLeaderStyle::BreakGapSize),
        Fields::Since(Version::R2010,
                      Fields::Part(&MLeaderStyle::Text, Fields::Layout(Fields::Short(&TextFormat::AttachmentDirection),
                                                                       Fields::Short(&TextFormat::TopAttachment),
                                                                       Fields::Short(&TextFormat::BottomAttachment)))),
        Fields::Since(Version::R2013, Fields::Bit(&MLeaderStyle::Unknown1)));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_SHAPES_MLEADERSTYLE_HPP */
