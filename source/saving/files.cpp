/***********************************************************************************************************************
 *  @file      source/saving/files.cpp
 *  @brief     The file a save writes in each container: paged, coded or by locator.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "saving/files.hpp"

#include "file/coded/layout.hpp"
#include "file/locator/head.hpp"
#include "file/locator/preview.hpp"
#include "file/locator/write.hpp"
#include "file/names.hpp"
#include "file/paged/head.hpp"
#include "file/paged/layout.hpp"
#include "saving/assembly.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/version.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace DwgIO::Saving
{
namespace
{
constexpr std::uint8_t Flag = 1;

constexpr std::uint8_t DwgVersion = 0x21;

constexpr std::uint8_t MaintRelease = 0xFF;

/**
 *  What AutoCAD 2026 stamps a paged file's header with in each release besides the maintenance version: the
 *  application's maintenance at 0x12 and the three bytes at 0x15.
 */
struct Stamp
{
  std::uint8_t MaintRelease{};
  std::array<std::uint8_t, 3> Repeat{};
};

auto StampOf(Version release) -> Stamp
{
  if (release == Version::R2018)
  {
    return Stamp{.MaintRelease = 0, .Repeat = {0, 0, 0}};
  }
  return Stamp{.MaintRelease = MaintRelease, .Repeat = {0, DwgVersion, MaintRelease}};
}

constexpr std::uint8_t PagedFlag = 3;

/**
 *  A paged or coded file's sections placed in the order given, but each built after the objects when it comes before
 *  them, as AutoCAD's R2007 files can have the map: the map is made of where the objects were written.
 */
void PlaceSections(Assembly &writing, std::span<const std::string> order)
{
  const auto objects = std::ranges::find(order, File::Names::Objects);
  std::vector<std::pair<std::size_t, std::string_view>> held;
  for (auto name = order.begin(); name != order.end(); ++name)
  {
    if (*name == File::Names::Map && name < objects)
    {
      held.emplace_back(writing.Sections.size(), *name);
      continue;
    }
    writing.Place(*name);
    if (name == objects)
    {
      // Each held section back where the order has it, past those put back before it.
      for (std::size_t back = 0; back < held.size(); ++back)
      {
        writing.Place(held[back].second);
        std::ranges::rotate(writing.Sections.begin() + static_cast<std::ptrdiff_t>(held[back].first + back),
                            writing.Sections.end() - 1, writing.Sections.end());
      }
    }
  }
}
} // namespace

auto WritePaged(Assembly &writing, const Drawing &drawing, std::span<const std::string> order) -> std::vector<std::byte>
{
  File::Paged::Head head;
  head.Release = writing.Release;
  head.Maintenance = static_cast<std::uint8_t>(writing.Maintenance);
  head.Flag = PagedFlag;
  head.DwgVersion = DwgVersion;
  head.MaintRelease = StampOf(writing.Release).MaintRelease;
  head.Repeat = StampOf(writing.Release).Repeat;
  head.Codepage = static_cast<std::uint16_t>(drawing.Codepage);
  PlaceSections(writing, order);
  std::vector<std::byte> file = File::Paged::WriteLayout(head, writing.Sections);
  if (!drawing.Preview.empty()) // The first pass wrote it at address 0; the head now knows where it sits.
  {
    writing.PreviewAddress = head.Image;
    for (File::Paged::Section &section : writing.Sections)
    {
      if (section.Name == File::Names::Preview)
      {
        section.Bytes = File::Locator::WritePreview(drawing.Preview, writing.PreviewAddress);
      }
    }
    file = File::Paged::WriteLayout(head, writing.Sections);
  }
  return file;
}

auto WriteCoded(Assembly &writing, const Drawing &drawing, std::span<const std::string> order) -> std::vector<std::byte>
{
  File::Coded::Meta meta;
  meta.Release = writing.Release;
  meta.Maintenance = static_cast<std::uint8_t>(writing.Maintenance);
  meta.Flag = PagedFlag;
  meta.DwgVersion = DwgVersion;
  meta.MaintRelease = StampOf(writing.Release).MaintRelease;
  meta.Codepage = static_cast<std::uint16_t>(drawing.Codepage);
  PlaceSections(writing, order);
  std::vector<Issue> first;
  std::vector<std::byte> file = File::Coded::WriteLayout(meta, writing.Sections, first);
  if (!drawing.Preview.empty()) // The first pass wrote it at address 0; the meta now knows where it sits.
  {
    writing.PreviewAddress = meta.Image;
    for (File::Paged::Section &section : writing.Sections)
    {
      if (section.Name == File::Names::Preview)
      {
        section.Bytes = File::Locator::WritePreview(drawing.Preview, writing.PreviewAddress);
      }
    }
    first.clear();
    file = File::Coded::WriteLayout(meta, writing.Sections, first);
  }
  writing.With.Issues->insert(writing.With.Issues->end(), first.begin(), first.end());
  return file;
}

auto WriteLocator(Assembly &writing, const Drawing &drawing, std::span<const std::string> order)
    -> std::vector<std::byte>
{
  writing.Head.Release = writing.Release;
  writing.Head.Maintenance = static_cast<std::uint8_t>(writing.Maintenance);
  writing.Head.Flag = Flag;
  writing.Head.DwgVersion = DwgVersion;
  writing.Head.MaintRelease = MaintRelease;
  writing.Head.Codepage = static_cast<std::uint16_t>(drawing.Codepage);
  // A release before R2000 has no auxiliary header; one kept from an R2000 file is left out.
  const bool auxiliary = writing.Release >= Version::R2000 && std::ranges::contains(order, File::Names::Auxiliary) &&
                         writing.Kept.Has(File::Names::Auxiliary);
  for (std::uint8_t number = 0; number <= (auxiliary ? Auxiliary : Measurement); ++number)
  {
    writing.Head.Records.push_back(File::Locator::Record{.Number = number, .Seeker = 0, .Size = 0});
  }
  writing.Cursor = SizeOf(writing.Head);
  for (const std::string &name : order)
  {
    writing.Place(name);
  }
  return File::Locator::WriteLayout(writing.Head, writing.Blocks);
}
} // namespace DwgIO::Saving
