/***********************************************************************************************************************
 *  @file      source/saving/kept.hpp
 *  @brief     The sections a drawing keeps as they were, and which of them a save writes back.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_SAVING_KEPT_HPP
#define DWGIO_SAVING_KEPT_HPP

#include "file/names.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/version.hpp"

#include <array>
#include <cstddef>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace DwgIO::Saving
{
/**
 *  The sections a save to another release silently leaves out: the application information, which a save makes again as
 *  AutoCAD's does, its history, which a release before R2004 has no place for as AutoCAD's saves keep none, and the
 *  revision history and the free space, which dwgio makes.
 */
constexpr std::array<std::string_view, 4> Remade{File::Names::AppInfo, File::Names::AppInfoHistory,
                                                 File::Names::RevHistory, File::Names::ObjFreeSpace};

/**
 *  Whether a drawing's data store goes into a file of another release: only one R2013 and R2018 both read.
 */
[[nodiscard]] auto Shares(Version from, Version to) -> bool;

/**
 *  Whether a kept block has the same shape in both releases: the template's description is counted from R2007, the
 *  auxiliary header takes more shorts in R2018.
 */
[[nodiscard]] auto Alike(std::string_view name, Version from, Version to) -> bool;

/**
 *  Whether a section left out by a save to another release loses the drawing's own data, which a save notes: the
 *  project of macros, a summary or file dependencies dwgio could not type, and any other section but the data store,
 *  noted by its objects.
 */
[[nodiscard]] auto IsLost(std::string_view name) -> bool;

/**
 *  The blocks a drawing keeps as they were, handed out by name in order, since a name may come more than once.
 */
class KeptSections
{
public:
  explicit KeptSections(const std::vector<Section> &blocks) : m_Blocks(&blocks) {}

  [[nodiscard]] auto Has(std::string_view name) const -> bool;
  /**
   *  The next block of that name not handed out yet; null when none is left.
   */
  auto Next(std::string_view name) -> const Section *;

private:
  const std::vector<Section> *m_Blocks;
  std::map<std::string, std::size_t> m_Given;
};
} // namespace DwgIO::Saving

#endif /* DWGIO_SAVING_KEPT_HPP */
