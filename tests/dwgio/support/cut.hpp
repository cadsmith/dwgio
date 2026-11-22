/***********************************************************************************************************************
 *  @file      tests/dwgio/support/cut.hpp
 *  @brief     An object cut at every byte, to count the reads that note it was cut short.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TESTS_SUPPORT_CUT_HPP
#define DWGIO_TESTS_SUPPORT_CUT_HPP

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "fields/context.hpp"
#include "objects/read.hpp"
#include "objects/write.hpp"
#include "support/reader.hpp"

#include "dwgio/issue.hpp"
#include "dwgio/object.hpp"
#include "dwgio/version.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <variant>
#include <vector>

namespace DwgIO::Objects
{
/**
 *  The object written as the release and cut at every byte in turn, its handles moved to the cut: how many cuts read
 *  back as the kind with a note that the data was cut short.
 */
template <typename KindT>
auto CutShort(const Object &object, Version release) -> std::size_t
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = release, .Text = &Latin, .Issues = &issues};
  const FileContext reading{.Release = release, .Text = &Latin, .Classes = {}, .Issues = &issues};
  const std::vector<std::byte> whole = WriteObject(object, with);
  Bits::Reader sized{whole};
  const auto size = static_cast<std::size_t>(sized.MS());
  const std::size_t start = sized.Position() / 8;
  std::size_t cuts = 0;
  for (std::size_t cut = 6; cut < size; ++cut)
  {
    Bits::Reader data{std::span{whole}.subspan(start, size)};
    Bits::Writer out;
    out.MS(cut);
    for (int bit = 0; bit < 10; ++bit) // The type, a bitshort of one byte.
    {
      out.B(data.B());
    }
    static_cast<void>(data.RL());
    out.RL(static_cast<std::uint32_t>(cut * 8 - 8));
    while (out.Bytes().size() < start + cut)
    {
      out.B(data.B());
    }
    out.RS(0);
    issues.clear();
    const std::optional<Object> read = ReadObject(out.Bytes(), 0, reading);
    if (read && Holds<KindT>(*read) &&
        std::ranges::any_of(issues, [](const Issue &one) { return one.What.contains("cut short"); }))
    {
      ++cuts;
    }
  }
  return cuts;
}
} // namespace DwgIO::Objects

#endif /* DWGIO_TESTS_SUPPORT_CUT_HPP */
