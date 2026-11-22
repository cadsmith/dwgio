/***********************************************************************************************************************
 *  @file      source/entities/shapes/multileader.hpp
 *  @brief     How a MULTILEADER lies in the file: its context, leaders and lines.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_MULTILEADER_HPP
#define DWGIO_ENTITIES_SHAPES_MULTILEADER_HPP

#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/field.hpp"
#include "fields/layout.hpp"
#include "fields/list.hpp"
#include "fields/nested.hpp"
#include "fields/since.hpp"
#include "fields/together.hpp"
#include "fields/when.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/multileader.hpp"
#include "dwgio/version.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace DwgIO::Entities::Shapes
{
/**
 *  A BL count of segments with breaks, each the segment's index and a BL count of its start and end pairs; one Gap per
 *  pair.
 */
inline auto ReadGaps(MultiLeaders::LeaderLine &stem, const Fields::Input &in) -> bool
{
  stem.Breaks.clear();
  std::size_t segments = 0;
  if (!Fields::ReadHandleCount(in, segments))
  {
    return false;
  }
  for (std::size_t segment = 0; segment < segments; ++segment)
  {
    const int index = in.In->BL();
    std::size_t pairs = 0;
    if (!Fields::ReadHandleCount(in, pairs))
    {
      return false;
    }
    for (std::size_t pair = 0; pair < pairs; ++pair)
    {
      MultiLeaders::LineBreak gap{.Index = index, .Start = in.In->Point3(), .End = {}};
      gap.End = in.In->Point3();
      stem.Breaks.push_back(gap);
    }
  }
  return !in.In->HasFailed();
}

/**
 *  Gaps of one segment that follow each other are written as one segment's breaks.
 */
inline auto WriteGaps(const MultiLeaders::LeaderLine &stem, const Fields::Output &to) -> bool
{
  std::vector<std::size_t> starts;
  for (std::size_t at = 0; at < stem.Breaks.size(); ++at)
  {
    if (at == 0 || stem.Breaks[at].Index != stem.Breaks[at - 1].Index)
    {
      starts.push_back(at);
    }
  }
  to.To->BL(static_cast<std::int32_t>(starts.size()));
  for (std::size_t run = 0; run < starts.size(); ++run)
  {
    const std::size_t first = starts[run];
    const std::size_t end = run + 1 < starts.size() ? starts[run + 1] : stem.Breaks.size();
    to.To->BL(stem.Breaks[first].Index);
    to.To->BL(static_cast<std::int32_t>(end - first));
    for (std::size_t at = first; at < end; ++at)
    {
      to.To->Point3(stem.Breaks[at].Start);
      to.To->Point3(stem.Breaks[at].End);
    }
  }
  return true;
}

/**
 *  Sixteen BD.
 */
inline auto ReadFrame(MultiLeaders::ContextData &note, const Fields::Input &in) -> bool
{
  for (double &cell : note.BlockTransform)
  {
    cell = in.In->BD();
  }
  return !in.In->HasFailed();
}

inline auto WriteFrame(const MultiLeaders::ContextData &note, const Fields::Output &to) -> bool
{
  for (const double cell : note.BlockTransform)
  {
    to.To->BD(cell);
  }
  return true;
}

/**
 *  The arrowhead's index is the bit the specification calls "is default".
 */
inline auto ReadTip(MultiLeaders::Arrowhead &tip, const Fields::Input &in) -> bool
{
  tip.Index = in.In->B() ? 1 : 0;
  return !in.In->HasFailed();
}

inline auto WriteTip(const MultiLeaders::Arrowhead &tip, const Fields::Output &to) -> bool
{
  to.To->B(tip.Index != 0);
  return true;
}

inline auto IsTexted(const MultiLeaders::ContextData &note) -> bool
{
  return note.HasMText;
}

/**
 *  Content that is not text is followed by the block bit.
 */
inline auto IsUntexted(const MultiLeaders::ContextData &note) -> bool
{
  return !note.HasMText;
}

inline auto IsBlocked(const MultiLeaders::ContextData &note) -> bool
{
  return note.HasBlock;
}

inline auto StemLayout()
{
  return Fields::Layout(
      Fields::List(&MultiLeaders::LeaderLine::Vertices, Fields::Counted::Long, Fields::Point3()),
      Fields::Custom(ReadGaps, WriteGaps), Fields::Long(&MultiLeaders::LeaderLine::Index),
      Fields::Since(
          Version::R2010, Fields::Short(&MultiLeaders::LeaderLine::LeaderType),
          Fields::CmColor(&MultiLeaders::LeaderLine::Color), Fields::Link(&MultiLeaders::LeaderLine::Linetype, 5),
          Fields::Long(&MultiLeaders::LeaderLine::Lineweight), Fields::Double(&MultiLeaders::LeaderLine::ArrowheadSize),
          Fields::Link(&MultiLeaders::LeaderLine::Arrowhead, 5), Fields::Long(&MultiLeaders::LeaderLine::Overrides)));
}

