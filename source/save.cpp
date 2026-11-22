/***********************************************************************************************************************
 *  @file      source/save.cpp
 *  @brief     Writing a drawing as a DWG file of any release.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/save.hpp"

#include "classes/catalog.hpp"
#include "encoding/encoding.hpp"
#include "fields/context.hpp"
#include "file/names.hpp"
#include "releases/appid.hpp"
#include "releases/header.hpp"
#include "releases/mtext.hpp"
#include "releases/names.hpp"
#include "releases/omission.hpp"
#include "releases/remake.hpp"
#include "releases/roundtrip.hpp"
#include "releases/store.hpp"
#include "releases/summary.hpp"
#include "releases/table/convert.hpp"
#include "saving/assembly.hpp"
#include "saving/files.hpp"
#include "saving/kept.hpp"
#include "saving/order.hpp"

#include "dwgio/class.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/header.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/version.hpp"

#include <algorithm>
#include <cstddef>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace DwgIO
{
namespace
{
/**
 *  The maintenance version a file is stamped with: the drawing's in its own release, else the one AutoCAD 2026 stamps a
 *  file of the release written with.
 */
auto MaintenanceOf(const Drawing &drawing, Version release) -> int
{
  if (drawing.Release == release)
  {
    return drawing.Maintenance;
  }
  return MaintenanceOf(release);
}

auto WriteDrawing(const Drawing &drawing, Version release) -> Saved
{
  Saved saved;
  const Encoding::Scheme scheme = Encoding::SchemeOf(drawing.Codepage);
  // The classes written: the drawing's, then the ones its objects are stored under in this release that it lacks.
  std::vector<Class> classes = drawing.Classes;
  std::vector<Class> needed = Classes::CollectNeeded(drawing, release);
  classes.insert(classes.end(), needed.begin(), needed.end());
  const bool paged = release >= Version::R2004;
  if (paged)
  {
    Classes::FillCounts(classes, drawing); // AutoCAD refuses a paged file whose classes carry no version.
  }
  Releases::HeaderTranslation header = Releases::ConvertHeader(drawing.Header, drawing, release);
  saved.Issues.insert(saved.Issues.end(), header.Issues.begin(), header.Issues.end());
  Saving::Assembly writing{
      .Drawing = &drawing,
      .Paged = paged,
      .Sections = {},
      .PreviewAddress = 0,
      .Header = &header.Header,
      .Release = release,
      .Maintenance = MaintenanceOf(drawing, release),
      .With = Fields::Context{.Release = release, .Text = &scheme, .Issues = &saved.Issues, .Classes = &classes},
      .Head = {},
      .Blocks = {},
      .Entries = {},
      .Cursor = 0,
      .Kept = Saving::KeptSections{drawing.Kept},
      .LeftOut = Releases::CollectLeftOut(drawing, release),
      .Stores = false,
      .Store = {}};
  std::vector<std::string> order =
      paged ? Saving::PagedOrder(drawing, saved.Issues) : Saving::LocatorOrder(drawing, saved.Issues);
  writing.Store = Releases::MakeStore(drawing, release, writing.LeftOut);
  if (writing.Store)
  {
    std::erase(order, File::Names::DataStore);
    order.insert(Saving::FindName(order, File::Names::RevHistory),
                 std::string{File::Names::DataStore}); // Where AutoCAD's files have it.
  }
  writing.Stores = writing.Store.has_value() ||
                   (Saving::Shares(drawing.Release, release) && std::ranges::contains(order, File::Names::DataStore) &&
                    writing.Kept.Has(File::Names::DataStore));
  std::vector<std::byte> file;
  if (release == Version::R2007)
  {
    file = Saving::WriteCoded(writing, drawing, order);
  }
  else
  {
    file = paged ? Saving::WritePaged(writing, drawing, order) : Saving::WriteLocator(writing, drawing, order);
  }
  saved.Bytes.assign(file.begin(), file.end());
  return saved;
}
} // namespace

auto Save(const Drawing &drawing, const SaveOptions &with) -> std::expected<Saved, Failure>
{
  const Version release = with.Release.value_or(drawing.Release);
  // One copy, made by the first pass that changes the drawing: a large drawing is held at most twice.
  Releases::Remake remade{drawing};
  // R14's names come back whole first, as AutoCAD reads them, for the passes after to find them so.
  Releases::WidenNames(remade, release);
  // A copy that registers the applications the release needs, and holds in records what its styles keep there.
  Releases::RegisterApplications(remade, release);
  // Below R2013 a body in the data store is written in the object.
  Releases::TakeOutOfStore(remade, release);
  // Tables in the release's form, as AutoCAD reads and writes them.
  Releases::Table::ConvertTables(remade, release);
  // An MTEXT AutoCAD split into columns below R2018 joined again.
  Releases::JoinColumns(remade, release);
  Releases::KeepRoundTrips(remade, release);
  // Before R2004 the summary is a record of the named objects dictionary.
  Releases::MoveSummary(remade, release);
  // Last, the names R14 cannot hold, those of the dictionaries made above among them.
  Releases::NarrowNames(remade, release);
  return WriteDrawing(remade.Now(), release);
}

auto Save(const Drawing &drawing, std::ostream &to, const SaveOptions &with)
    -> std::expected<std::vector<Issue>, Failure>
{
  return Save(drawing, with)
      .and_then(
          [&to](Saved saved) -> std::expected<std::vector<Issue>, Failure>
          {
            std::ranges::transform(saved.Bytes, std::ostreambuf_iterator<char>{to},
                                   [](std::byte one) -> char { return static_cast<char>(one); });
            if (!to)
            {
              return std::unexpected(Failure{.Code = FailureCode::Io, .What = "stream not writable"});
            }
            return std::move(saved.Issues);
          });
}

auto SaveFile(const Drawing &drawing, const std::filesystem::path &file, const SaveOptions &with)
    -> std::expected<std::vector<Issue>, Failure>
{
  std::ofstream stream{file, std::ios::binary};
  if (!stream)
  {
    return std::unexpected(Failure{.Code = FailureCode::Io, .What = std::format("cannot write {}", file.string())});
  }
  return Save(drawing, stream, with);
}
} // namespace DwgIO
