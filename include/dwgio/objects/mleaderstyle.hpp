/***********************************************************************************************************************
 *  @file      dwgio/objects/mleaderstyle.hpp
 *  @brief     The MLEADERSTYLE object: how a multileader draws its leaders and content.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_MLEADERSTYLE_HPP
#define DWGIO_OBJECTS_MLEADERSTYLE_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/color.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/vector.hpp"

#include <string>

namespace DwgIO::Objects::MLeaderStyles
{
/**
 *  The version a multileader and its style carry, from R2010 in their data and before it in ACAD_MLEADERVER extended
 *  data.
 */
inline constexpr int Revision = 2;

/**
 *  How a multileader style draws its leader lines.
 */
struct DWGIO_API LeaderFormat
{
  int DrawOrder{};             /**< Which end of a leader is drawn first: 0 the head, 1 the tail. Group 172. */
  int MaxPoints{2};            /**< The most points a leader line has. Group 90.                             */
  double FirstSegmentAngle{};  /**< The angle the first segment snaps to; 0 for any. Group 40.               */
  double SecondSegmentAngle{}; /**< The angle the second segment snaps to; 0 for any. Group 41.              */
  int LeaderType{1};           /**< The kind of line: 0 invisible, 1 straight, 2 spline. Group 173.          */
  DwgIO::Color Color{Colors::ByBlock};
  Handle Linetype{};
  /**
   *  The lineweight in hundredths of a millimeter, or -1 ByLayer, -2 ByBlock, -3 the default. Group 92.
   */
  int Lineweight{-2};
  bool EnableLanding{true};
  double LandingGap{2}; /**< Between the landing and the content. Group 42. */
  bool EnableDogleg{true};
  double DoglegLength{8};  /**< Of the dogleg. Group 43.                                                   */
  Handle Arrowhead{};      /**< The arrowhead's BLOCK_RECORD; null for the closed filled arrow. Group 341. */
  double ArrowheadSize{4}; /**< Of the arrowhead. Group 44.                                                */

  auto operator==(const LeaderFormat &) const -> bool = default;
};

/**
 *  How a multileader style draws its text.
 */
struct DWGIO_API TextFormat
{
  std::string DefaultText{}; /**< The text a new multileader starts with. Group 300. */
  Handle Style{};
  /**
   *  Where a leader from the left meets the text, such as 1 at the middle of its top line. Group 174.
   */
  int LeftAttachment{1};
  int RightAttachment{1}; /**< Where a leader from the right meets the text, as LeftAttachment. Group 178. */
  /**
   *  How the text is turned: 0 along the leader, 1 horizontal, 2 along the leader but never upside down; stored only by
   *  a style of the new format. Group 175.
   */
  int AngleType{1};
  int Alignment{}; /**< 0 left, 1 center, 2 right. Group 176. */
  DwgIO::Color Color{Colors::ByBlock};
  double Height{4};
  bool Frame{};
  bool AlwaysLeft{};    /**< The text is always aligned left; stored only by a style of the new format. Group 297. */
  double AlignSpace{4}; /**< Between the text and its alignment. Group 46.                                         */
  /**
   *  Which way leaders meet the text: 0 from the sides, 1 from above or below. Group 271, from R2010.
   */
  int AttachmentDirection{};
  int BottomAttachment{9}; /**< Where a leader from below meets the text. Group 272, from R2010. */
  int TopAttachment{9};    /**< Where a leader from above meets the text. Group 273, from R2010. */

  auto operator==(const TextFormat &) const -> bool = default;
};

/**
 *  How a multileader style draws a block content.
 */
struct DWGIO_API BlockFormat
{
  Handle Content{};
  DwgIO::Color Color{Colors::ByBlock};
  Vector Scale{.X = 1, .Y = 1, .Z = 1};
  bool EnableScale{true};    /**< Whether Scale applies. Group 293.                                                  */
  double Rotation{};         /**< In radians. Group 141.                                                             */
  bool EnableRotation{true}; /**< Whether Rotation applies. Group 294.                                               */
  int Connection{};          /**< Where the leader meets the block: 0 its extents, 1 its insertion point. Group 177. */

  auto operator==(const BlockFormat &) const -> bool = default;
};
} // namespace DwgIO::Objects::MLeaderStyles

namespace DwgIO::Objects
{
/**
 *  An MLEADERSTYLE: how a multileader draws its leader lines and its text or block, named in the ACAD_MLEADERSTYLE
 *  dictionary.
 *
 *  A style of the new format, any from R2010 and one with ACAD_MLEADERVER extended data before, stores the text's angle
 *  and flush bit.
 */
struct DWGIO_API MLeaderStyle
{
  DwgIO::Base Base{};
  /**
   *  2 as AutoCAD writes it.
   *
   *  Group 179 from R2010; before, the 1070 of the ACAD_MLEADERVER extended data.
   */
  int Revision{MLeaderStyles::Revision};
  /**
   *  The content at the end of the leader: 0 none, 1 a block, 2 text, 3 a tolerance. Group 170.
   */
  int ContentType{2};
  int DrawOrder{1}; /**< What is drawn first: 0 the content, 1 the leader. Group 171. */
  std::string Description{};
  MLeaderStyles::LeaderFormat Leader{};
  MLeaderStyles::TextFormat Text{};
  MLeaderStyles::BlockFormat Block{};
  double Scale{1};          /**< Scales the whole multileader. Group 142.           */
  bool OverwriteProperty{}; /**< A property of the style was overridden. Group 295. */
  bool Annotative{};
  double BreakGapSize{3.75}; /**< The gap left where a leader is broken. Group 143.                            */
  bool Unknown1{};           /**< Unnamed in the specification; 0 as AutoCAD writes it. Group 298, from R2013. */

  auto operator==(const MLeaderStyle &) const -> bool = default;
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_MLEADERSTYLE_HPP */
