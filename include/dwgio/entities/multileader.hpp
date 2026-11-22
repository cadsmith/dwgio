/***********************************************************************************************************************
 *  @file      dwgio/entities/multileader.hpp
 *  @brief     The MULTILEADER entity: leaders pointing from a text, a block or a tolerance.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_MULTILEADER_HPP
#define DWGIO_ENTITIES_MULTILEADER_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/color.hpp"
#include "dwgio/common.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/matrix.hpp"
#include "dwgio/vector.hpp"

#include <string>
#include <vector>

namespace DwgIO::Entities::MultiLeaders
{
/**
 *  A break in a leader line, between two points of one of its segments.
 */
struct DWGIO_API LineBreak
{
  int Index{};    /**< The segment the break is in. Group 90.   */
  Vector Start{}; /**< In world coordinates. Groups 11, 21, 31. */
  Vector End{};   /**< In world coordinates. Groups 12, 22, 32. */

  auto operator==(const LineBreak &) const -> bool = default;
};

/**
 *  A leader line of a MULTILEADER: its vertices, its breaks and what it overrides of its MULTILEADER.
 */
struct DWGIO_API LeaderLine
{
  std::vector<Vector> Vertices{};      /**< In world coordinates. Groups 10, 20, 30.                  */
  std::vector<LineBreak> Breaks{};     /**< Each opened by group 90.                                  */
  int Index{};                         /**< The line's index in its leader. Group 91.                 */
  DwgIO::Color Color{Colors::ByBlock}; /**< From R2010. Group 92.                                     */
  int LeaderType{1};                   /**< From R2010: 0 invisible, 1 straight, 2 spline. Group 170. */
  Handle Linetype{};                   /**< From R2010. Group 340.                                    */
  int Lineweight{-2};                  /**< From R2010. Group 171.                                    */
  double ArrowheadSize{};              /**< From R2010. Group 40.                                     */
  Handle Arrowhead{};                  /**< The arrowhead's BLOCK_RECORD, from R2010. Group 341.      */
  /**
   *  Which properties the line has of its own, from R2010: 1 the leader type, 2 the color, 4 the linetype, 8 the
   *  lineweight, 16 the arrowhead size, 32 the arrowhead. Group 94.
   */
  int Overrides{};

  auto operator==(const LeaderLine &) const -> bool = default;
};

/**
 *  A break in the dogleg of a leader.
 */
struct DWGIO_API DoglegBreak
{
  Vector Start{}; /**< In world coordinates. Groups 12, 22, 32. */
  Vector End{};   /**< In world coordinates. Groups 13, 23, 33. */

  auto operator==(const DoglegBreak &) const -> bool = default;
};

/**
 *  A leader of a MULTILEADER, which DXF calls its leader root: its landing, its dogleg and the leader lines that run
 *  from it.
 */
struct DWGIO_API LeaderRoot
{
  bool HasLastPoint{true};    /**< Group 290. */
  bool HasDoglegVector{true}; /**< Group 291. */
  /**
   *  The point the leader lines end at, where the landing starts, in world coordinates. Groups 10, 20, 30.
   */
  Vector LastPoint{};
  Vector DoglegVector{DwgIO::Right}; /**< Groups 11, 21, 31.       */
  std::vector<DoglegBreak> Breaks{}; /**< Each opened by group 12. */
  int Index{};
  double DoglegLength{};           /**< Group 40.                                          */
  std::vector<LeaderLine> Lines{}; /**< Each between group 304 LEADER_LINE{ and group 305. */
  /**
   *  How the leader attaches to the content: 0 horizontally, 1 vertically. Group 271, from R2010.
   */
  int AttachmentDirection{};

  auto operator==(const LeaderRoot &) const -> bool = default;
};

/**
 *  The context data of a MULTILEADER: where its content and leaders lie, and how its text or block is drawn.
 */
