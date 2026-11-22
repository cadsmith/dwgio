/***********************************************************************************************************************
 *  @file      source/releases/passage.hpp
 *  @brief     What a translation to another release goes by, and the release boundaries it crosses.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_PASSAGE_HPP
#define DWGIO_RELEASES_PASSAGE_HPP

#include "releases/lookup.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/version.hpp"

#include <string>
#include <vector>

namespace DwgIO::Releases
{
/**
 *  What a translation of an object or of the header goes by: the drawing, where it notes what the release written
 *  cannot keep, and that release.
 */
struct Passage
{
  const DwgIO::Drawing *Drawing;
  const Releases::Lookup *Lookup;
  std::vector<Issue> *Issues;
  std::string What;
  Version To{}; /**< The release the save writes. */
};

/**
 *  The release boundaries a save crosses, each upward or downward.
 */
struct Crossings
{
  bool R2000Up{};
  bool R2000Down{};
  bool R2004Up{};
  bool R2004Down{};
  bool R2007Up{};
  bool R2007Down{};
  bool R2010Up{};
  bool R2010Down{};
  bool R2013Up{};
  bool R2013Down{};
  bool R2018Up{};
  bool R2018Down{};
};

/**
 *  Notes what the release written cannot keep.
 */
void Note(const Passage &passage, Handle self, std::string what);

[[nodiscard]] auto CrossesUp(Version from, Version to, Version boundary) -> bool;

[[nodiscard]] auto CrossesDown(Version from, Version to, Version boundary) -> bool;

[[nodiscard]] auto CrossingsOf(Version from, Version to) -> Crossings;
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_PASSAGE_HPP */
