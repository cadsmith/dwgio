/***********************************************************************************************************************
 *  @file      source/saving/assembly.hpp
 *  @brief     The file a save puts together, section by section.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_SAVING_ASSEMBLY_HPP
#define DWGIO_SAVING_ASSEMBLY_HPP

#include "fields/context.hpp"
#include "file/locator/head.hpp"
#include "file/locator/layout.hpp"
#include "file/locator/sentinels.hpp"
#include "file/paged/layout.hpp"
#include "objects/map.hpp"
#include "saving/kept.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/header.hpp"
#include "dwgio/version.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace DwgIO::Saving
{
/**
 *  The numbers of the locator records of the measurement and the auxiliary header.
 */
constexpr std::uint8_t Measurement = 4;
constexpr std::uint8_t Auxiliary = 5;

/**
 *  What a save writes a file from, and the sections or blocks put together so far.
 */
struct Assembly
{
  const DwgIO::Drawing *Drawing{};
  bool Paged{}; /**< Whether the file is paged: its Sections take the blocks' place. */
  std::vector<File::Paged::Section> Sections{};
  std::uint32_t PreviewAddress{}; /**< The file address of the preview's data, which its records count from. */
  const DwgIO::Header *Header{};  /**< The header as the release written holds it.                           */
  Version Release{};
  /**
   *  The maintenance the file is stamped with: the drawing's in its own release, else AutoCAD 2026's.
   */
  int Maintenance{};
  Fields::Context With{};
  File::Locator::Head Head{};
  std::vector<File::Locator::Located> Blocks{};
  std::vector<Objects::Mapped> Entries{};
  std::size_t Cursor{};
  KeptSections Kept;
  std::vector<Handle> LeftOut{}; /**< The objects left out, which no pointer may name. */
  /**
   *  Whether the drawing's data store is written: an object's bit for data in it is cleared when it is not.
   */
  bool Stores{};
  /**
   *  The data store a save from below R2013 makes for the bodies it writes, when there are any.
   */
  std::optional<std::vector<std::byte>> Store{};

  /**
   *  The objects section: each object as this release holds it, those left out noted; their places go to Entries.
   */
  auto WriteObjects() -> std::vector<std::byte>;
  /**
   *  The second header, pointing at the blocks placed so far.
   */
  auto WriteSecond(const Section *kept) const -> std::vector<std::byte>;
  /**
   *  A kept block written as it was: always in the drawing's own release; in another paged release only those whose
   *  shape does not change, the data store between R2013 and R2018 among them; else only one whose shape is alike in
   *  both releases. The others are made again or left out.
   */
  [[nodiscard]] auto FindKept(std::string_view name) -> const Section *;
  /**
   *  A section's bytes, typed, kept, made or none.
   */
  auto Build(std::string_view name) -> std::optional<std::vector<std::byte>>;
  /**
   *  A section of a paged file written from what dwgio types of the drawing: the data store made for the bodies
   *  written, the summary and the file dependencies in the release's form; none for any other name, or one the drawing
   *  lacks.
   */
  [[nodiscard]] auto WriteTyped(std::string_view name) const -> std::optional<std::vector<std::byte>>;
  /**
   *  A block every file has, from the drawing: the header, the classes, the pad, the objects, the map, the second
   *  header and the measurement; none for any other name.
   */
  [[nodiscard]] auto WriteFixed(std::string_view name, const Section *kept) -> std::optional<std::vector<std::byte>>;
  /**
   *  A section AutoCAD refuses a paged file without, made for a drawing that has none: the revision history, the object
   *  free space and the auxiliary header; and the application information, which every save makes, naming dwgio as
   *  AutoCAD's own names AutoCAD; none for any other name.
   */
  [[nodiscard]] auto WriteRequired(std::string_view name) const -> std::optional<std::vector<std::byte>>;
  /**
   *  The marks of the header and the classes, with the second RL from R2010.
   */
  [[nodiscard]] auto MarksOf(const File::Locator::Marks &marks) const -> File::Locator::Marks;
  /**
   *  The classes section: the frame, and in a paged file the eight bytes after it.
   */
  [[nodiscard]] auto WriteClasses() const -> std::vector<std::byte>;
  /**
   *  Builds a section and puts it in the file: among the sections of a paged file, else after the blocks placed.
   */
  void Place(std::string_view name);
};
} // namespace DwgIO::Saving

#endif /* DWGIO_SAVING_ASSEMBLY_HPP */
