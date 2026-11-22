/***********************************************************************************************************************
 *  @file      dwgio/dependencies.hpp
 *  @brief     The files a drawing depends on, its AcDb:FileDepList section.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_DEPENDENCIES_HPP
#define DWGIO_DEPENDENCIES_HPP

#include "dwgio/api.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace DwgIO
{
/**
 *  One file the drawing depends on; its feature and full file name are its key.
 */
struct DWGIO_API FileDependency
{
  std::string FullFilename{}; /**< As the drawing names it. */
  std::string FoundPath{};
  std::string FingerprintGuid{}; /**< Set for an external reference only. */
  std::string VersionGuid{};     /**< Set for an external reference only. */
  std::int32_t FeatureIndex{};   /**< Into the list's Features.           */
  std::int32_t Timestamp{};      /**< Seconds since 1 January 1980.       */
  std::int32_t Filesize{};
  bool AffectsGraphics{};
  std::int32_t ReferenceCount{};

  auto operator==(const FileDependency &) const -> bool = default;
};

/**
 *  The files the drawing depends on, its AcDb:FileDepList, from R2004: fonts, images, plot configurations and external
 *  references.
 */
struct DWGIO_API FileDependencies
{
  std::vector<std::string> Features{}; /**< Such as Acad:XRef, Acad:Image, Acad:PlotConfig and Acad:Text. */
  std::vector<FileDependency> Files{};

  auto operator==(const FileDependencies &) const -> bool = default;
};
} // namespace DwgIO

#endif /* DWGIO_DEPENDENCIES_HPP */
