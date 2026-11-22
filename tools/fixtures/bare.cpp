/***********************************************************************************************************************
 *  @file      tools/fixtures/bare.cpp
 *  @brief     Writes a drawing without its tables' content, the input of the -bare resaves in tools/acad.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/drawing.hpp"
#include "dwgio/entities/table.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/save.hpp"

#include <cstdint>
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <iostream>
#include <unordered_set>
#include <vector>

namespace
{
/**
 *  Takes away each table's extension dictionary and every object under it, so that AutoCAD's save of the drawing shows
 *  the content it makes from a table's grid alone.
 */
void RemoveTableContent(DwgIO::Drawing &drawing)
{
  std::unordered_set<std::uint64_t> removed;
  for (DwgIO::Object &object : drawing.Objects)
  {
    if (auto *table = DwgIO::GetIf<DwgIO::Entities::AcadTable>(&object); table != nullptr && table->Base.Dictionary)
    {
      removed.insert(table->Base.Dictionary.Value);
      table->Base.Dictionary = {};
    }
  }
  for (std::size_t before = 0; before != removed.size();)
  {
    before = removed.size();
    for (const DwgIO::Object &object : drawing.Objects)
    {
      if (removed.contains(DwgIO::BaseOf(object).Owner.Value))
      {
        removed.insert(DwgIO::BaseOf(object).Self.Value);
      }
    }
  }
  std::erase_if(drawing.Objects,
                [&removed](const DwgIO::Object &object) { return removed.contains(DwgIO::BaseOf(object).Self.Value); });
}
} // namespace

auto main(int count, char **arguments) -> int
{
  const std::vector<std::filesystem::path> paths(arguments + 1, arguments + count);
  if (paths.size() != 2)
  {
    std::cerr << "usage: dwgio_bare <drawing.dwg> <bare.dwg>\n";
    return EXIT_FAILURE;
  }
  std::expected<DwgIO::Loaded, DwgIO::Failure> loaded = DwgIO::LoadFile(paths[0]);
  if (!loaded)
  {
    std::cerr << paths[0].string() << ": " << loaded.error().What << '\n';
    return EXIT_FAILURE;
  }
  RemoveTableContent(loaded->Drawing);
  const auto saved = DwgIO::SaveFile(loaded->Drawing, paths[1]);
  if (!saved)
  {
    std::cerr << paths[1].string() << ": " << saved.error().What << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
