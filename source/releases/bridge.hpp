/***********************************************************************************************************************
 *  @file      source/releases/bridge.hpp
 *  @brief     The links of a chain carried past the entities a save leaves out.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_BRIDGE_HPP
#define DWGIO_RELEASES_BRIDGE_HPP

#include "releases/lookup.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/version.hpp"

#include <span>

namespace DwgIO::Releases
{
/**
 *  What a save leaves out of a drawing, for a release that chains entities.
 */
struct Omission
{
  Version To{};
  std::span<const Handle> LeftOut{}; /**< Sorted. */
  const Releases::Lookup *Lookup{};
};

/**
 *  Before R2004, carries an entity's links, and a block record's, polyline's or insert's first and last, past the
 *  objects the save leaves out to the nearest it writes: AutoCAD refuses a chain that runs into an object the file
 *  lacks. The object is as ConvertObject gives it.
 */
void Bridge(DwgIO::Object &object, const Drawing &drawing, const Omission &leaving);
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_BRIDGE_HPP */
