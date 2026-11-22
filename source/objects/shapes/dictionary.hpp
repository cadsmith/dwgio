/***********************************************************************************************************************
 *  @file      source/objects/shapes/dictionary.hpp
 *  @brief     How a DICTIONARY lies in the file, with or without a default.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_SHAPES_DICTIONARY_HPP
#define DWGIO_OBJECTS_SHAPES_DICTIONARY_HPP

#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "objects/shape.hpp"

#include "dwgio/objects/dictionary.hpp"
#include "dwgio/version.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <string_view>
#include <utility>

namespace DwgIO::Objects::Shapes
{
constexpr int SoftOwner = 2;

constexpr int HardPointer = 5;

constexpr std::string_view Defaulted = "ACDBDICTIONARYWDFLT";

/**
 *  The hard owner flag is what the specification calls an unknown R14 byte. A count beyond what is left is noted, the
 *  dictionary kept empty.
 */
inline auto ReadDictionary(Dictionary &dictionary, const Fields::Input &in) -> bool
{
  const std::int32_t count = in.In->BL();
  // The cloning flag came with R2000; a dictionary with a default, stored under its class, carries it in R14 too.
  if (in.With.Release >= Version::R2000 || dictionary.Default.has_value())
  {
    dictionary.Cloning = in.In->BS();
  }
  dictionary.HardOwner = in.In->RC() != 0;
  if (in.In->HasFailed())
  {
    return false;
  }
  const std::size_t room = (in.In->Size() - in.In->Position()) / 2;
  if (count < 0 || std::cmp_greater(count, room))
  {
    Fields::Note(in, IssueCode::Damaged, std::format("dictionary count {} out of range", count));
    return false;
  }
  dictionary.Entries.resize(static_cast<std::size_t>(count));
  for (Entry &entry : dictionary.Entries)
  {
    entry.Name = Fields::TextCode::Read(in);
  }
  for (Entry &entry : dictionary.Entries)
  {
    if (!Fields::ReadLink(entry.Object, SoftOwner, in))
    {
      return false;
    }
  }
  return !dictionary.Default || Fields::ReadLink(*dictionary.Default, HardPointer, in);
}

inline auto WriteDictionary(const Dictionary &dictionary, const Fields::Output &to) -> bool
{
  to.To->BL(static_cast<std::int32_t>(dictionary.Entries.size()));
  if (to.With.Release >= Version::R2000 || dictionary.Default.has_value())
  {
    to.To->BS(dictionary.Cloning);
  }
  to.To->RC(dictionary.HardOwner ? 1 : 0);
  for (const Entry &entry : dictionary.Entries)
  {
    Fields::TextCode::Write(entry.Name, to);
  }
  for (const Entry &entry : dictionary.Entries)
  {
    Fields::WriteLink(entry.Object, SoftOwner, to);
  }
  if (dictionary.Default)
  {
    Fields::WriteLink(*dictionary.Default, HardPointer, to);
  }
  return true;
}
} // namespace DwgIO::Objects::Shapes

namespace DwgIO::Objects
{
/**
 *  Chapters 20.4.44 and 20.4.45; a dictionary with a default is stored under the class ACDBDICTIONARYWDFLT.
 */
template <>
struct Shape<Objects::Dictionary>
{
  static constexpr std::array<std::uint16_t, 1> Types{42};
  static constexpr std::string_view Name = "DICTIONARY";
  static constexpr std::array<std::string_view, 1> Classes{Shapes::Defaulted};

  static void Open(Objects::Dictionary &dictionary, std::string_view name)
  {
    if (name == Shapes::Defaulted)
    {
      dictionary.Default = Handle{};
    }
  }

  static auto IsClassed(const Objects::Dictionary &dictionary) -> bool { return dictionary.Default.has_value(); }

  static auto NameOf(const Objects::Dictionary &dictionary) -> std::string_view
  {
    return dictionary.Default ? Shapes::Defaulted : Name;
  }

  static auto Layout() { return Fields::Layout(Fields::Custom(Shapes::ReadDictionary, Shapes::WriteDictionary)); }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_SHAPES_DICTIONARY_HPP */
