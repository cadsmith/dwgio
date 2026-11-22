/***********************************************************************************************************************
 *  @file      dwgio/objects/mlinestyle.hpp
 *  @brief     The MLINESTYLE object: the lines a multiline draws.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_MLINESTYLE_HPP
#define DWGIO_OBJECTS_MLINESTYLE_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/color.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/vector.hpp"

#include <string>
#include <vector>

namespace DwgIO::Objects::MLineStyles
{
/**
 *  One line of a multiline style.
 */
struct DWGIO_API Element
{
  double Offset{}; /**< From the multiline's center line. */
  DwgIO::Color Color{.Index = 256};
  /**
   *  Up to R2013, the linetype's index in the file's table, where DXF names it; 32767 for ByLayer.
   */
  int Linetype{32767};
  Handle Record{}; /**< R2018 stores this in place of the index. */

  auto operator==(const Element &) const -> bool = default;
};
} // namespace DwgIO::Objects::MLineStyles

namespace DwgIO::Objects
{
struct DWGIO_API MLineStyle
{
  DwgIO::Base Base{};
  std::string Name{}; /**< Such as "Standard". */
  std::string Description{};
  /**
   *  Bits as DXF has them: 1 filled, 2 miters shown, 16, 32 and 64 a line, inner arc or outer arc capping the start,
   *  256, 512 and 1024 the same at the end. Group 70.
   */
  int Flags{};
  DwgIO::Color Fill{.Index = 256};
  double Start{Turn / 4}; /**< In radians. */
  double End{Turn / 4};   /**< In radians. */
  std::vector<MLineStyles::Element> Elements{};

  auto operator==(const MLineStyle &) const -> bool = default;
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_MLINESTYLE_HPP */
