/***********************************************************************************************************************
 *  @file      source/store/write.cpp
 *  @brief     The data store written, as AutoCAD's first save lays it out.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "store/write.hpp"

#include "bits/writer.hpp"
#include "store/entry.hpp"
#include "store/layout.hpp"
#include "store/schemas.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace DwgIO::Store
{
namespace
{
/**
 *  The indexes of the segments every store has, in their order; the blob segments follow, the search last.
 */
namespace Index
{
constexpr std::uint32_t Data = 2;
constexpr std::uint32_t Records = 3;
constexpr std::uint32_t SchemaIndex = 4;
constexpr std::uint32_t Blobs = 6;
} // namespace Index

/**
 *  A segment: its name, what follows its header, its offsets of system and object data in sixteens, and whether it is a
 *  blob's page.
 */
struct Segment
{
  std::string_view Name{};
  std::vector<std::byte> Bytes{};
  std::uint32_t System{};
  std::uint32_t Object{};
  bool Blob{};
};

auto PadSize(std::size_t size) -> std::size_t
{
  return (size + Alignment - 1) / Alignment * Alignment;
}

/**
 *  A segment's size: its header and data padded to 0x80.
 */
auto Size(const Segment &segment) -> std::size_t
{
  return PadSize(HeaderSize + segment.Bytes.size());
}

void Raw(Bits::Writer &out, std::string_view data)
{
  for (const char one : data)
  {
    out.RC(static_cast<std::uint8_t>(one));
  }
}

/**
 *  A record's pages, each at most a blob segment's worth.
 */
auto Pages(std::size_t size) -> std::size_t
{
  return (size + MostPage - 1) / MostPage;
}

/**
 *  The data segment: each record's header, then its data, or a reference to its pages in the blob segments, which it
 *  adds.
 */
auto Records(std::span<const Entry> bodies, std::vector<Segment> &blobs) -> Segment
{
  Bits::Writer out;
  std::vector<std::uint32_t> offsets;
  Bits::Writer data;
  for (const Entry &body : bodies)
  {
    offsets.push_back(static_cast<std::uint32_t>(data.Bytes().size()));
    if (body.Data.size() <= MostInline)
    {
      data.RL(static_cast<std::uint32_t>(body.Data.size()));
      Raw(data, body.Data);
      continue;
    }
    const std::size_t pages = Pages(body.Data.size());
    const std::size_t last = body.Data.size() - ((pages - 1) * MostPage);
    data.RL(BlobMark);
    data.RLL(body.Data.size());
    data.RL(static_cast<std::uint32_t>(pages));
    data.RL(BlobReferenceSize);
    data.RL(static_cast<std::uint32_t>(pages == 1 ? last : MostPage));
    data.RL(static_cast<std::uint32_t>(last));
    data.RL(0);
    data.RL(0);
    for (std::size_t page = 0; page < pages; ++page)
    {
      const std::string_view part = std::string_view{body.Data}.substr(page * MostPage, MostPage);
      data.RL(static_cast<std::uint32_t>(Index::Blobs + blobs.size()));
      data.RL(static_cast<std::uint32_t>(part.size()));
      Bits::Writer blob;
      blob.RLL(body.Data.size());
      blob.RLL(page * MostPage);
      blob.RL(static_cast<std::uint32_t>(page));
      blob.RL(static_cast<std::uint32_t>(pages));
      blob.RLL(part.size());
      Raw(blob, part);
      blobs.push_back(Segment{.Name = Names::Blob, .Bytes = blob.Take(), .System = 0, .Object = 0, .Blob = true});
    }
  }
  for (std::size_t at = 0; at < bodies.size(); ++at)
  {
    out.RL(RecordHeaderSize);
    out.RL(RecordUnknown);
    out.RLL(bodies[at].Handle);
    out.RL(offsets[at]);
  }
  while ((HeaderSize + out.Bytes().size()) % Paragraph != 0)
  {
    out.RC(RecordFill);
  }
  const auto object = static_cast<std::uint32_t>((HeaderSize + out.Bytes().size()) / Paragraph);
  out.Raw(data.Bytes());
  return Segment{.Name = Names::Data, .Bytes = out.Take(), .System = 0, .Object = object, .Blob = false};
}

/**
 *  The data index: each record's segment, the offset of its header there, and its schema.
 */
auto MakeIndex(std::size_t count) -> Segment
{
  Bits::Writer out;
  out.RL(static_cast<std::uint32_t>(count));
  out.RL(0);
  for (std::size_t at = 0; at < count; ++at)
  {
    out.RL(Index::Records);
    out.RL(static_cast<std::uint32_t>(at * RecordHeaderSize));
    out.RL(BodySchema);
  }
  return Segment{.Name = Names::Index, .Bytes = out.Take(), .System = 0, .Object = 0, .Blob = false};
}

