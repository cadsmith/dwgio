/***********************************************************************************************************************
 *  @file      source/releases/passage.cpp
 *  @brief     What a translation to another release goes by, and the release boundaries it crosses.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/passage.hpp"

#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/version.hpp"

#include <string>
#include <utility>

namespace DwgIO::Releases
{
void Note(const Passage &passage, Handle self, std::string what)
{
  passage.Issues->push_back(Issue{.Code = IssueCode::NotKept, .Subject = self, .What = std::move(what)});
}

auto CrossesUp(Version from, Version to, Version boundary) -> bool
{
  return from < boundary && to >= boundary;
}

auto CrossesDown(Version from, Version to, Version boundary) -> bool
{
  return from >= boundary && to < boundary;
}

auto CrossingsOf(Version from, Version to) -> Crossings
{
  return Crossings{.R2000Up = CrossesUp(from, to, Version::R2000),
                   .R2000Down = CrossesDown(from, to, Version::R2000),
                   .R2004Up = CrossesUp(from, to, Version::R2004),
                   .R2004Down = CrossesDown(from, to, Version::R2004),
                   .R2007Up = CrossesUp(from, to, Version::R2007),
                   .R2007Down = CrossesDown(from, to, Version::R2007),
                   .R2010Up = CrossesUp(from, to, Version::R2010),
                   .R2010Down = CrossesDown(from, to, Version::R2010),
                   .R2013Up = CrossesUp(from, to, Version::R2013),
                   .R2013Down = CrossesDown(from, to, Version::R2013),
                   .R2018Up = CrossesUp(from, to, Version::R2018),
                   .R2018Down = CrossesDown(from, to, Version::R2018)};
}
} // namespace DwgIO::Releases
