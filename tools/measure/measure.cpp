/***********************************************************************************************************************
 *  @file      tools/measure/measure.cpp
 *  @brief     Times loading and saving a drawing in each release, or building one.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/dwgio.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <format>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#if defined(__APPLE__)
#  include <malloc/malloc.h>
#endif

namespace
{
constexpr std::array Releases{DwgIO::Version::R14,   DwgIO::Version::R2000, DwgIO::Version::R2004,
                              DwgIO::Version::R2007, DwgIO::Version::R2010, DwgIO::Version::R2013,
                              DwgIO::Version::R2018};

/**
 *  NameOf in lower case, as the command line names a release.
 */
auto LowerName(DwgIO::Version release) -> std::string
{
  std::string name{DwgIO::NameOf(release)};
  std::ranges::transform(name, name.begin(), [](char letter) { return letter == 'R' ? 'r' : letter; });
  return name;
}

auto ParseRelease(std::string_view name) -> std::optional<DwgIO::Version>
{
  const auto found = std::ranges::find(Releases, name, LowerName);
  return found == Releases.end() ? std::nullopt : std::optional{*found};
}

template <typename StepT>
auto TimeStep(StepT step)
{
  const auto start = std::chrono::steady_clock::now();
  auto result = step();
  const std::chrono::duration<double> took = std::chrono::steady_clock::now() - start;
  return std::pair{std::move(result), took.count()};
}

/**
 *  Builds a new drawing of a release one addition at a time through an editor: lines into model space, then inserts
 *  each given an attribute, printing the seconds each run takes.
 */
auto MeasureBuild(DwgIO::Version release, std::size_t lines, std::size_t inserts) -> int
{
  DwgIO::Drawing drawing = DwgIO::Create(release);
  DwgIO::Edit::Editor editor{drawing};
  const auto [added, adding] = TimeStep(
      [&]
      {
        for (std::size_t at = 0; at < lines; ++at)
        {
          DwgIO::Entities::Line line;
          line.End = DwgIO::Vector{.X = static_cast<double>(at), .Y = 1, .Z = 0};
          if (!editor.Add(line))
          {
            return false;
          }
        }
        return true;
      });
  const auto block = editor.AddBlock("Tag");
  const auto [placed, placing] = TimeStep(
      [&]
      {
        for (std::size_t at = 0; block && at < inserts; ++at)
        {
          DwgIO::Entities::Insert insert;
          insert.Block = *block;
          insert.Position = DwgIO::Vector{.X = static_cast<double>(at), .Y = 0, .Z = 0};
          const auto self = editor.Add(insert);
          DwgIO::Entities::Attrib attribute;
          attribute.Tag = "NUMBER";
          if (!self || !editor.AddAttribute(*self, attribute))
          {
            return false;
          }
        }
        return block.has_value();
      });
  std::cout << std::format("{} lines {:.3f} s, {} inserts with an attribute {:.3f} s, {} objects\n", lines, adding,
                           inserts, placing, drawing.Objects.size());
  return added && placed ? 0 : 1;
}

/**
 *  Prints how to call the tool, for the exit code of a call it cannot run.
 */
auto PrintUsage() -> int
{
  std::cerr << "usage: dwgio_measure [--fuzz] drawing.dwg [--load | r14|r2000|r2004|r2007|r2010|r2013|r2018...]\n"
               "       dwgio_measure --build [r14|r2000|r2004|r2007|r2010|r2013|r2018]\n";
  return 2;
}

/**
 *  The megabytes the heap holds now, where the platform says.
 */
auto MakeHeapNote() -> std::string
{
#if defined(__APPLE__)
  malloc_statistics_t statistics{};
  malloc_zone_statistics(nullptr, &statistics);
  constexpr double Mega = 1024.0 * 1024.0;
  return std::format(", {:.0f} MB held", static_cast<double>(statistics.size_in_use) / Mega);
#else
  return {};
#endif
}
} // namespace

/**
 *  Measures how long a drawing takes to load and to save.
 *
 *  Loads the DWG file, then saves it to each release named after it (r14 to r2018), or to its own when none is named,
 *  printing the seconds of each step, the objects, the issues and the bytes. --load after the file only loads it; with
 *  --fuzz the file's first byte names the release to save to, as the cross fuzzer reads its inputs. --build, with a
 *  release or R2000, measures building a drawing instead: 100,000 lines added one at a time, then 10,000 inserts each
 *  with an attribute.
 */
auto main(int count, char **values) -> int
{
  const std::vector<std::string_view> arguments(values + 1, values + count);
  if (arguments.empty())
  {
    return PrintUsage();
  }
  if (arguments.front() == "--build")
  {
    constexpr std::size_t Lines = 100'000;
    constexpr std::size_t Inserts = 10'000;
    const std::optional<DwgIO::Version> release =
        arguments.size() > 1 ? ParseRelease(arguments[1]) : std::optional{DwgIO::Version::R2000};
    return release && arguments.size() <= 2 ? MeasureBuild(*release, Lines, Inserts) : PrintUsage();
  }
  const bool fuzz = arguments.front() == "--fuzz";
  const std::span<const std::string_view> rest = std::span{arguments}.subspan(fuzz ? 1 : 0);
  if (rest.empty())
  {
    return PrintUsage();
  }
  std::ifstream in{std::string{rest.front()}, std::ios::binary};
  if (!in)
  {
    std::cerr << std::format("cannot read {}\n", rest.front());
    return 1;
  }
  std::string bytes{std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
  const bool only = rest.size() > 1 && rest[1] == "--load";
  std::vector<std::optional<DwgIO::Version>> releases;
  if (!only && rest.size() == 1)
  {
    releases.emplace_back(std::nullopt);
  }
  if (fuzz && !bytes.empty())
  {
    if (!only)
    {
      releases = {Releases.at(static_cast<unsigned char>(bytes.front()) % Releases.size())};
    }
    bytes.erase(0, 1);
  }
  for (const std::string_view name : rest.subspan(only ? 2 : 1))
  {
    const std::optional<DwgIO::Version> release = ParseRelease(name);
    if (!release)
    {
      return PrintUsage();
    }
    releases.emplace_back(release);
  }
  const auto [loaded, loading] = TimeStep([&] { return DwgIO::Load(std::as_bytes(std::span{bytes})); });
  if (!loaded)
  {
    std::cout << std::format("load failed after {:.3f} s: {}\n", loading, loaded.error().What);
    return 1;
  }
  std::cout << std::format("load {:.3f} s, {} bytes, {} objects, {} issues{}\n", loading, bytes.size(),
                           loaded->Drawing.Objects.size(), loaded->Issues.size(), MakeHeapNote());
  for (const std::optional<DwgIO::Version> release : releases)
  {
    DwgIO::SaveOptions options;
    options.Release = release;
    const auto [saved, saving] = TimeStep([&] { return DwgIO::Save(loaded->Drawing, options); });
    const std::string name = release ? LowerName(*release) : "own";
    if (!saved)
    {
      std::cout << std::format("save {} failed after {:.3f} s: {}\n", name, saving, saved.error().What);
      continue;
    }
    std::cout << std::format("save {} {:.3f} s, {} bytes, {} issues\n", name, saving, saved->Bytes.size(),
                             saved->Issues.size());
  }
  return 0;
}
