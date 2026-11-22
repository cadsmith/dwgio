/***********************************************************************************************************************
 *  @file      source/store/layout.hpp
 *  @brief     The constants of the data store's layout, chapter 24.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_STORE_LAYOUT_HPP
#define DWGIO_STORE_LAYOUT_HPP

#include <cstddef>
#include <cstdint>
#include <string_view>

/**
 *  The data store of chapter 24, `AcDb:AcDsPrototype_1b`, where AutoCAD keeps an ACIS entity's body from R2013: a head,
 *  then segments, each a header and its data padded to 0x80 bytes, as AutoCAD's first save of a drawing lays them out.
 */
namespace DwgIO::Store
{
/**
 *  The head: its signature, `jard`, and its size.
 */
constexpr std::uint32_t Signature = 0x6472616A;
constexpr std::size_t HeadSize = 0x80;
/**
 *  The longs after the size that AutoCAD's heads hold: 8 and the version 2.
 */
constexpr std::uint32_t HeadUnknown = 8;
constexpr std::uint32_t HeadVersion = 2;

/**
 *  A segment's header: the signature, the name of six characters, then eight longs and eight bytes of 0x55.
 */
constexpr std::uint16_t SegmentSignature = 0xD5AC;
constexpr std::size_t HeaderSize = 0x30;
constexpr std::size_t NameSize = 6;
constexpr std::uint8_t HeaderFill = 0x55;
/**
 *  What a segment is padded with to its size, a multiple of 0x80.
 */
constexpr std::uint8_t Fill = 0x70;
constexpr std::size_t Alignment = 0x80;
/**
 *  What pads a data segment's record headers to its data, and the schema data's names, to sixteen bytes.
 */
constexpr std::uint8_t RecordFill = 0x62;
constexpr std::uint8_t NameFill = 0x73;
constexpr std::size_t Paragraph = 0x10;
/**
 *  The revision of every segment AutoCAD's first save writes.
 */
constexpr std::uint32_t Revision = 1;

namespace Names
{
constexpr std::string_view Segments = "segidx";
constexpr std::string_view Index = "datidx";
constexpr std::string_view Data = "_data_";
constexpr std::string_view SchemaIndex = "schidx";
constexpr std::string_view SchemaData = "schdat";
constexpr std::string_view Search = "search";
constexpr std::string_view Blob = "blob01";
} // namespace Names

/**
 *  A data record's header: its size, a long of 1, the handle and the record's offset.
 */
constexpr std::uint32_t RecordHeaderSize = 20;
constexpr std::uint32_t RecordUnknown = 1;
/**
 *  What a record's size is in place of one when its data lies in blob segments, and the size of what follows it.
 */
constexpr std::uint32_t BlobMark = 0xBB106BB1;
constexpr std::uint32_t BlobReferenceSize = 40;
/**
 *  The most data a record holds itself, and the most a blob segment holds.
 */
constexpr std::size_t MostInline = 0x40000;
constexpr std::size_t MostPage = 0xFFFB0;
/**
 *  What a blob segment's header gives as its size, and the long before it.
 */
constexpr std::uint32_t BlobHeaderSize = 0x30;
constexpr std::uint32_t BlobUnknown = 1;

/**
 *  The schema AutoCAD keeps an ACIS entity's body under.
 */
constexpr std::string_view Bodies = "AcDb3DSolid_ASM_Data";
} // namespace DwgIO::Store

#endif /* DWGIO_STORE_LAYOUT_HPP */
