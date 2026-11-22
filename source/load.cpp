/***********************************************************************************************************************
 *  @file      source/load.cpp
 *  @brief     Reading a DWG file of any release into a drawing.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/load.hpp"

#include "bits/reader.hpp"
#include "classes/layout.hpp"
#include "encoding/codepage.hpp"
#include "encoding/encoding.hpp"
#include "fields/context.hpp"
#include "file/coded/layout.hpp"
#include "file/locator/layout.hpp"
#include "file/locator/preview.hpp"
#include "file/locator/section.hpp"
#include "file/locator/sentinels.hpp"
#include "file/names.hpp"
#include "file/paged/layout.hpp"
#include "file/sections/dependencies.hpp"
#include "file/sections/summary.hpp"
#include "objects/context.hpp"
#include "objects/map.hpp"
#include "objects/read.hpp"
#include "objects/shape.hpp"
#include "objects/walk.hpp"
#include "octets.hpp"
#include "releases/summary.hpp"
#include "variables/layout.hpp"

#include <algorithm>
#include <cstddef>
#include <format>
#include <fstream>
#include <istream>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <variant>

namespace DwgIO
{
namespace
{
struct Pieces
{
  std::vector<Objects::Mapped> Map;
  std::optional<File::Locator::Located> Objects;
  bool Mapped{};
  /**
   *  The bytes the map's offsets index: the file, or a paged file's objects section.
   */
  std::span<const std::byte> Span{};
};

auto Scheme(int codepage, std::vector<Issue> &issues) -> Encoding::Scheme
{
  if (!Encoding::FindScheme(codepage))
  {
    issues.push_back(Issue{.Code = IssueCode::NotKept,
                           .Offset = 0x13,
                           .What = std::format("code page {} not carried, ANSI_1252 used", codepage)});
  }
  return Encoding::SchemeOf(codepage);
}

/**
 *  Puts each block of the layout where the drawing holds it.
 */
struct Placer
{
  Loaded *Into{};
  Pieces *Parts{};
  Fields::Context With{};

  void TypeBlock(File::Locator::Located &block) const
  {
    Drawing &drawing = Into->Drawing;
    Fields::Context here = With;
    here.Offset = block.Offset;
    if (block.Name == File::Names::Header)
    {
      drawing.Header = Variables::Read(block.Bytes, here);
    }
    else if (block.Name == File::Names::Classes)
    {
      drawing.Classes = Classes::Read(block.Bytes, here);
    }
    else
    {
      drawing.Preview = File::Locator::ReadPreview(block.Bytes, block.Offset, Into->Issues);
    }
  }

  /**
   *  Whether the block is a section dwgio types and reads back as it was: the summary or the file dependencies, put
   *  where the drawing holds them; or the application information, which names the program that saved the file and
   *  which every save makes again, as AutoCAD's own do.
   */
  [[nodiscard]] auto IsTyped(const File::Locator::Located &block) const -> bool
  {
    if (block.Name == File::Names::AppInfo)
    {
      return true;
    }
    Drawing &drawing = Into->Drawing;
    if (block.Name == File::Names::SummaryInfo)
    {
      drawing.SummaryInfo = File::Sections::ReadSummary(block.Bytes, drawing.Release, *With.Text);
      return drawing.SummaryInfo.has_value();
    }
    if (block.Name == File::Names::FileDepList)
    {
      drawing.FileDependencies = File::Sections::ReadDependencies(block.Bytes, drawing.Release, *With.Text);
      return drawing.FileDependencies.has_value();
    }
    return false;
  }

  void Place(File::Locator::Located &block) const
  {
    Drawing &drawing = Into->Drawing;
    drawing.Order.push_back(block.Name);
    if (block.Name == File::Names::Header || block.Name == File::Names::Classes || block.Name == File::Names::Preview)
    {
      TypeBlock(block);
    }
    else if (IsTyped(block))
    {
      return;
    }
    else if (block.Name == File::Names::Map)
    {
      Parts->Map = Objects::ReadMap(block.Bytes, block.Offset, Into->Issues);
      Parts->Mapped = true;
    }
    else if (block.Name == File::Names::Objects)
    {
      Parts->Objects = std::move(block);
    }
    else
    {
      drawing.Kept.push_back(Section{.Name = std::move(block.Name), .Bytes = std::move(block.Bytes)});
    }
  }
};