struct DWGIO_API ContextData
{
  double ContentScale{1};     /**< Group 40.                                                   */
  Vector ContentBasePoint{};  /**< In world coordinates. Groups 10, 20, 30.                    */
  double TextHeight{};        /**< Group 41.                                                   */
  double ArrowheadSize{};     /**< Group 140.                                                  */
  double LandingGap{};        /**< Between the landing and the text. Group 145.                */
  int TextLeftAttachment{1};  /**< How text attaches to a leader on its left. Group 174.       */
  int TextRightAttachment{1}; /**< How text attaches to a leader on its right. Group 175.      */
  int TextAlignment{};        /**< Group 176.                                                  */
  int AttachmentType{};       /**< How the content attaches, as AutoCAD records it. Group 177. */
  /**
   *  Whether the content is text, which the members from Text to Unknown1 then describe. Group 290.
   */
  bool HasMText{};
  std::string Text{};                            /**< With its MTEXT formatting codes. Group 304.     */
  Vector TextNormal{DwgIO::Up};                  /**< Groups 11, 21, 31.                              */
  Handle TextStyle{};                            /**< The STYLE record. Group 340.                    */
  Vector TextLocation{};                         /**< In world coordinates. Groups 12, 22, 32.        */
  Vector TextDirection{DwgIO::Right};            /**< Groups 13, 23, 33.                              */
  double TextRotation{};                         /**< In radians. Group 42.                           */
  double BoundaryWidth{};                        /**< Of the text's frame. Group 43.                  */
  double BoundaryHeight{};                       /**< Of the text's frame. Group 44.                  */
  double LineSpacingFactor{1};                   /**< A factor of the default line spacing. Group 45. */
  int LineSpacingStyle{1};                       /**< 1 at least, 2 exactly. Group 170.               */
  DwgIO::Color TextColor{Colors::ByLayer};       /**< Group 90.                                       */
  int TextAttachment{1};                         /**< As an MTEXT's group 71 gives it. Group 171.     */
  int FlowDirection{5};                          /**< As an MTEXT's group 72 gives it. Group 172.     */
  DwgIO::Color BackgroundColor{Colors::ByLayer}; /**< Group 91.                                       */
  /**
   *  How far the background reaches past the text, as a factor of the letter height. Group 141.
   */
  double BackgroundScale{};
  int BackgroundTransparency{}; /**< Group 92.                                                   */
  bool BackgroundFill{};        /**< The text has a background fill. Group 291.                  */
  bool BackgroundMask{};        /**< The background takes the drawing window's color. Group 292. */
  int ColumnType{};             /**< 0 none, 1 static, 2 dynamic. Group 173.                     */
  bool AutoHeight{};            /**< The frame's height follows the text. Group 293.             */
  double ColumnWidth{};         /**< Group 142.                                                  */
  double ColumnGutter{};
  bool ColumnFlowReversed{};           /**< The text flows through the columns in reverse. Group 294.         */
  std::vector<double> ColumnHeights{}; /**< Group 144.                                                        */
  bool WordBreak{};                    /**< Words break at the frame's edge. Group 295.                       */
  bool Unknown1{};                     /**< Unnamed in the specification; 0 as AutoCAD writes it. Not in DXF. */
  /**
   *  Whether the content is a block, which the members from BlockContent to BlockTransform then describe; stored only
   *  when the content is no text. Group 296.
   */
  bool HasBlock{};
  Handle BlockContent{};                     /**< The BLOCK_RECORD. Group 341.             */
  Vector BlockNormal{DwgIO::Up};             /**< Groups 14, 24, 34.                       */
  Vector BlockPosition{};                    /**< In world coordinates. Groups 15, 25, 35. */
  Vector BlockScale{.X = 1, .Y = 1, .Z = 1}; /**< Groups 16, 26, 36.                       */
  double BlockRotation{};                    /**< In radians. Group 46.                    */
  DwgIO::Color BlockColor{Colors::ByLayer};  /**< Group 93.                                */
  Matrix BlockTransform{Identity};           /**< Each cell a group 47. */
  Vector PlaneOrigin{};                      /**< In world coordinates. Groups 110, 120, 130. */
  Vector PlaneXAxis{DwgIO::Right};           /**< Groups 111, 121, 131.                       */
  Vector PlaneYAxis{.X = 0, .Y = 1, .Z = 0}; /**< Groups 112, 122, 132.                       */
  bool PlaneNormalReversed{};                /**< Group 297.                                  */
  std::vector<LeaderRoot> Leaders{};         /**< Each between group 302 LEADER{ and group 303. */
  int TextBottomAttachment{9}; /**< How text attaches to a leader below it, 9 at its center. Group 272, from R2010. */
  int TextTopAttachment{9};    /**< How text attaches to a leader above it, 9 at its center. Group 273, from R2010. */

  auto operator==(const ContextData &) const -> bool = default;
};

/**
 *  An arrowhead of a MULTILEADER, by the index of the leader line it ends.
 */
struct DWGIO_API Arrowhead
{
  int Index{};    /**< Group 94.                                  */
  Handle Block{}; /**< The BLOCK_RECORD that draws it. Group 345. */

  auto operator==(const Arrowhead &) const -> bool = default;
};

/**
 *  The value a MULTILEADER gives an attribute of its block.
 */
