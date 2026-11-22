/***********************************************************************************************************************
 *  @file      tests/dwgio/support/comparable.hpp
 *  @brief     Which R14 objects say the same as their R2000 twin, AutoCAD's own remaking aside.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TESTS_SUPPORT_COMPARABLE_HPP
#define DWGIO_TESTS_SUPPORT_COMPARABLE_HPP

#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/dictionaryvar.hpp"
#include "dwgio/objects/layout.hpp"

#include <cstddef>
#include <variant>

namespace DwgIO::Objects
{
/**
 *  Whether an R14 object's twin in the R2000 file says the same thing: an extension dictionary is remade at each save,
 *  a dictionary with other entries is one AutoCAD fills at each save, one whose names differ in case alone keeps R14's
 *  upper case, and one owned hard in one release alone is the plot style names.
 */
inline auto Comparable(const Object &older, const Object &newer) -> bool
{
  if (BaseOf(older).Extended.size() != BaseOf(newer).Extended.size() ||
      BaseOf(older).Reactors.size() != BaseOf(newer).Reactors.size() ||
      BaseOf(older).Dictionary != BaseOf(newer).Dictionary)
  {
    return false;
  }
  if (const auto *variable = GetIf<DictionaryVar>(&older);
      variable != nullptr && variable->Value != Get<DictionaryVar>(newer).Value)
  {
    return false;
  }
  if (const auto *layout = GetIf<Layout>(&older); layout != nullptr && layout->Name != Get<Layout>(newer).Name)
  {
    return false;
  }
  const auto *one = GetIf<Dictionary>(&older);
  const auto *other = GetIf<Dictionary>(&newer);
  if (one != nullptr && other != nullptr)
  {
    if (one->Entries.size() != other->Entries.size() || one->HardOwner != other->HardOwner)
    {
      return false;
    }
    for (std::size_t at = 0; at < one->Entries.size(); ++at)
    {
      if (one->Entries[at] != other->Entries[at])
      {
        return false;
      }
    }
  }
  return true;
}

} // namespace DwgIO::Objects

#endif /* DWGIO_TESTS_SUPPORT_COMPARABLE_HPP */
