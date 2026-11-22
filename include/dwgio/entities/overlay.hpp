/***********************************************************************************************************************
 *  @file      dwgio/entities/overlay.hpp
 *  @brief     The kinds of file an underlay shows: PDF, DWF and DGN.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_OVERLAY_HPP
#define DWGIO_ENTITIES_OVERLAY_HPP

#include <cstdint>

namespace DwgIO::Entities
{
/**
 *  The kind of file an underlay shows.
 */
enum class UnderlayKind : std::uint8_t
{
  Pdf, /**< A PDFUNDERLAY.                      */
  Dwf, /**< A DWFUNDERLAY.                      */
  Dgn, /**< A DGNUNDERLAY, a MicroStation file. */
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_OVERLAY_HPP */