inline auto BranchLayout()
{
  return Fields::Layout(
      Fields::Bit(&MultiLeaders::LeaderRoot::HasLastPoint), Fields::Bit(&MultiLeaders::LeaderRoot::HasDoglegVector),
      Fields::Point3(&MultiLeaders::LeaderRoot::LastPoint), Fields::Point3(&MultiLeaders::LeaderRoot::DoglegVector),
      Fields::List(&MultiLeaders::LeaderRoot::Breaks, Fields::Counted::Long,
                   Fields::Point3(&MultiLeaders::DoglegBreak::Start), Fields::Point3(&MultiLeaders::DoglegBreak::End)),
      Fields::Long(&MultiLeaders::LeaderRoot::Index), Fields::Double(&MultiLeaders::LeaderRoot::DoglegLength),
      Fields::List(&MultiLeaders::LeaderRoot::Lines, Fields::Counted::Long, Fields::Together(StemLayout())),
      Fields::Since(Version::R2010, Fields::Short(&MultiLeaders::LeaderRoot::AttachmentDirection)));
}

inline auto WordingLayout()
{
  return Fields::Layout(
      Fields::Text(&MultiLeaders::ContextData::Text), Fields::Point3(&MultiLeaders::ContextData::TextNormal),
      Fields::Link(&MultiLeaders::ContextData::TextStyle, 5), Fields::Point3(&MultiLeaders::ContextData::TextLocation),
      Fields::Point3(&MultiLeaders::ContextData::TextDirection),
      Fields::Double(&MultiLeaders::ContextData::TextRotation),
      Fields::Double(&MultiLeaders::ContextData::BoundaryWidth),
      Fields::Double(&MultiLeaders::ContextData::BoundaryHeight),
      Fields::Double(&MultiLeaders::ContextData::LineSpacingFactor),
      Fields::Short(&MultiLeaders::ContextData::LineSpacingStyle),
      Fields::CmColor(&MultiLeaders::ContextData::TextColor), Fields::Short(&MultiLeaders::ContextData::TextAttachment),
      Fields::Short(&MultiLeaders::ContextData::FlowDirection),
      Fields::CmColor(&MultiLeaders::ContextData::BackgroundColor),
      Fields::Double(&MultiLeaders::ContextData::BackgroundScale),
      Fields::Long(&MultiLeaders::ContextData::BackgroundTransparency),
      Fields::Bit(&MultiLeaders::ContextData::BackgroundFill), Fields::Bit(&MultiLeaders::ContextData::BackgroundMask),
      Fields::Short(&MultiLeaders::ContextData::ColumnType), Fields::Bit(&MultiLeaders::ContextData::AutoHeight),
      Fields::Double(&MultiLeaders::ContextData::ColumnWidth), Fields::Double(&MultiLeaders::ContextData::ColumnGutter),
      Fields::Bit(&MultiLeaders::ContextData::ColumnFlowReversed),
      Fields::List(&MultiLeaders::ContextData::ColumnHeights, Fields::Counted::Long,
                   Fields::Self<double, Fields::DoubleCode>{}),
      Fields::Bit(&MultiLeaders::ContextData::WordBreak), Fields::Bit(&MultiLeaders::ContextData::Unknown1));
}

inline auto BlockingLayout()
{
  return Fields::Layout(Fields::Link(&MultiLeaders::ContextData::BlockContent, 4),
                        Fields::Point3(&MultiLeaders::ContextData::BlockNormal),
                        Fields::Point3(&MultiLeaders::ContextData::BlockPosition),
                        Fields::Point3(&MultiLeaders::ContextData::BlockScale),
                        Fields::Double(&MultiLeaders::ContextData::BlockRotation),
                        Fields::CmColor(&MultiLeaders::ContextData::BlockColor),
                        Fields::Custom<MultiLeaders::ContextData>(ReadFrame, WriteFrame));
}

inline auto NoteLayout()
{
  return Fields::Layout(
      Fields::List(&MultiLeaders::ContextData::Leaders, Fields::Counted::Long, Fields::Together(BranchLayout())),
      Fields::Double(&MultiLeaders::ContextData::ContentScale),
      Fields::Point3(&MultiLeaders::ContextData::ContentBasePoint),
      Fields::Double(&MultiLeaders::ContextData::TextHeight), Fields::Double(&MultiLeaders::ContextData::ArrowheadSize),
      Fields::Double(&MultiLeaders::ContextData::LandingGap),
      Fields::Short(&MultiLeaders::ContextData::TextLeftAttachment),
      Fields::Short(&MultiLeaders::ContextData::TextRightAttachment),
      Fields::Short(&MultiLeaders::ContextData::TextAlignment),
      Fields::Short(&MultiLeaders::ContextData::AttachmentType), Fields::Bit(&MultiLeaders::ContextData::HasMText),
      Fields::When(IsTexted, Fields::Together(WordingLayout())),
      Fields::When(IsUntexted, Fields::Bit(&MultiLeaders::ContextData::HasBlock),
                   Fields::When(IsBlocked, Fields::Together(BlockingLayout()))),
      Fields::Point3(&MultiLeaders::ContextData::PlaneOrigin), Fields::Point3(&MultiLeaders::ContextData::PlaneXAxis),
      Fields::Point3(&MultiLeaders::ContextData::PlaneYAxis),
      Fields::Bit(&MultiLeaders::ContextData::PlaneNormalReversed),
      Fields::Since(Version::R2010, Fields::Short(&MultiLeaders::ContextData::TextTopAttachment),
                    Fields::Short(&MultiLeaders::ContextData::TextBottomAttachment)));
}

