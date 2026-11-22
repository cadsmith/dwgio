/***********************************************************************************************************************
 *  @file      source/store/read.cpp
 *  @brief     The reader of a data store's records.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "store/read.hpp"

#include "bits/reader.hpp"
#include "store/entry.hpp"
#include "store/layout.hpp"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace DwgIO::Store
{
namespace
{
/**
 *  The places in the head of the segment index's offset, the segment count and the indexes of the schema index and the
 *  data index.
 */
constexpr std::size_t SegmentsAt = 0x18;
constexpr std::size_t CountAt = 0x20;
constexpr std::size_t SchemaIndexAt = 0x24;
constexpr std::size_t DataIndexAt = 0x28;
/**
 *  The places in a segment's header of the offsets of its system and object data, in sixteens.
 */
constexpr std::size_t SystemAt = 0x20;
constexpr std::size_t ObjectAt = 0x24;
/**
 *  A segment index entry: an offset of eight bytes and a size of four; a data index entry: three longs.
 */
constexpr std::size_t SegmentEntry = 12;
constexpr std::size_t IndexEntry = 12;
/**
 *  In a blob reference, where its page count and pages lie; in a blob segment, where its page's size and data lie.
 */
constexpr std::size_t PagesAt = 8;
constexpr std::size_t ListAt = 32;
constexpr std::size_t PageSizeAt = 24;
constexpr std::size_t PageAt = 32;
constexpr unsigned Shift = 4;
constexpr std::size_t ByteBits = 8;

/**
 *  Reads the store through a reader that fails, and gives zeros, past its end.
 */
class StoreReader
{
public:
  explicit StoreReader(std::span<const std::byte> store) : m_In(store) {}

  auto Entries() -> std::expected<std::vector<Entry>, std::string>
  {
    if (Long(0) != Signature)
    {
      return std::unexpected("data store head not read");
    }
    const std::size_t segments = Long(SegmentsAt);
    const std::uint32_t count = Long(CountAt);
    for (std::uint32_t one = 0; one < count && !m_In.HasFailed(); ++one)
    {
      m_Offsets.push_back(static_cast<std::size_t>(Wide(segments + HeaderSize + (one * SegmentEntry))));
    }
    Names(Segment(Long(SchemaIndexAt)));
    const std::size_t index = Segment(Long(DataIndexAt));
    const std::uint32_t records = Long(index + HeaderSize);
    std::vector<Entry> entries;
    for (std::uint32_t at = 0; at < records && !m_In.HasFailed(); ++at)
    {
      const std::size_t place = index + HeaderSize + 8 + (at * IndexEntry);
      const std::uint32_t segment = Long(place);
      const std::uint32_t local = Long(place + 4);
      const std::uint32_t schema = Long(place + 8);
      if (segment != 0) // A stub, which the text says to pass over, has none.
      {
        Entry entry = Record(Segment(segment), local);
        entry.Schema = schema < m_Names.size() ? m_Names[schema] : std::string{};
        if (!m_In.HasFailed())
        {
          entries.push_back(std::move(entry));
        }
      }
    }
    if (m_In.HasFailed())
    {
      return std::unexpected(std::format("data store not read past {} records", entries.size()));
    }
    return entries;
  }

private:
  auto Long(std::size_t at) -> std::uint32_t
  {
    m_In.Seek(at * ByteBits);
    return m_In.RL();
  }

  auto Wide(std::size_t at) -> std::uint64_t
  {
    m_In.Seek(at * ByteBits);
    return m_In.RLL();
  }

  auto Text(std::size_t at, std::size_t size) -> std::string
  {
    m_In.Seek(at * ByteBits);
    std::string text;
    for (const std::byte one : m_In.Bytes(size))
    {
      text.push_back(static_cast<char>(one));
    }
    return text;
  }

  /**
   *  The offset of a segment by its index, failing for one the store lacks.
   */
  auto Segment(std::uint32_t index) -> std::size_t
  {
    if (index == 0 || index >= m_Offsets.size())
    {
      m_In.Seek(m_In.Size() + 1);
      return 0;
    }
    return m_Offsets[index];
  }

  /**
   *  The schemas' names, at the schema index's system data: a count, then names each ended by a zero.
   */
  void Names(std::size_t segment)
  {
    const std::size_t at = segment + (static_cast<std::size_t>(Long(segment + SystemAt)) << Shift);
    const std::uint32_t count = Long(at);
    for (std::uint32_t one = 0; one < count && !m_In.HasFailed(); ++one)
    {
      std::string name;
      for (std::uint8_t letter = m_In.RC(); letter != 0; letter = m_In.RC()) // Zero past the end.
      {
        name.push_back(static_cast<char>(letter));
      }
      m_Names.push_back(std::move(name));
    }
  }

  /**
   *  A record: its header in its data segment, then its data there, or in the blob segments of its pages.
   */
  auto Record(std::size_t segment, std::uint32_t local) -> Entry
  {
    const std::size_t header = segment + HeaderSize + local;
    Entry entry{.Handle = Wide(header + 8), .Schema = {}, .Data = {}, .Ranges = {}};
    const std::size_t at = segment + (static_cast<std::size_t>(Long(segment + ObjectAt)) << Shift) + Long(header + 16);
    const std::uint32_t size = Long(at);
    if (size != BlobMark)
    {
      entry.Data = Text(at + 4, size);
      entry.Ranges.push_back(Range{.Offset = at + 4, .Size = size});
      return entry;
    }
    const std::uint32_t pages = Long(at + 4 + PagesAt);
    for (std::uint32_t page = 0; page < pages && !m_In.HasFailed(); ++page)
    {
      const std::size_t blob = Segment(Long(at + 4 + ListAt + (std::size_t{page} * 8))) + HeaderSize;
      const auto length = static_cast<std::size_t>(Wide(blob + PageSizeAt));
      // Each page is a segment of the store's own, so a record's data never holds more than the store: one that
      // does names a page again, which would grow it past any size.
      if (length > (m_In.Size() / ByteBits) - entry.Data.size())
      {
        m_In.Seek(m_In.Size() + 1);
        break;
      }
      entry.Data += Text(blob + PageAt, length);
      entry.Ranges.push_back(Range{.Offset = blob + PageAt, .Size = length});
    }
    return entry;
  }

  Bits::Reader m_In;
  std::vector<std::size_t> m_Offsets{};
  std::vector<std::string> m_Names{};
};
} // namespace

auto Read(std::span<const std::byte> store) -> std::expected<std::vector<Entry>, std::string>
{
  return StoreReader{store}.Entries();
}
} // namespace DwgIO::Store
