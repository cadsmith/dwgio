/***********************************************************************************************************************
 *  @file      source/saving/assembly.cpp
 *  @brief     The file a save puts together, section by section.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "saving/assembly.hpp"

#include "bits/byte.hpp"
#include "classes/layout.hpp"
#include "file/locator/head.hpp"
#include "file/locator/preview.hpp"
#include "file/locator/second.hpp"
#include "file/locator/section.hpp"
#include "file/locator/sentinels.hpp"
#include "file/locator/write.hpp"
#include "file/names.hpp"
#include "file/paged/essential.hpp"
#include "file/paged/head.hpp"
#include "file/paged/layout.hpp"
#include "file/paged/names.hpp"
#include "file/sections/dependencies.hpp"
#include "file/sections/information.hpp"
#include "file/sections/summary.hpp"
#include "objects/map.hpp"
#include "objects/write.hpp"
#include "releases/bridge.hpp"
#include "releases/lookup.hpp"
#include "releases/object.hpp"
#include "releases/omission.hpp"
#include "releases/store.hpp"
#include "saving/kept.hpp"
#include "variables/layout.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/header.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/object.hpp"
#include "dwgio/version.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace DwgIO::Saving
{
namespace
{
constexpr std::size_t Roots = 14;

constexpr std::size_t JunkBytes = 8;

constexpr std::size_t ConstantBytes = 13;

/**
 *  The handles the second header repeats, in its numbering.
 */
auto RootsOf(const Header &header) -> std::array<Handle, Roots>
{
  return {header.Handseed,
          header.BlockControlObject,
          header.LayerControlObject,
          header.StyleControlObject,
          header.LTypeControlObject,
          header.ViewControlObject,
          header.UcsControlObject,
          header.VPortControlObject,
          header.AppIdControlObject,
          header.DimStyleControlObject,
          header.ViewportEntityHeaderControlObject,
          header.DictionaryNamedObjects,
          header.DictionaryAcadMLineStyle,
          header.DictionaryAcadGroup};
}

/**
 *  The objects mark as a long, its low byte first.
 */
auto MarkBytes() -> std::vector<std::byte>
{
  std::vector<std::byte> out;
  for (unsigned shift = 0; shift < 32; shift += Bits::Byte)
  {
    out.push_back(static_cast<std::byte>((File::Paged::ObjectsMark >> shift) & 0xFFU));
  }
  return out;
}
} // namespace

auto Assembly::WriteObjects() -> std::vector<std::byte>
{
  std::vector<std::byte> bytes;
  if (Paged)
  {
    bytes = MarkBytes();
  }
  std::size_t unstored = 0;
  const Releases::Lookup found{*Drawing};
  for (const Object &object : Drawing->Objects)
  {
    const Handle self = BaseOf(object).Self;
    if (std::ranges::binary_search(LeftOut, self.Value, {}, &Handle::Value))
    {
      With.Issues->push_back(
          Issue{.Code = IssueCode::NotKept, .Subject = self, .What = Releases::Refusal(object, *Drawing, Release)});
      continue;
    }
    // An object of another release is written as this one holds it.
    Releases::Translation translated = Releases::ConvertObject(object, found, Release);
    With.Issues->insert(With.Issues->end(), translated.Issues.begin(), translated.Issues.end());
    Releases::ClearPointers(translated.Object, LeftOut, *With.Issues);
    Releases::Bridge(translated.Object, *Drawing,
                     Releases::Omission{.To = Release, .LeftOut = LeftOut, .Lookup = &found});
    bool &stored = BaseOf(translated.Object).Stored;
    unstored += !Stores && stored ? 1U : 0U;
    stored = stored && Stores;
    // Every class a kind needs was declared with the drawing's, so the writer has something for each.
    const std::vector<std::byte> written = Objects::WriteObject(translated.Object, With);
    Entries.push_back(Objects::Mapped{.Self = self, .Offset = Cursor + bytes.size()});
    bytes.insert(bytes.end(), written.begin(), written.end());
  }
  if (unstored > 0)
  {
    With.Issues->push_back(Issue{.Code = IssueCode::NotKept,
                                 .What = std::format("data store left out, the data of {} {} in it lost", unstored,
                                                     unstored == 1 ? "object" : "objects")});
  }
  return bytes;
}