/**
 *  Chapter 20.4.48 after the common data.
 */
inline auto CalloutLayout()
{
  return Fields::Layout(
      Fields::Since(Version::R2010, Fields::Short(&MultiLeaders::MLeader::Version)),
      Fields::Part(&MultiLeaders::MLeader::Context, NoteLayout()), Fields::Link(&MultiLeaders::MLeader::Style, 5),
      Fields::Long(&MultiLeaders::MLeader::Overrides), Fields::Short(&MultiLeaders::MLeader::LeaderType),
      Fields::CmColor(&MultiLeaders::MLeader::LineColor), Fields::Link(&MultiLeaders::MLeader::Linetype, 5),
      Fields::Long(&MultiLeaders::MLeader::Lineweight), Fields::Bit(&MultiLeaders::MLeader::EnableLanding),
      Fields::Bit(&MultiLeaders::MLeader::EnableDogleg), Fields::Double(&MultiLeaders::MLeader::DoglegLength),
      Fields::Link(&MultiLeaders::MLeader::Arrowhead, 5), Fields::Double(&MultiLeaders::MLeader::ArrowheadSize),
      Fields::Short(&MultiLeaders::MLeader::ContentType), Fields::Link(&MultiLeaders::MLeader::TextStyle, 5),
      Fields::Short(&MultiLeaders::MLeader::TextLeftAttachment),
      Fields::Short(&MultiLeaders::MLeader::TextRightAttachment), Fields::Short(&MultiLeaders::MLeader::TextAngleType),
      Fields::Short(&MultiLeaders::MLeader::TextAlignment), Fields::CmColor(&MultiLeaders::MLeader::TextColor),
      Fields::Bit(&MultiLeaders::MLeader::TextFrame), Fields::Link(&MultiLeaders::MLeader::BlockContent, 5),
      Fields::CmColor(&MultiLeaders::MLeader::BlockColor), Fields::Point3(&MultiLeaders::MLeader::BlockScale),
      Fields::Double(&MultiLeaders::MLeader::BlockRotation), Fields::Short(&MultiLeaders::MLeader::BlockConnection),
      Fields::Bit(&MultiLeaders::MLeader::Annotative),
      Fields::Until(Version::R2007,
                    Fields::List(&MultiLeaders::MLeader::Arrowheads, Fields::Counted::Long,
                                 Fields::Custom(ReadTip, WriteTip), Fields::Link(&MultiLeaders::Arrowhead::Block, 5))),
      Fields::List(
          &MultiLeaders::MLeader::BlockAttributes, Fields::Counted::Long,
          Fields::Link(&MultiLeaders::BlockAttribute::Definition, 4), Fields::Text(&MultiLeaders::BlockAttribute::Text),
          Fields::Short(&MultiLeaders::BlockAttribute::Index), Fields::Double(&MultiLeaders::BlockAttribute::Width)),
      Fields::Bit(&MultiLeaders::MLeader::TextDirectionNegative), Fields::Short(&MultiLeaders::MLeader::IpeAlignment),
      Fields::Short(&MultiLeaders::MLeader::TextAttachmentPoint), Fields::Double(&MultiLeaders::MLeader::OverallScale),
      Fields::Since(Version::R2010, Fields::Short(&MultiLeaders::MLeader::TextAttachmentDirection),
                    Fields::Short(&MultiLeaders::MLeader::TextTopAttachment),
                    Fields::Short(&MultiLeaders::MLeader::TextBottomAttachment)),
      Fields::Since(Version::R2013, Fields::Bit(&MultiLeaders::MLeader::ExtendToText)));
}
} // namespace DwgIO::Entities::Shapes

namespace DwgIO::Objects
{
/**
 *  Chapter 20.4.48; stored under its class in every release.
 */
template <>
struct Shape<Entities::MultiLeader>
{
  static constexpr std::array<std::uint16_t, 0> Types{};
  static constexpr std::string_view Name = "MULTILEADER";

  static auto Layout()
  {
    return Fields::Layout(Fields::Part(&Entities::MultiLeader::MLeader, Entities::Shapes::CalloutLayout()));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_MULTILEADER_HPP */
