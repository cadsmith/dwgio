/***********************************************************************************************************************
 *  @file      dwgio/save.hpp
 *  @brief     Writing a drawing as a DWG file of any release from R14 to R2018.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_SAVE_HPP
#define DWGIO_SAVE_HPP

#include "dwgio/api.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/version.hpp"

#include <cstddef>
#include <expected>
#include <filesystem>
#include <iosfwd>
#include <optional>
#include <vector>

namespace DwgIO
{
struct DWGIO_API SaveOptions
{
  /**
   *  The release to write, any from R14 to R2018; empty writes the drawing's own.
   *
   *  A save to another release writes what AutoCAD's own save to it writes: an object the library does not type crosses
   *  where AutoCAD writes its class the same way in both releases, and what the release cannot hold is left out, each
   *  with an issue.
   */
  std::optional<Version> Release{};
};

struct DWGIO_API Saved
{
  std::vector<std::byte> Bytes{};
  std::vector<Issue> Issues{}; /**< What the writer clamped, moved or left out. */
};

/**
 *  Writes the drawing as the bytes of a DWG file.
 */
[[nodiscard]] DWGIO_API auto Save(const Drawing &drawing, const SaveOptions &with = {})
    -> std::expected<Saved, Failure>;

[[nodiscard]] DWGIO_API auto Save(const Drawing &drawing, std::ostream &to, const SaveOptions &with = {})
    -> std::expected<std::vector<Issue>, Failure>;

/**
 *  Replaces the file when it exists.
 */
[[nodiscard]] DWGIO_API auto SaveFile(const Drawing &drawing, const std::filesystem::path &file,
                                      const SaveOptions &with = {}) -> std::expected<std::vector<Issue>, Failure>;
} // namespace DwgIO

#endif /* DWGIO_SAVE_HPP */
