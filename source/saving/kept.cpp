/***********************************************************************************************************************
 *  @file      source/saving/kept.cpp
 *  @brief     The sections a drawing keeps as they were, and which of them a save writes back.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "saving/kept.hpp"

#include "file/names.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/version.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>

namespace DwgIO::Saving
{
auto Shares(Version from, Version to) -> bool
{
  return from == to || (from >= Version::R2013 && to >= Version::R2013);
}

auto Alike(std::string_view name, Version from, Version to) -> bool
{
  if (name == File::Names::Measurement)
  {
    return (from >= Version::R2007) == (to >= Version::R2007);
  }
  if (name == File::Names::Auxiliary)
  {
    return (from >= Version::R2018) == (to >= Version::R2018);
  }
  return true;
}

auto IsLost(std::string_view name) -> bool
{
  return name.starts_with("AcDb:") && name != File::Names::DataStore && !std::ranges::contains(Remade, name);
}

auto KeptSections::Has(std::string_view name) const -> bool
{
  return std::ranges::any_of(*m_Blocks, [name](const Section &block) -> bool { return block.Name == name; });
}

auto KeptSections::Next(std::string_view name) -> const Section *
{
  std::size_t skip = m_Given[std::string{name}]++;
  for (const Section &block : *m_Blocks)
  {
    if (block.Name == name && skip-- == 0)
    {
      return &block;
    }
  }
  return nullptr;
}
} // namespace DwgIO::Saving