auto Offsets(Loaded &loaded, std::span<const std::byte> file, const Pieces &pieces) -> std::vector<std::size_t>
{
  std::vector<std::size_t> offsets;
  if (pieces.Mapped)
  {
    for (const Objects::Mapped &entry : pieces.Map)
    {
      offsets.push_back(entry.Offset);
    }
    std::ranges::sort(offsets);
    return offsets;
  }
  if (pieces.Objects)
  {
    const File::Locator::Located &block = *pieces.Objects;
    offsets = Objects::Walk(file, block.Offset, block.Offset + block.Bytes.size());
    loaded.Issues.push_back(Issue{.Code = IssueCode::Damaged,
                                  .Offset = block.Offset,
                                  .What = std::format("object map missing, {} objects walked", offsets.size())});
  }
  return offsets;
}

/**
 *  Fills an entity's owner from the header's space records when its mode names a space rather than an owner.
 */
void Own(Drawing &drawing)
{
  for (Object &object : drawing.Objects)
  {
    const Common *common = CommonOf(object);
    if (common == nullptr)
    {
      continue;
    }
    if (common->Space == Space::Model)
    {
      BaseOf(object).Owner = drawing.Header.BlockRecordModelSpace;
    }
    else if (common->Space == Space::Paper)
    {
      BaseOf(object).Owner = drawing.Header.BlockRecordPaperSpace;
    }
  }
}

/**
 *  A framed section's body, its frame's issues noted at their offsets in the section.
 */
auto ReadFramed(std::span<const std::byte> section, const File::Locator::Marks &marks, std::vector<Issue> &issues)
    -> std::vector<std::byte>
{
  const File::Locator::Record whole{.Number = 0, .Seeker = 0, .Size = 0};
  std::expected<File::Locator::Framed, Failure> framed = File::Locator::Cut(section, whole, marks);
  if (!framed)
  {
    issues.push_back(Issue{.Code = IssueCode::Damaged, .Offset = framed.error().Offset, .What = framed.error().What});
    return {};
  }
  issues.insert(issues.end(), framed->Issues.begin(), framed->Issues.end());
  return std::move(framed->Body);
}

/**
 *  The objects read from their span by the map, or walked; the owners filled.
 */
void ReadObjects(Loaded &loaded, const Pieces &pieces, const Encoding::Scheme &scheme)
{
  const std::span<const std::byte> objects = pieces.Span;
  Drawing &drawing = loaded.Drawing;
  const Objects::FileContext reading{
      .Release = drawing.Release, .Text = &scheme, .Classes = drawing.Classes, .Issues = &loaded.Issues};
  for (const std::size_t offset : Offsets(loaded, objects, pieces))
  {
    if (std::optional<Object> object = Objects::ReadObject(objects, offset, reading))
    {
      drawing.Objects.emplace_back(std::move(*object));
    }
  }
  Own(drawing);
  // R14 and R2000 keep the summary in a record of the named objects dictionary, which stays among the objects.
  const Objects::XRecord *props = drawing.Release < Version::R2004 ? Releases::FindProperties(drawing) : nullptr;
  if (props != nullptr)
  {
    drawing.SummaryInfo = Releases::SummaryOf(*props);
  }
}

auto LoadLocated(std::span<const std::byte> file, Loaded &loaded) -> std::expected<Loaded, Failure>
{
  std::expected<File::Locator::Layout, Failure> layout = File::Locator::ReadLayout(file, loaded.Issues);
  if (!layout)
  {
    return std::unexpected(layout.error());
  }
  Drawing &drawing = loaded.Drawing;
  drawing.Release = layout->Head.Release;
  drawing.Codepage = layout->Head.Codepage;
  drawing.Maintenance = layout->Head.Maintenance;
  const Encoding::Scheme scheme = Scheme(drawing.Codepage, loaded.Issues);
  const Fields::Context with{.Release = drawing.Release, .Text = &scheme, .Issues = &loaded.Issues};
  Pieces pieces;
  pieces.Span = file;
  const Placer placing{.Into = &loaded, .Parts = &pieces, .With = with};
  for (File::Locator::Located &block : layout->Blocks)
  {
    placing.Place(block);
  }
  ReadObjects(loaded, pieces, scheme);
  return std::move(loaded); // Moved, not copied: a drawing may be large.
}

/**
 *  What a paged or coded file gives the loader: the file header's fields and the sections.
 */
struct Sectioned
{
  Version Release{};
  int Codepage{};
  int Maintenance{};
  std::uint32_t Image{}; /**< The file address of the preview's data, which its records count from. */
  std::vector<File::Paged::Section> Sections{};
};

