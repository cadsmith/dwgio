/***********************************************************************************************************************
 *  @file      dwgio/entities/mtext.hpp
 *  @brief     The MTEXT entity, its background and its columns.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_MTEXT_HPP
#define DWGIO_ENTITIES_MTEXT_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/color.hpp"
#include "dwgio/common.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/vector.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace DwgIO::Entities
{
/**
 *  An MTEXT's background, from R2004.
 */
struct DWGIO_API Background
{
  /**
   *  0 none, 1 a fill, 2 the drawing's background color, 16 a frame around the text from R2018. Group 90.
   */
  int Fill{};
  /**
   *  The fill's margin as a multiple of the text height; kept when Fill has 1 or 16. Group 45.
   */
  double Margin{1.5};
  DwgIO::Color Color{}; /**< Kept when Fill has 1 or 16. Group 63.             */
  int Transparency{};   /**< As stored; kept when Fill has 1 or 16. Group 441. */

  auto operator==(const Background &) const -> bool = default;
};

/**
 *  An MTEXT's columns, from R2018.
 */
struct DWGIO_API Columns
{
  int Kind{}; /**< 0 none, 1 static columns, 2 dynamic ones. Group 75. */
  std::int32_t Count{};
  double Breadth{}; /**< The width of a column. */
  double Gutter{};
  bool Automatic{};              /**< The columns' height follows the text. Group 79.          */
  bool Reversed{};               /**< The columns flow right to left. Group 78.                */
  std::vector<double> Heights{}; /**< Kept for dynamic columns of their own heights. Group 50. */

  auto operator==(const Columns &) const -> bool = default;
};

struct DWGIO_API MText
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  Vector Position{}; /**< The point the text hangs from, as Attachment says, in world coordinates. */
  Vector Extrusion{Up};
  Vector Direction{Right}; /**< A unit vector.                                          */
  double Width{};          /**< Of the wrapping rectangle; 0 for no wrapping. Group 41. */
  double Defined{};        /**< Of the wrapping rectangle. Group 46, from R2007.        */
  double Height{};         /**< Of a line's letters. Group 40.                          */
  /**
   *  Where Position sits on the text: 1 top left through 9 bottom right, row by row. Group 71.
   */
  int Attachment{1};
  int Flow{1};           /**< 1 left to right, 3 top to bottom, 5 by the style. Group 72.                 */
  double ExtentHeight{}; /**< As drawn, kept by AutoCAD; not in DXF.                                      */
  double ExtentWidth{};  /**< As drawn, kept by AutoCAD; not in DXF.                                      */
  std::string Value{};   /**< Paragraphs split by \P, with their formatting codes. Groups 3 and 1 joined. */
  Handle Style{};
  int LineSpacingStyle{1};           /**< 1 at least the factor, 2 exactly it. Group 73.                              */
  double LineSpacingFactor{1};       /**< A multiple of single line spacing. Group 44.                                */
  bool Unknown1{};                   /**< Unnamed in the specification; R2000 writes it.                              */
  Entities::Background Background{}; /**< From R2004.                                                                 */
  bool Annotative{};                 /**< From R2018; when set, the file leaves out the fields below.                 */
  int Version{4};                    /**< From R2018, when not annotative; 4 in AutoCAD 2026's files.                 */
  bool Defaulted{true};              /**< From R2018, when not annotative; AutoCAD sets it.                           */
  Handle Application{};              /**< From R2018, when not annotative: the application the data is registered to. */
  Entities::Columns Columns{};       /**< From R2018, when not annotative.                                            */

  auto operator==(const MText &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_MTEXT_HPP */
