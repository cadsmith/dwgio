/***********************************************************************************************************************
 *  @file      dwgio/objects/underlay.hpp
 *  @brief     The PDFDEFINITION, DWFDEFINITION and DGNDEFINITION objects: the files underlays show.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_UNDERLAY_HPP
#define DWGIO_OBJECTS_UNDERLAY_HPP

#include "dwgio/base.hpp"
#include "dwgio/entities/overlay.hpp"

#include <string>

namespace DwgIO::Objects
{
/**
 *  An underlay definition: the PDF, DWF or DGN file the drawing's underlays of that kind show, owned by the
 *  ACAD_PDFDEFINITIONS, ACAD_DWFDEFINITIONS or ACAD_DGNDEFINITIONS dictionary.
 */
template <Entities::UnderlayKind KindV>
struct UnderlayDefinition
{
  DwgIO::Base Base{};
  std::string File{};
  std::string Name{}; /**< A PDF's page, a DWF's sheet or a DGN's model. Group 2. */

  auto operator==(const UnderlayDefinition &) const -> bool = default;
};

using PdfDefinition = UnderlayDefinition<Entities::UnderlayKind::Pdf>;
using DwfDefinition = UnderlayDefinition<Entities::UnderlayKind::Dwf>;
using DgnDefinition = UnderlayDefinition<Entities::UnderlayKind::Dgn>;
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_UNDERLAY_HPP */
