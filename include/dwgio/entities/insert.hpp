/***********************************************************************************************************************
 *  @file      dwgio/entities/insert.hpp
 *  @brief     The INSERT and MINSERT entities: a block placed once or as a grid.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_INSERT_HPP
#define DWGIO_ENTITIES_INSERT_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/vector.hpp"

#include <optional>
#include <vector>

namespace DwgIO::Entities
{
/**
 *  An INSERT, or a MINSERT when Columns holds a value: a block placed once, or as a grid of columns and rows.
 */
struct DWGIO_API Insert
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  Vector Position{}; /**< Where the block's base point goes, in object coordinates. */
  Vector Scale{.X = 1, .Y = 1, .Z = 1};
  double Rotation{}; /**< In radians. */
  Vector Extrusion{Up};
  bool Follows{};               /**< Whether attributes follow, up to the SEQEND. Group 66. */
  std::optional<int> Columns{}; /**< A MINSERT's; none for an INSERT.                       */
  std::optional<int> Rows{};    /**< A MINSERT's; none for an INSERT.                       */
  double ColumnSpacing{};
  double RowSpacing{};
  Handle Block{};
  Handle First{};              /**< Null without attributes.               */
  Handle Last{};               /**< Null without attributes.               */
  Handle SeqEnd{};             /**< A hard owner; null without attributes. */
  std::vector<Handle> Owned{}; /**< From R2004; First and Last before.     */

  auto operator==(const Insert &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_INSERT_HPP */
