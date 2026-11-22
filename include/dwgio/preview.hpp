/***********************************************************************************************************************
 *  @file      dwgio/preview.hpp
 *  @brief     A record of the preview AutoCAD shows before it opens a file.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_PREVIEW_HPP
#define DWGIO_PREVIEW_HPP

#include "dwgio/api.hpp"

#include <cstddef>
#include <vector>

namespace DwgIO
{
/**
 *  One record of the preview: the header data, the bitmap or the metafile AutoCAD shows before it opens a file.
 */
struct DWGIO_API PreviewImage
{
  int Code{}; /**< 1 for the header data, 2 for a BMP, 3 for a WMF. */
  std::vector<std::byte> Bytes{};

  auto operator==(const PreviewImage &) const -> bool = default;
};
} // namespace DwgIO

#endif /* DWGIO_PREVIEW_HPP */
