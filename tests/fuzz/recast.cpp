/***********************************************************************************************************************
 *  @file      tests/fuzz/recast.cpp
 *  @brief     Fuzzes recasting own bits between releases, and the sections read from the same bytes.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "encoding/codepage.hpp"
#include "encoding/encoding.hpp"
#include "file/sections/dependencies.hpp"
#include "file/sections/summary.hpp"
#include "releases/untyped.hpp"

#include "dwgio/unknown.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace
{
constexpr std::array<std::string_view, 30> Classes{"ACDB_BLKREFOBJECTCONTEXTDATA_CLASS",
                                                   "ACSH_BOX_CLASS",
                                                   "DIMASSOC",
                                                   "BLOCKGRIPLOCATIONCOMPONENT",
                                                   "BLOCKALIGNMENTGRIP",
                                                   "BLOCKFLIPGRIP",
                                                   "BLOCKLOOKUPGRIP",
                                                   "BLOCKALIGNMENTPARAMETER",
                                                   "BLOCKBASEPOINTPARAMETER",
                                                   "BLOCKFLIPPARAMETER",
                                                   "BLOCKLINEARPARAMETER",
                                                   "BLOCKLOOKUPPARAMETER",
                                                   "BLOCKPOINTPARAMETER",
                                                   "BLOCKPOLARPARAMETER",
                                                   "BLOCKROTATIONPARAMETER",
                                                   "BLOCKVISIBILITYPARAMETER",
                                                   "BLOCKXYPARAMETER",
                                                   "BLOCKARRAYACTION",
                                                   "BLOCKFLIPACTION",
                                                   "BLOCKLOOKUPACTION",
                                                   "BLOCKMOVEACTION",
                                                   "BLOCKPOLARSTRETCHACTION",
                                                   "BLOCKROTATEACTION",
                                                   "BLOCKSCALEACTION",
                                                   "BLOCKSTRETCHACTION",
                                                   "ACDBDETAILVIEWSTYLE",
                                                   "ACDBSECTIONVIEWSTYLE",
                                                   "ACAD_EVALUATION_GRAPH",
                                                   "SECTION_MANAGER",
                                                   "ACDB_MTEXTATTRIBUTEOBJECTCONTEXTDATA_CLASS"};
} // namespace

/**
 *  Byte 0: class; byte 1: from and to; byte 2: how the rest splits into data and strings.
 */
extern "C" auto LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size) -> int
{
  using namespace DwgIO;
  if (size < 3)
  {
    return 0;
  }
  const auto from = static_cast<Version>(data[1] % 7);
  const auto to = static_cast<Version>((data[1] / 7) % 7);
  const std::span<const std::byte> rest{reinterpret_cast<const std::byte *>(data + 3), size - 3};
  const std::size_t split = rest.size() * data[2] / 256;
  Unknown unknown;
  unknown.Class = Symbol{Classes[data[0] % Classes.size()]};
  unknown.Body = Opaque{
      .Release = from,
      .Data = BitString{.Bytes = {rest.begin(), rest.first(split).end()}, .Size = split * 8},
      .Handles = {},
      .Strings = BitString{.Bytes = {rest.subspan(split).begin(), rest.end()}, .Size = (rest.size() - split) * 8}};
  const Encoding::Scheme scheme = Encoding::MakeScheme(Encoding::Standard());
  std::vector<Content> taken;
  const std::vector<Content> given{Content{std::string{"x"}}, Content{std::int64_t{3}}};
  if (const auto own = Releases::Carry(
          unknown, Releases::CarryContext{.From = from, .To = to, .Text = &scheme, .Given = given, .Taken = &taken}))
  {
    Unknown back = unknown;
    back.Body = own;
    static_cast<void>(Releases::Carry(
        back, Releases::CarryContext{.From = to, .To = from, .Text = &scheme, .Given = taken, .Taken = nullptr}));
  }
  // The file's own sections from the same bytes.
  if (const auto summary = File::Sections::ReadSummary(rest, from, scheme))
  {
    static_cast<void>(File::Sections::WriteSummary(*summary, to, scheme));
  }
  if (const auto list = File::Sections::ReadDependencies(rest, from, scheme))
  {
    static_cast<void>(File::Sections::WriteDependencies(*list, to, scheme));
  }
  return 0;
}
