/***********************************************************************************************************************
 *  @file      dwgio/load.hpp
 *  @brief     Reading a DWG file of any release from R14 to R2018.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_LOAD_HPP
#define DWGIO_LOAD_HPP

#include "dwgio/api.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/issue.hpp"

#include <cstddef>
#include <expected>
#include <filesystem>
#include <iosfwd>
#include <span>
#include <vector>

namespace DwgIO
{
struct DWGIO_API Loaded
{
  DwgIO::Drawing Drawing{};
  /**
   *  What the reader kept, mended or left out, each with its byte offset when it has one.
   */
  std::vector<Issue> Issues{};
};

/**
 *  Reads a DWG file of any release from R14 to R2018.
 *
 *  Anything the reader can skip or repair becomes an issue, and a file cut short keeps the objects before the cut.
 *
 *  @return A failure on another release, on bytes that are no DWG, or on missing header variables or classes.
 */
[[nodiscard]] DWGIO_API auto Load(std::span<const std::byte> bytes) -> std::expected<Loaded, Failure>;

/**
 *  Reads to the stream's end.
 *
 *  @return A failure also on a stream that has already failed, such as a file that did not open.
 */
[[nodiscard]] DWGIO_API auto Load(std::istream &from) -> std::expected<Loaded, Failure>;

[[nodiscard]] DWGIO_API auto LoadFile(const std::filesystem::path &file) -> std::expected<Loaded, Failure>;
} // namespace DwgIO

#endif /* DWGIO_LOAD_HPP */
