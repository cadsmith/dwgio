/***********************************************************************************************************************
 *  @file      dwgio/entities/face.hpp
 *  @brief     The 3DFACE entity.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_FACE_HPP
#define DWGIO_ENTITIES_FACE_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/corners.hpp"
#include "dwgio/vector.hpp"

namespace DwgIO::Entities
{
/**
 *  A 3DFACE: three or four corners anywhere in space.
 */
struct DWGIO_API Face3d
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  DwgIO::Corners Corners{}; /**< In world coordinates. */
  /**
   *  Which edges are invisible: 1 the first, 2 the second, 4 the third, 8 the fourth. Group 70.
   */
  int Flags{};

  auto operator==(const Face3d &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_FACE_HPP */
