/***********************************************************************************************************************
 *  @file      tests/dwgio/save/stream.cpp
 *  @brief     Saving to a stream or a file, and the limits of what a field holds.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "support/fixtures.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/header.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/save.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <expected>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace DwgIO
{
TEST_CASE("save/stream: a drawing saves to a stream and to a file")
{
  const Drawing drawing = LoadDrawing("foundation-r2000.dwg");
  const std::expected<Saved, Failure> saved = Save(drawing);
  REQUIRE(saved);
  std::ostringstream stream;
  const auto streamed = Save(drawing, stream);
  REQUIRE(streamed);
  CHECK(streamed->empty());
  CHECK(stream.str() == StringOf(saved->Bytes));
  const std::filesystem::path path = std::filesystem::temp_directory_path() / "dwgio-save-test.dwg";
  const auto written = SaveFile(drawing, path);
  REQUIRE(written);
  std::ifstream back{path, std::ios::binary};
  const std::string read{std::istreambuf_iterator<char>{back}, std::istreambuf_iterator<char>{}};
  back.close();
  CHECK(read == StringOf(saved->Bytes));
  std::filesystem::remove(path);
  CHECK_FALSE(SaveFile(drawing, "/nonexistent/dir/none.dwg"));
}

TEST_CASE("save/stream: a stream that takes the bytes succeeds and one that fails is reported")
{
  const Drawing newer = LoadDrawing("foundation-r2000.dwg");
  std::ostringstream stream;
  REQUIRE(Save(newer, stream, SaveOptions{.Release = Version::R2007}));
  const std::string streamed = stream.str();
  CHECK(Load(std::as_bytes(std::span{streamed})).value().Drawing.Release == Version::R2007);
  std::ostringstream broken;
  broken.setstate(std::ios::badbit);
  CHECK(Save(newer, broken).error().What == "stream not writable");
}

TEST_CASE("save/stream: a text longer than a bitshort can count is cut and noted")
{
  Drawing drawing = LoadDrawing("foundation-r2000.dwg");
  drawing.Header.Hyperlinkbase = std::string(70000, 'a');
  const std::expected<Saved, Failure> saved = Save(drawing);
  REQUIRE(saved);
  REQUIRE(saved->Issues.size() == 1);
  CHECK(saved->Issues.front().What == "text cut to 32766 code units");
  const std::expected<Loaded, Failure> loaded = Load(saved->Bytes);
  REQUIRE(loaded);
  CHECK(loaded->Issues.empty());
  CHECK(loaded->Drawing.Header.Hyperlinkbase.size() == 32766);
  CHECK(loaded->Drawing.Objects == drawing.Objects);
}

TEST_CASE("save/stream: a short outside what a bitshort holds is clamped and noted")
{
  Drawing drawing = LoadDrawing("foundation-r2000.dwg");
  drawing.Header.Isolines = 70000;
  const std::expected<Saved, Failure> saved = Save(drawing);
  REQUIRE(saved);
  REQUIRE(saved->Issues.size() == 1);
  CHECK(saved->Issues.front().What == "short 70000 out of range, 32767 written");
  const std::expected<Loaded, Failure> loaded = Load(saved->Bytes);
  REQUIRE(loaded);
  CHECK(loaded->Issues.empty());
  CHECK(loaded->Drawing.Header.Isolines == 32767);
}
} // namespace DwgIO
