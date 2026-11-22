/***********************************************************************************************************************
 *  @file      dwgio/tables/style.hpp
 *  @brief     The STYLE record: a text style or a shape file.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TABLES_STYLE_HPP
#define DWGIO_TABLES_STYLE_HPP

#include "dwgio/api.hpp"
#include "dwgio/tables/record.hpp"

#include <string>

namespace DwgIO::Tables
{
/**
 *  A STYLE record: a text style, or a shape file.
 */
struct DWGIO_API Style
{
  Tables::Record Record{};
  bool Vertical{};         /**< The 4 bit of group 70.                                                   */
  bool Shape{};            /**< A shape file rather than a text style: the 1 bit of group 70.            */
  double Height{};         /**< 0 when each text gives its own. Group 40.                                */
  double Width{1};         /**< Character width over height. Group 41.                                   */
  double Oblique{};        /**< In radians; group 50 gives degrees.                                      */
  int Generation{};        /**< Bits: 2 for text drawn backward, 4 for text drawn upside down. Group 71. */
  double LastHeight{2.5};  /**< The height the style was last used at. Group 42.                         */
  std::string Font{"txt"}; /**< Such as "txt" or "Arial.ttf".                                            */
  std::string Bigfont{};   /**< For Asian characters; empty for none.                                    */

  auto operator==(const Style &) const -> bool = default;
};
} // namespace DwgIO::Tables

#endif /* DWGIO_TABLES_STYLE_HPP */