auto Assembly::WriteSecond(const Section *kept) const -> std::vector<std::byte>
{
  File::Locator::Second second;
  std::vector<Issue> scratch;
  if (kept != nullptr)
  {
    second = File::Locator::ReadSecond(kept->Bytes, 0, scratch).value_or(second);
  }
  if (second.Constants.size() != ConstantBytes)
  {
    second.Constants.assign(ConstantBytes, std::byte{0});
    second.Junk.assign(JunkBytes, std::byte{0});
  }
  second.Location = static_cast<std::uint32_t>(Cursor);
  second.Version = std::string{AcadverOf(Release)};
  second.Records = Head.Records;
  for (File::Locator::Record &record : second.Records)
  {
    if (record.Number == Measurement)
    {
      record = File::Locator::Record{.Number = Measurement, .Seeker = 0, .Size = 0};
    }
  }
  const std::array<Handle, Roots> roots = RootsOf(*Header);
  second.Handles.clear();
  for (std::size_t number = 0; number < Roots; ++number)
  {
    second.Handles.push_back(
        File::Locator::Named{.Number = static_cast<std::uint8_t>(number), .Value = roots.at(number)});
  }
  return File::Locator::WriteSecond(second);
}

auto Assembly::FindKept(std::string_view name) -> const Section *
{
  const Section *kept = Kept.Next(name);
  if (name == File::Names::Signature)
  {
    // It signs bytes no save keeps; AutoCAD's saves leave it out too.
    if (kept != nullptr)
    {
      With.Issues->push_back(Issue{.Code = IssueCode::NotKept,
                                   .What = "section AcDb:Signature left out, it signs bytes a save does not keep"});
    }
    return nullptr;
  }
  if (name == File::Names::AppInfo)
  {
    return nullptr; // Every save names the program that made it, as AutoCAD's own name AutoCAD.
  }
  if (Drawing->Release == Release || (name == File::Names::DataStore && Shares(Drawing->Release, Release)))
  {
    return kept;
  }
  const bool shaped = name == File::Names::Auxiliary || name == File::Names::Measurement || name.starts_with("AcDb:");
  if (Paged && shaped && Drawing->Release >= Version::R2004)
  {
    if (name == File::Names::AppInfoHistory)
    {
      return kept;
    }
    if (kept != nullptr && IsLost(name))
    {
      With.Issues->push_back(Issue{.Code = IssueCode::NotKept,
                                   .What = std::format("section {} left out, its {} bytes cannot be written as {}",
                                                       name, NameOf(Drawing->Release), NameOf(Release))});
    }
    return nullptr;
  }
  return Alike(name, Drawing->Release, Release) ? kept : nullptr;
}

auto Assembly::Build(std::string_view name) -> std::optional<std::vector<std::byte>>
{
  if (std::optional<std::vector<std::byte>> typed = WriteTyped(name))
  {
    return typed;
  }
  const Section *kept = FindKept(name);
  if (name == File::Names::DataStore && kept != nullptr && Drawing->Release == Version::R2018 &&
      Release == Version::R2013)
  {
    return Releases::LowerStore(kept->Bytes, *With.Issues); // AutoCAD writes each body as R2013 holds it.
  }
  if (Paged && kept == nullptr)
  {
    if (std::optional<std::vector<std::byte>> made = WriteRequired(name))
    {
      return made;
    }
  }
  if (name == File::Names::Auxiliary && Release >= Version::R2000 && kept == nullptr && Kept.Has(name))
  {
    return File::Paged::WriteAuxHeader(*Header, Release,
                                       Maintenance); // The drawing's, of a shape this release has not.
  }
  if (name == File::Names::Auxiliary || name == File::Names::Unknown)
  {
    return kept == nullptr || (name == File::Names::Auxiliary && Release < Version::R2000) ? std::nullopt
                                                                                           : std::optional{kept->Bytes};
  }
  if (name == File::Names::Preview)
  {
    return Drawing->Preview.empty()
               ? std::nullopt
               : std::optional{File::Locator::WritePreview(Drawing->Preview, Paged ? PreviewAddress : Cursor)};
  }
  if (std::optional<std::vector<std::byte>> fixed = WriteFixed(name, kept))
  {
    return fixed;
  }
  // A section dwgio does not type, written back as it was in a paged file; a locator file has no place for it.
  return Paged && kept != nullptr ? std::optional{kept->Bytes} : std::nullopt;
}

