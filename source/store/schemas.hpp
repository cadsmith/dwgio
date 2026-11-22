/***********************************************************************************************************************
 *  @file      source/store/schemas.hpp
 *  @brief     The schema data and index AutoCAD's first save writes.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_STORE_SCHEMAS_HPP
#define DWGIO_STORE_SCHEMAS_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

namespace DwgIO::Store
{
/**
 *  What a segment holds after its header, and where its system data lies, in sixteens from the segment's start.
 */
struct Contents
{
  std::vector<std::byte> Bytes{}; /**< The bytes after the header, unpadded.                                        */
  std::uint32_t System{};         /**< The offset of the names that close it, in sixteens from the segment's start. */
};

/**
 *  The schema data AutoCAD's first save writes: eight unknown properties, the schemas of the thumbnail, the ACIS bodies
 *  and the four of AutoCAD's own, each with its properties, then the properties' names. The schemas' indexes are
 *  AutoCAD's: 0 the thumbnail, 1 the bodies.
 */
[[nodiscard]] auto SchemaData() -> Contents;

/**
 *  The schema index that finds them: where each schema and unknown property lies in the schema data, which is segment
 *  5, then the schemas' names.
 */
[[nodiscard]] auto SchemaIndex() -> Contents;

/**
 *  The index of the schema AutoCAD keeps bodies under.
 */
constexpr std::uint32_t BodySchema = 1;
constexpr std::uint32_t SchemaDataSegment = 5;
} // namespace DwgIO::Store

#endif /* DWGIO_STORE_SCHEMAS_HPP */
