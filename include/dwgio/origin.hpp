/***********************************************************************************************************************
 *  @file      dwgio/origin.hpp
 *  @brief     Where an entity takes its linetype, plot style or material from.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ORIGIN_HPP
#define DWGIO_ORIGIN_HPP

#include <cstdint>

namespace DwgIO
{
/**
 *  Where an entity takes its linetype, plot style or material from, as the two bits the file stores say.
 */
enum class Origin : std::uint8_t
{
  ByLayer,
  ByBlock,    /**< The block insert the entity is drawn in.     */
  Continuous, /**< The linetype Continuous; a linetype only.    */
  Record,     /**< The record the entity names beside the bits. */
};
} // namespace DwgIO

#endif /* DWGIO_ORIGIN_HPP */