auto Assembly::WriteTyped(std::string_view name) const -> std::optional<std::vector<std::byte>>
{
  if (name == File::Names::DataStore && Store)
  {
    return Store;
  }
  if (Paged && name == File::Names::SummaryInfo && Drawing->SummaryInfo)
  {
    return File::Sections::WriteSummary(*Drawing->SummaryInfo, Release, *With.Text);
  }
  if (Paged && name == File::Names::FileDepList && Drawing->FileDependencies)
  {
    return File::Sections::WriteDependencies(*Drawing->FileDependencies, Release, *With.Text);
  }
  return std::nullopt;
}

auto Assembly::WriteFixed(std::string_view name, const Section *kept) -> std::optional<std::vector<std::byte>>
{
  if (name == File::Names::Header)
  {
    return File::Locator::Wrap(Variables::Write(*Header, With), MarksOf(File::Locator::Sentinels::Header));
  }
  if (name == File::Names::Classes)
  {
    return WriteClasses();
  }
  if (name == File::Names::Pad)
  {
    return kept == nullptr ? MarkBytes() : kept->Bytes;
  }
  if (name == File::Names::Objects)
  {
    return WriteObjects();
  }
  if (name == File::Names::Map)
  {
    return Objects::WriteMap(Entries);
  }
  if (name == File::Names::Second)
  {
    return WriteSecond(kept);
  }
  if (name == File::Names::Measurement)
  {
    // Alike in either container. AutoCAD's R2004 saves may describe a template of no pages, read as no bytes,
    // and it refuses a file whose template section is empty: one is made instead.
    return kept == nullptr || kept->Bytes.empty() ? File::Paged::WriteTemplate(Release) : kept->Bytes;
  }
  return std::nullopt;
}

auto Assembly::WriteRequired(std::string_view name) const -> std::optional<std::vector<std::byte>>
{
  if (name == File::Names::RevHistory)
  {
    return File::Paged::WriteRevHistory();
  }
  if (name == File::Names::ObjFreeSpace)
  {
    return File::Paged::WriteObjFreeSpace(*Header, Release, Drawing->Objects.size() - LeftOut.size()); // Those written.
  }
  if (name == File::Names::Auxiliary)
  {
    return File::Paged::WriteAuxHeader(*Header, Release, Maintenance);
  }
  if (name == File::Names::AppInfo)
  {
    return File::Sections::WriteInformation();
  }
  return std::nullopt;
}

auto Assembly::MarksOf(const File::Locator::Marks &marks) const -> File::Locator::Marks
{
  return File::Paged::IsWide(Release, Maintenance) ? File::Locator::Widen(marks) : marks;
}

auto Assembly::WriteClasses() const -> std::vector<std::byte>
{
  std::vector<std::byte> classes =
      File::Locator::Wrap(Classes::Write(*With.Classes, With), MarksOf(File::Locator::Sentinels::Classes));
  if (Paged)
  {
    classes.resize(classes.size() + Classes::Trailer, std::byte{0});
  }
  return classes;
}

void Assembly::Place(std::string_view name)
{
  if (Paged)
  {
    Cursor = 0; // Each section's offsets count from its own start.
  }
  std::optional<std::vector<std::byte>> bytes = Build(name);
  if (!bytes)
  {
    return;
  }
  if (Paged)
  {
    const File::Paged::Known *known = File::Paged::Find(File::Paged::AcdbOf(name));
    Sections.push_back(File::Paged::Section{.Name = std::string{name},
                                            .Bytes = std::move(*bytes),
                                            .Compressed = known == nullptr || known->Compressed,
                                            .PageSize = known == nullptr ? File::Paged::UsualPage
                                                                         : File::Paged::PageOf(*known, Release),
                                            .Id = known == nullptr ? 0 : known->Id});
    return;
  }
  const std::size_t size = bytes->size();
  Blocks.push_back(File::Locator::Located{.Name = std::string{name}, .Offset = Cursor, .Bytes = std::move(*bytes)});
  Cursor += size;
  Head = File::Locator::PointHead(Head, Blocks);
}
} // namespace DwgIO::Saving
