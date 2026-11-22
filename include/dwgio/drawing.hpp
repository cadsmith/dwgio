/***********************************************************************************************************************
 *  @file      dwgio/drawing.hpp
 *  @brief     A drawing as a DWG file holds it, and finding its objects by handle.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_DRAWING_HPP
#define DWGIO_DRAWING_HPP

#include "dwgio/api.hpp"
#include "dwgio/class.hpp"
#include "dwgio/dependencies.hpp"
#include "dwgio/header.hpp"
#include "dwgio/object.hpp"
#include "dwgio/preview.hpp"
#include "dwgio/section.hpp"
#include "dwgio/summary.hpp"
#include "dwgio/version.hpp"

#include <optional>
#include <string>
#include <vector>

namespace DwgIO
{
/**
 *  A drawing as a DWG file holds it: the release, the header variables, the classes, every object, the preview and the
 *  sections kept as they were.
 *
 *  Every string of a drawing is UTF-8, whatever the release: a load converts the file's code page or UTF-16 text, and a
 *  save converts back.
 */
struct DWGIO_API Drawing
{
  /**
   *  The release the file was in, which a save writes unless its options name another.
   */
  Version Release{Version::R2000};
  /**
   *  The code page the file stores its text in before R2007, by the number its header gives, 30 for ANSI_1252; from
   *  R2007 the file stores UTF-16. The strings dwgio gives are UTF-8 either way.
   */
  int Codepage{30};
  /**
   *  The maintenance version the file was stamped with, which a save to its own release stamps again: in the files
   *  AutoCAD 2026 writes, and in a drawing from Create, `DwgIO::MaintenanceOf` of the release.
   */
  int Maintenance{15};
  /**
   *  The sections of the file in its order: auxiliary, preview, header, classes, pad, objects, map, second and
   *  measurement in an AutoCAD file, with unknown for bytes no section claimed.
   *
   *  Saving moves the map after the objects and the second header after the sections it points at where the order has
   *  them earlier, and notes the move.
   */
  std::vector<std::string> Order{};
  DwgIO::Header Header{};
  std::vector<Class> Classes{};  /**< They name the object types of 500 and up. */
  std::vector<Object> Objects{}; /**< In the file's order.                      */
  std::vector<PreviewImage> Preview{};
  /**
   *  The summary: from R2004 the section AcDb:SummaryInfo, in R14 and R2000 the DWGPROPS record of the named objects
   *  dictionary, which stays among the objects.
   *
   *  Empty when the file has none, or one that would not write back as it was, which then stays among the kept
   *  sections. A save writes it in the release's form.
   */
  std::optional<DwgIO::SummaryInfo> SummaryInfo{};
  /**
   *  The file dependencies, from the section AcDb:FileDepList of R2004 and later.
   *
   *  Empty when the file has none, or one that would not write back as it was, which then stays among the kept
   *  sections.
   */
  std::optional<DwgIO::FileDependencies> FileDependencies{};
  /**
   *  The sections kept as they were: auxiliary, pad, second, measurement and unknown, and a paged file's sections dwgio
   *  does not type.
   */
  std::vector<Section> Kept{};

  auto operator==(const Drawing &) const -> bool = default;
};

/**
 *  @return Null when no object has that handle, and for the null handle.
 */
[[nodiscard]] DWGIO_API auto Find(const Drawing &drawing, Handle self) -> const Object *;

[[nodiscard]] DWGIO_API auto Find(Drawing &drawing, Handle self) -> Object *;
} // namespace DwgIO

#endif /* DWGIO_DRAWING_HPP */
