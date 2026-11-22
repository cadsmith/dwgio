/***********************************************************************************************************************
 *  @file      source/file/sections/dependencies.cpp
 *  @brief     The file dependencies section, chapter 17.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/sections/dependencies.hpp"

#include "file/sections/text.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <utility>

namespace DwgIO::File::Sections
{
namespace
{
/**
 *  A file's texts, in the section's order.
 */
constexpr std::array Texts{&FileDependency::FullFilename, &FileDependency::FoundPath, &FileDependency::FingerprintGuid,
                           &FileDependency::VersionGuid};

/**
 *  A file's longs before the short for the graphics, in the section's order.
 */
constexpr std::array Longs{&FileDependency::FeatureIndex, &FileDependency::Timestamp, &FileDependency::Filesize};
} // namespace

auto ReadDependencies(std::span<const std::byte> bytes, Version release, const Encoding::Scheme &text)
    -> std::optional<FileDependencies>
{
  TextReader in{bytes, release, text};
  FileDependencies list;
  // Each count read while the bytes last, so that none grows a list past them.
  const std::uint32_t features = in.Long();
  for (std::uint32_t at = 0; at < features && !in.HasFailed(); ++at)
  {
    list.Features.push_back(in.Text(TextCount::Sized));
  }
  const std::uint32_t files = in.Long();
  for (std::uint32_t at = 0; at < files && !in.HasFailed(); ++at)
  {
    FileDependency file;
    for (const auto member : Texts)
    {
      file.*member = in.Text(TextCount::Sized);
    }
    for (const auto member : Longs)
    {
      file.*member = static_cast<std::int32_t>(in.Long());
    }
    file.AffectsGraphics = in.Short() != 0;
    file.ReferenceCount = static_cast<std::int32_t>(in.Long());
    list.Files.push_back(std::move(file));
  }
  // Typed only when it writes back as it was: each text as written, the graphics 0 or 1, nothing after the files.
  if (in.HasFailed() || !std::ranges::equal(WriteDependencies(list, release, text), bytes))
  {
    return std::nullopt;
  }
  return list;
}

auto WriteDependencies(const FileDependencies &list, Version release, const Encoding::Scheme &text)
    -> std::vector<std::byte>
{
  TextWriter out{release, text};
  out.Long(static_cast<std::uint32_t>(list.Features.size()));
  for (const std::string &feature : list.Features)
  {
    out.Text(feature, TextCount::Sized);
  }
  out.Long(static_cast<std::uint32_t>(list.Files.size()));
  for (const FileDependency &file : list.Files)
  {
    for (const auto member : Texts)
    {
      out.Text(file.*member, TextCount::Sized);
    }
    for (const auto member : Longs)
    {
      out.Long(static_cast<std::uint32_t>(file.*member));
    }
    out.Short(file.AffectsGraphics ? 1 : 0);
    out.Long(static_cast<std::uint32_t>(file.ReferenceCount));
  }
  return out.Take();
}
} // namespace DwgIO::File::Sections
