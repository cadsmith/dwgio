/***********************************************************************************************************************
 *  @file      dwgio/tables/ltype.hpp
 *  @brief     The LTYPE record: a linetype and its pattern.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TABLES_LTYPE_HPP
#define DWGIO_TABLES_LTYPE_HPP

#include "dwgio/api.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/tables/record.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace DwgIO::Tables
{
/**
 *  One element of a linetype's pattern: a dash, a dot or a space, with the shape or text it may carry.
 */
struct DWGIO_API Dash
{
  double Length{}; /**< Positive for a dash, 0 for a dot, negative for a space. Group 49. */
  /**
   *  The shape's number in the shape file when Flags has 4; the text's index in the strings area when it has 2. Group
   *  75 gives the shape number.
   */
  int Shape{};
  double Along{};    /**< Shape or text offset along the line. Group 44.            */
  double Across{};   /**< Shape or text offset across the line. Group 45.           */
  double Scale{1};   /**< Of the shape or text; 1 for a plain dash. Group 46.       */
  double Rotation{}; /**< Of the shape or text, in radians; group 50 gives degrees. */
  /**
   *  Bits: 1 when Rotation is absolute rather than along the line, 2 when text is drawn, 4 when a shape is, 8 when the
   *  text stays upright.
   *
   *  Group 74 has the first three; the specification swaps 2 and 4, the files do not.
   */
  int Flags{};
  Handle Style{}; /**< The text style or shape file the shape or text is drawn from. Group 340. */
  /**
   *  The text drawn, read from the strings area when Flags has 2. Group 9. A save writes the strings area as it is, not
   *  this.
   */
  std::string Text{};

  auto operator==(const Dash &) const -> bool = default;
};

struct DWGIO_API LType
{
  Tables::Record Record{};
  std::string Description{}; /**< Such as "__ __ __". Group 3. */
  double Length{};
  int Alignment{'A'}; /**< Always 'A'. Group 72. */
  std::vector<Dash> Dashes{};
  /**
   *  The NUL-ended texts the dashes index, kept as the file has them: 256 bytes up to R2004; from R2007 512 bytes of
   *  UTF-16 when a dash has text, else none.
   */
  std::vector<std::byte> Strings{};

  auto operator==(const LType &) const -> bool = default;
};
} // namespace DwgIO::Tables

#endif /* DWGIO_TABLES_LTYPE_HPP */