/**
 *  The search data: none for the thumbnail's schema, and for the bodies' each record's index sorted, then the handles,
 *  sorted, each with its record's index. AutoCAD ignores a record its search data lacks.
 */
auto MakeSearch(std::span<const Entry> bodies) -> Segment
{
  Bits::Writer out;
  out.RL(2);
  out.RL(0);
  out.RLL(0);
  out.RL(0);
  out.RL(BodySchema);
  out.RLL(bodies.size());
  for (std::size_t at = 0; at < bodies.size(); ++at)
  {
    out.RLL(at);
  }
  out.RL(bodies.empty() ? 0 : 1);
  if (!bodies.empty())
  {
    std::vector<std::pair<std::uint64_t, std::size_t>> handles;
    handles.reserve(bodies.size());
    for (std::size_t at = 0; at < bodies.size(); ++at)
    {
      handles.emplace_back(bodies[at].Handle, at);
    }
    std::ranges::sort(handles);
    out.RL(0);
    out.RL(static_cast<std::uint32_t>(handles.size()));
    for (const auto &[handle, at] : handles)
    {
      out.RLL(handle);
      out.RLL(1);
      out.RLL(at);
    }
  }
  return Segment{.Name = Names::Search, .Bytes = out.Take(), .System = 0, .Object = 0, .Blob = false};
}

void Header(Bits::Writer &out, const Segment &segment, std::uint32_t index)
{
  out.RS(SegmentSignature);
  Raw(out, segment.Name);
  out.RL(index);
  out.RL(segment.Blob ? BlobUnknown : 0);
  out.RL(segment.Blob ? BlobHeaderSize : static_cast<std::uint32_t>(Size(segment)));
  out.RL(0);
  out.RL(Revision);
  out.RL(0);
  out.RL(segment.System);
  out.RL(segment.Object);
  for (std::size_t one = 0; one < 8; ++one)
  {
    out.RC(HeaderFill);
  }
}
} // namespace

auto Write(std::span<const Entry> bodies) -> std::vector<std::byte>
{
  std::vector<Segment> blobs;
  Segment records = Records(bodies, blobs);
  const Contents schemaIndex = SchemaIndex();
  const Contents schemaData = SchemaData();
  std::vector<Segment> segments;
  segments.push_back(Segment{.Name = Names::Segments, .Bytes = {}, .System = 0, .Object = 0, .Blob = false});
  segments.push_back(MakeIndex(bodies.size()));
  segments.push_back(std::move(records));
  segments.push_back(
      Segment{.Name = Names::SchemaIndex, .Bytes = schemaIndex.Bytes, .System = schemaIndex.System, .Object = 0});
  segments.push_back(
      Segment{.Name = Names::SchemaData, .Bytes = schemaData.Bytes, .System = schemaData.System, .Object = 0});
  for (Segment &blob : blobs)
  {
    segments.push_back(std::move(blob));
  }
  segments.push_back(MakeSearch(bodies));
  const std::size_t count = segments.size() + 1; // The first entry of the segment index is none.
  segments.front().Bytes.resize(count * 12);
  std::vector<std::size_t> offsets;
  std::size_t end = HeadSize;
  for (const Segment &segment : segments)
  {
    offsets.push_back(end);
    end += Size(segment);
  }
  Bits::Writer index;
  index.RLL(0);
  index.RL(0);
  for (std::size_t at = 0; at < segments.size(); ++at)
  {
    index.RLL(offsets[at]);
    index.RL(static_cast<std::uint32_t>(Size(segments[at])));
  }
  segments.front().Bytes = index.Take();
  Bits::Writer out;
  for (const std::uint32_t value :
       {Signature, static_cast<std::uint32_t>(HeadSize), HeadUnknown, HeadVersion, std::uint32_t{0}, Revision,
        static_cast<std::uint32_t>(HeadSize), std::uint32_t{0}, static_cast<std::uint32_t>(count), Index::SchemaIndex,
        Index::Data, static_cast<std::uint32_t>(segments.size()), std::uint32_t{0}, static_cast<std::uint32_t>(end)})
  {
    out.RL(value);
  }
  while (out.Bytes().size() < HeadSize)
  {
    out.RC(0);
  }
  for (std::size_t at = 0; at < segments.size(); ++at)
  {
    Header(out, segments[at], static_cast<std::uint32_t>(at + 1));
    out.Raw(segments[at].Bytes);
    while (out.Bytes().size() < offsets[at] + Size(segments[at]))
    {
      out.RC(Fill);
    }
  }
  return out.Take();
}
} // namespace DwgIO::Store