struct DWGIO_API BlockAttribute
{
  Handle Definition{}; /**< The block's ATTDEF the value is for. Group 330. */
  int Index{};         /**< Group 177.                                      */
  double Width{};      /**< Of the attribute's text. Group 44.              */
  std::string Text{};

  auto operator==(const BlockAttribute &) const -> bool = default;
};

/**
 *  What a MULTILEADER's AcDbMLeader subclass holds besides its common data: its context data, and what it overrides of
 *  its MLEADERSTYLE.
 */
struct DWGIO_API MLeader
{
  /**
   *  The version of the MULTILEADER's layout, 2 as AutoCAD writes it.
   *
   *  Group 270, from R2010; before, the 1070 of its ACAD_MLEADERVER extended data.
   */
  int Version{2};
  ContextData Context{}; /**< Between group 300 CONTEXT_DATA{ and group 301. */
  Handle Style{};
  int Overrides{};   /**< Which properties of the style the MULTILEADER overrides, as bits. Group 90. */
  int LeaderType{1}; /**< 0 invisible, 1 straight, 2 spline. Group 170.                               */
  DwgIO::Color LineColor{Colors::ByBlock}; /**< Group 91. */
  Handle Linetype{};                       /**< Of the leader lines. Group 341. */
  int Lineweight{-2};                      /**< As an entity's group 370 gives it. Group 171. */
  bool EnableLanding{true};
  bool EnableDogleg{true};    /**< Group 291.                                                   */
  double DoglegLength{};      /**< Group 41.                                                    */
  Handle Arrowhead{};         /**< The BLOCK_RECORD; null for the default arrowhead. Group 342. */
  double ArrowheadSize{};     /**< Group 42.                                                    */
  int ContentType{2};         /**< 0 none, 1 a block, 2 an MTEXT, 3 a tolerance. Group 172.     */
  Handle TextStyle{};         /**< The STYLE record. Group 343.                                 */
  int TextLeftAttachment{1};  /**< How text attaches to a leader on its left. Group 173.        */
  int TextRightAttachment{1}; /**< How text attaches to a leader on its right. Group 95.        */
  /**
   *  How the text is turned: 0 along the last leader segment, 1 horizontal, 2 along it but kept upright. Group 174.
   */
  int TextAngleType{1};
  int TextAlignment{};                     /**< 0 left, 1 center, 2 right. Group 175. */
  DwgIO::Color TextColor{Colors::ByBlock}; /**< Group 92.                             */
  bool TextFrame{};
  Handle BlockContent{};                     /**< The BLOCK_RECORD. Group 344. */
  DwgIO::Color BlockColor{Colors::ByBlock};  /**< Group 93.                    */
  Vector BlockScale{.X = 1, .Y = 1, .Z = 1}; /**< Groups 10, 20, 30.           */
  double BlockRotation{};                    /**< In radians. Group 43.        */
  /**
   *  Where the block meets the leaders: 0 the center of its extents, 1 its insertion point. Group 176.
   */
  int BlockConnection{};
  bool Annotative{};
  std::vector<MultiLeaders::Arrowhead> Arrowheads{}; /**< Of the leader lines, each opened by group 94; up to R2007. */
  std::vector<BlockAttribute> BlockAttributes{};     /**< Each opened by group 330.                                  */
  bool TextDirectionNegative{}; /**< Group 294.                                                   */
  int IpeAlignment{};           /**< The alignment of the text while it is edited in place. Group 178. */
  int TextAttachmentPoint{1};   /**< 1 left, 2 center, 3 right. Group 179.                        */
  double OverallScale{1};       /**< Group 45.                                                    */
  /**
   *  How text attaches to the leaders: 0 horizontally, 1 vertically. Group 271, from R2010.
   */
  int TextAttachmentDirection{};
  int TextBottomAttachment{9}; /**< How text attaches to a leader below it, 9 at its center. Group 272, from R2010. */
  int TextTopAttachment{9};    /**< How text attaches to a leader above it, 9 at its center. Group 273, from R2010. */
  bool ExtendToText{};         /**< Whether the leaders extend to the text. Group 295, from R2013.                  */

  auto operator==(const MLeader &) const -> bool = default;
};
} // namespace DwgIO::Entities::MultiLeaders

namespace DwgIO::Entities
{
/**
 *  A MULTILEADER: leaders that point from a text, a block or a tolerance. AutoCAD stores it under its class in every
 *  release, R14 included.
 */
struct DWGIO_API MultiLeader
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  MultiLeaders::MLeader MLeader{};

  auto operator==(const MultiLeader &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_MULTILEADER_HPP */
