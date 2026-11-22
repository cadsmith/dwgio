/***********************************************************************************************************************
 *  @file      source/classes/classes.cpp
 *  @brief     The reader and writer of the classes section.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/byte.hpp"
#include "bits/reader.hpp"
#include "bits/strings.hpp"
#include "bits/writer.hpp"
#include "classes/layout.hpp"

#include "dwgio/version.hpp"

#include <algorithm>
#include <cstdint>
#include <expected>
#include <format>
#include <optional>
#include <string>

namespace DwgIO::Classes
{
namespace
{
using Bits::Byte;

/**
 *  The bits of the RL that opens the classes from R2007.
 */
constexpr std::size_t LeadBits = 32;
} // namespace

auto Read(std::span<const std::byte> body, const Fields::Context &with) -> std::vector<Class>
{
  std::vector<Class> classes;
  Bits::Reader reader{body};
  Fields::Input in{.In = &reader, .With = with};
  const auto layout = Layout();
  // From R2007 the data opens with the bit it ends at, the strings in a stream of their own before it.
  std::size_t end = reader.Size();
  std::optional<Bits::Reader> strings;
  if (with.Release >= Version::R2007)
  {
    end = std::min<std::size_t>(reader.RL(), reader.Size());
    const std::expected<std::optional<Bits::Strung>, std::string> found = Bits::FindStrings(body, end);
    if (!found)
    {
      Note(in, IssueCode::Damaged, std::format("classes {}", found.error()));
    }
    const std::optional<Bits::Strung> strung = found.value_or(std::nullopt);
    strings = Bits::MakeStringReader(body, strung);
    in.Strings = strings ? &*strings : nullptr;
    if (strung)
    {
      end = strung->Start;
    }
    else if (end > 0)
    {
      --end;
    }
  }
  if (with.Release >= Version::R2004)
  {
    // The greatest class number, two zero bytes and a true bit open the classes from R2004.
    static_cast<void>(reader.BS());
    static_cast<void>(reader.RC());
    static_cast<void>(reader.RC());
    static_cast<void>(reader.B());
  }
  while (reader.Position() < end && end - reader.Position() >= Byte)
  {
    Class one;
    if (!Fields::Read(layout, one, in))
    {
      Note(in, IssueCode::Truncated, "classes cut short");
      break;
    }
    classes.push_back(std::move(one));
  }
  return classes;
}

auto Write(std::span<const Class> classes, const Fields::Context &with) -> std::vector<std::byte>
{
  Bits::Writer writer;
  Bits::Writer strings;
  Fields::Output to{.To = &writer, .Strings = with.Release >= Version::R2007 ? &strings : nullptr, .With = with};
  const auto layout = Layout();
  if (with.Release >= Version::R2004)
  {
    int greatest = 0;
    for (const Class &one : classes)
    {
      greatest = std::max(greatest, one.Number);
    }
    writer.BS(greatest);
    writer.RC(0);
    writer.RC(0);
    writer.B(true);
  }
  for (const Class &one : classes)
  {
    static_cast<void>(Fields::Write(layout, one, to));
  }
  if (with.Release >= Version::R2007)
  {
    writer.Append(Bits::WriteStringTail(strings));
    Bits::Writer whole;
    whole.RL(static_cast<std::uint32_t>(LeadBits + writer.Position()));
    whole.Append(writer);
    whole.Fill();
    return whole.Take();
  }
  writer.Fill();
  return writer.Take();
}
} // namespace DwgIO::Classes