auto Sections(Sectioned file, Loaded &loaded) -> std::expected<Loaded, Failure>
{
  Drawing &drawing = loaded.Drawing;
  drawing.Release = file.Release;
  drawing.Codepage = file.Codepage;
  drawing.Maintenance = file.Maintenance;
  const Encoding::Scheme scheme = Scheme(drawing.Codepage, loaded.Issues);
  const Fields::Context with{.Release = drawing.Release, .Text = &scheme, .Issues = &loaded.Issues};
  Pieces pieces;
  const Placer placing{.Into = &loaded, .Parts = &pieces, .With = with};
  const bool wide = File::Paged::IsWide(drawing.Release, drawing.Maintenance);
  const auto marks = [wide](const File::Locator::Marks &plain) -> File::Locator::Marks
  {
    return wide ? File::Locator::Widen(plain) : plain;
  };
  for (File::Paged::Section &section : file.Sections)
  {
    File::Locator::Located block{.Name = std::move(section.Name), .Offset = 0, .Bytes = std::move(section.Bytes)};
    if (block.Name == File::Names::Header)
    {
      block.Bytes = ReadFramed(block.Bytes, marks(File::Locator::Sentinels::Header), loaded.Issues);
    }
    else if (block.Name == File::Names::Classes)
    {
      const std::size_t framed = block.Bytes.size() - std::min(block.Bytes.size(), Classes::Trailer);
      block.Bytes =
          ReadFramed(std::span{block.Bytes}.first(framed), marks(File::Locator::Sentinels::Classes), loaded.Issues);
    }
    else if (block.Name == File::Names::Preview)
    {
      block.Offset = file.Image; // The images' starts are file addresses, as in the locator file.
    }
    else if (block.Name == File::Names::Objects)
    {
      Bits::Reader in{block.Bytes};
      if (in.RL() != File::Paged::ObjectsMark)
      {
        loaded.Issues.push_back(
            Issue{.Code = IssueCode::Damaged, .What = "objects section does not start with 0x0DCA"});
      }
    }
    placing.Place(block);
  }
  if (pieces.Objects)
  {
    pieces.Span = pieces.Objects->Bytes;
  }
  ReadObjects(loaded, pieces, scheme);
  return std::move(loaded); // Moved, not copied: a drawing may be large.
}

auto LoadPaged(std::span<const std::byte> file, Loaded &loaded) -> std::expected<Loaded, Failure>
{
  std::expected<File::Paged::Layout, Failure> layout = File::Paged::ReadLayout(file, loaded.Issues);
  if (!layout)
  {
    return std::unexpected(layout.error());
  }
  return Sections(Sectioned{.Release = layout->Head.Release,
                            .Codepage = layout->Head.Codepage,
                            .Maintenance = layout->Head.Maintenance,
                            .Image = layout->Head.Image,
                            .Sections = std::move(layout->Sections)},
                  loaded);
}

auto LoadCoded(std::span<const std::byte> file, Loaded &loaded) -> std::expected<Loaded, Failure>
{
  std::expected<File::Coded::Layout, Failure> layout = File::Coded::ReadLayout(file, loaded.Issues);
  if (!layout)
  {
    return std::unexpected(layout.error());
  }
  return Sections(Sectioned{.Release = layout->Meta.Release,
                            .Codepage = layout->Meta.Codepage,
                            .Maintenance = layout->Meta.Maintenance,
                            .Image = layout->Meta.Image,
                            .Sections = std::move(layout->Sections)},
                  loaded);
}
} // namespace

auto Load(std::span<const std::byte> bytes) -> std::expected<Loaded, Failure>
{
  Loaded loaded;
  const std::optional<Version> release = bytes.size() < 6 ? std::nullopt : ReleaseOf(TextOf(bytes.first(6)));
  if (release == Version::R2007)
  {
    return LoadCoded(bytes, loaded);
  }
  return release && *release >= Version::R2004 ? LoadPaged(bytes, loaded) : LoadLocated(bytes, loaded);
}

auto Load(std::istream &from) -> std::expected<Loaded, Failure>
{
  if (!from)
  {
    return std::unexpected(Failure{.Code = FailureCode::Io, .What = "stream not readable"});
  }
  const std::string bytes{std::istreambuf_iterator<char>{from}, std::istreambuf_iterator<char>{}};
  return Load(std::as_bytes(std::span{bytes}));
}

auto LoadFile(const std::filesystem::path &file) -> std::expected<Loaded, Failure>
{
  std::ifstream stream{file, std::ios::binary};
  if (!stream)
  {
    return std::unexpected(Failure{.Code = FailureCode::Io, .What = std::format("cannot open {}", file.string())});
  }
  return Load(stream);
}
} // namespace DwgIO
