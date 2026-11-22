/***********************************************************************************************************************
 *  @file      tests/dwgio/fuzz.cpp
 *  @brief     Every input the fuzzers found, run as the target that found it runs it.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/lz21.hpp"
#include "support/fixtures.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/save.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace DwgIO
{
namespace
{
/**
 *  As the load target: a drawing loaded is saved to its own release, and the save loads back.
 */
void ReplayLoad(std::span<const std::byte> bytes)
{
  const std::expected<Loaded, Failure> loaded = Load(bytes);
  if (!loaded)
  {
    return;
  }
  const std::expected<Saved, Failure> saved = Save(loaded->Drawing);
  REQUIRE(saved);
  const std::expected<Loaded, Failure> again = Load(saved->Bytes);
  REQUIRE(again);
  CHECK(again->Drawing.Objects.size() == loaded->Drawing.Objects.size());
}

/**
 *  As the cross target: byte 0 the release to save to, the rest a drawing.
 */
void ReplayCross(std::span<const std::byte> bytes)
{
  REQUIRE(bytes.size() >= 2);
  const std::expected<Loaded, Failure> loaded = Load(bytes.subspan(1));
  if (!loaded)
  {
    return;
  }
  SaveOptions options;
  options.Release = static_cast<Version>(std::to_integer<std::uint8_t>(bytes.front()) % 7);
  if (const std::expected<Saved, Failure> saved = Save(loaded->Drawing, options))
  {
    CHECK(Load(saved->Bytes));
  }
}

/**
 *  As the lz21 target: the bytes decompressed as an R2007 page, and the page compressed and decompressed back.
 */
void ReplayLz21(std::span<const std::byte> bytes)
{
  constexpr std::size_t Page = 0xf800;
  static_cast<void>(Bits::Lz21::Decompress(bytes, Page));
  const std::span<const std::byte> page = bytes.first(std::min(bytes.size(), Page));
  const std::optional<std::vector<std::byte>> back = Bits::Lz21::Decompress(Bits::Lz21::Compress(page), page.size());
  REQUIRE(back);
  CHECK(std::ranges::equal(*back, page));
}
} // namespace

TEST_CASE("fuzz: every input the fuzzers found runs through its target")
{
  const std::filesystem::path found = std::filesystem::path{DWGIO_SOURCE_DIR} / "tests" / "fuzz" / "found";
  for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator{found})
  {
    const std::string name = entry.path().filename().string();
    CAPTURE(name);
    const std::vector<std::byte> bytes = ReadBytes(entry.path().string());
    if (name.ends_with("-cross"))
    {
      ReplayCross(bytes);
    }
    else if (name.ends_with("-load"))
    {
      ReplayLoad(bytes);
    }
    else
    {
      ReplayLz21(bytes);
    }
  }
}
} // namespace DwgIO
