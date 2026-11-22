/***********************************************************************************************************************
 *  @file      source/tables/shapes/block.hpp
 *  @brief     How a block record lies in the file, with its inserts and entities.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TABLES_SHAPES_BLOCK_HPP
#define DWGIO_TABLES_SHAPES_BLOCK_HPP

#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/list.hpp"
#include "fields/nested.hpp"
#include "fields/since.hpp"
#include "fields/when.hpp"
#include "objects/shape.hpp"
#include "tables/shapes/record.hpp"

#include "dwgio/tables/block.hpp"
#include "dwgio/version.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace DwgIO::Tables::Shapes
{

/**
 *  One non-zero byte per insert, then a zero; the handles come later in the handle stream.
 */
inline auto ReadInsertCount(BlockRecord &record, const Fields::Input &in) -> bool
{
  record.Inserts.clear();
  for (;;)
  {
    const std::uint8_t one = in.In->RC();
    if (in.In->HasFailed() || one == 0)
    {
      return !in.In->HasFailed();
    }
    record.Inserts.emplace_back();
  }
}

inline auto WriteInsertCount(const BlockRecord &record, const Fields::Output &to) -> bool
{
  for (std::size_t at = 0; at < record.Inserts.size(); ++at)
  {
    to.To->RC(1);
  }
  to.To->RC(0);
  return true;
}

inline auto ReadPreview(BlockRecord &record, const Fields::Input &in) -> bool
{
  const std::int32_t count = std::max(in.In->BL(), 0);
  record.Preview = in.In->Bytes(static_cast<std::size_t>(count));
  return !in.In->HasFailed();
}

inline auto WritePreview(const BlockRecord &record, const Fields::Output &to) -> bool
{
  to.To->BL(static_cast<std::int32_t>(record.Preview.size()));
  to.To->Raw(record.Preview);
  return true;
}

inline auto ReadInserts(BlockRecord &record, const Fields::Input &in) -> bool
{
  return std::ranges::all_of(record.Inserts,
                             [&in](Handle &insert) -> bool { return Fields::ReadLink(insert, SoftPointer, in); });
}

inline auto WriteInserts(const BlockRecord &record, const Fields::Output &to) -> bool
{
  return std::ranges::all_of(record.Inserts,
                             [&to](const Handle insert) -> bool { return Fields::WriteLink(insert, SoftPointer, to); });
}

inline auto ReadOwnedCount(BlockRecord &record, const Fields::Input &in) -> bool
{
  std::size_t count = 0;
  if (!Fields::ReadHandleCount(in, count))
  {
    return false;
  }
  record.Entities.assign(count, Handle{});
  return true;
}

inline auto WriteOwnedCount(const BlockRecord &record, const Fields::Output &to) -> bool
{
  to.To->BL(static_cast<std::int32_t>(record.Entities.size()));
  return true;
}

inline auto ReadEntities(BlockRecord &record, const Fields::Input &in) -> bool
{
  return std::ranges::all_of(record.Entities,
                             [&in](Handle &entity) -> bool { return Fields::ReadLink(entity, HardOwner, in); });
}

inline auto WriteEntities(const BlockRecord &record, const Fields::Output &to) -> bool
{
  return std::ranges::all_of(record.Entities,
                             [&to](const Handle entity) -> bool { return Fields::WriteLink(entity, HardOwner, to); });
}

/**
 *  An external reference's entities are not in the file.
 */
inline auto IsDefined(const BlockRecord &record) -> bool
{
  return !record.Xref && !record.Overlaid;
}
} // namespace DwgIO::Tables::Shapes

namespace DwgIO::Objects
{
template <>
struct Shape<Tables::BlockRecord>
{
  static constexpr std::array<std::uint16_t, 1> Types{49};
  static constexpr std::string_view Name = "BLOCK_HEADER";

  static auto Layout()
  {
    using Tables::BlockRecord;
    using namespace Tables::Shapes;
    return Fields::Layout(
        Fields::Part(&BlockRecord::Record, RecordLayout()), Fields::Bit(&BlockRecord::Anonymous),
        Fields::Bit(&BlockRecord::Attributes), Fields::Bit(&BlockRecord::Xref), Fields::Bit(&BlockRecord::Overlaid),
        Fields::Since(Version::R2000, Fields::Bit(&BlockRecord::Loaded)),
        Fields::Since(Version::R2004, Fields::Custom(ReadOwnedCount, WriteOwnedCount)),
        Fields::Point3(&BlockRecord::Origin), Fields::Text(&BlockRecord::Path),
        Fields::Since(Version::R2000, Fields::Custom(ReadInsertCount, WriteInsertCount),
                      Fields::Text(&BlockRecord::Description), Fields::Custom(ReadPreview, WritePreview)),
        Fields::Since(Version::R2007, Fields::Short(&BlockRecord::Units), Fields::Bit(&BlockRecord::Explodable),
                      Fields::Byte(&BlockRecord::Scaling)),
        Fields::Link(&BlockRecord::Block, HardOwner),
        Fields::Until(Version::R2000, Fields::When(IsDefined, Fields::Link(&BlockRecord::First, SoftPointer),
                                                   Fields::Link(&BlockRecord::Last, SoftPointer))),
        Fields::Since(Version::R2004, Fields::Custom(ReadEntities, WriteEntities)),
        Fields::Link(&BlockRecord::EndBlk, HardOwner),
        Fields::Since(Version::R2000, Fields::Custom(ReadInserts, WriteInserts),
                      Fields::Link(&BlockRecord::Layout, HardPointer)));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_TABLES_SHAPES_BLOCK_HPP */
