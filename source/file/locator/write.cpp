/***********************************************************************************************************************
 *  @file      source/file/locator/write.cpp
 *  @brief     A locator file written: R14 and R2000.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/locator/write.hpp"

#include "file/names.hpp"

#include <array>
#include <string_view>

namespace DwgIO::File::Locator
{
namespace
{
struct Target
{
  std::uint8_t Number{};
  std::string_view Block;
};

constexpr std::array Targets{
    Target{.Number = 0, .Block = Names::Header},    Target{.Number = 1, .Block = Names::Classes},
    Target{.Number = 2, .Block = Names::Map},       Target{.Number = 4, .Block = Names::Measurement},
    Target{.Number = 5, .Block = Names::Auxiliary},
};

void Point(Head &head, const Located &block)
{
  for (const Target &target : Targets)
  {
    if (target.Block != block.Name)
    {
      continue;
    }
    for (Record &record : head.Records)
    {
      if (record.Number == target.Number)
      {
        record.Seeker = static_cast<std::uint32_t>(block.Offset);
        record.Size = static_cast<std::uint32_t>(block.Bytes.size());
      }
    }
  }
  if (block.Name == Names::Preview)
  {
    head.Image = static_cast<std::uint32_t>(block.Offset);
  }
}
} // namespace

auto PointHead(Head head, std::span<const Located> blocks) -> Head
{
  for (const Located &block : blocks)
  {
    Point(head, block);
  }
  return head;
}

auto WriteLayout(const Head &head, std::span<const Located> blocks) -> std::vector<std::byte>
{
  std::vector<std::byte> file = WriteHead(PointHead(head, blocks));
  for (const Located &block : blocks)
  {
    file.insert(file.end(), block.Bytes.begin(), block.Bytes.end());
  }
  return file;
}
} // namespace DwgIO::File::Locator
