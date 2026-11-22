/***********************************************************************************************************************
 *  @file      dwgio/space.hpp
 *  @brief     The space an entity lies in.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_SPACE_HPP
#define DWGIO_SPACE_HPP

#include <cstdint>

namespace DwgIO
{
/**
 *  The space an entity lies in, as an entity's mode stores it.
 */
enum class Space : std::uint8_t
{
  Owner, /**< A block's: the entity stores its owner's handle. */
  Paper, /**< The active paper space.                          */
  Model,
};
} // namespace DwgIO

#endif /* DWGIO_SPACE_HPP */
