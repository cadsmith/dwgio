/***********************************************************************************************************************
 *  @file      source/store/schemas.cpp
 *  @brief     The schema data and index AutoCAD's first save writes.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "store/schemas.hpp"

#include "bits/writer.hpp"
#include "store/layout.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace DwgIO::Store
{
namespace
{
/**
 *  The types of property AutoCAD's schemas hold: an identifier of eight bytes, a byte array, a flag of none, a byte.
 */
namespace Types
{
constexpr std::uint32_t Identifier = 10;
constexpr std::uint32_t Array = 15;
constexpr std::uint32_t Flag = 1;
constexpr std::uint32_t Byte = 7;
} // namespace Types

/**
 *  A property's flags: 8 when a long follows its type.
 */
constexpr std::uint32_t Extended = 8;

/**
 *  A property: its flags, the index of its name, its type, the long after the type when flagged, and its values.
 */
struct Property
{
  std::uint32_t Flags{};
  std::uint32_t Name{};
  std::uint32_t Type{};
  std::uint32_t Extra{};
  std::span<const std::uint64_t> Values{};
};

/**
 *  A schema: the unknown properties it names, and its properties.
 */
struct Schema
{
  std::span<const std::uint64_t> Unknowns{};
  std::span<const Property> Properties{};
};

constexpr std::array<std::uint64_t, 2> ThumbnailUnknowns{1, 3};
constexpr std::array<std::uint64_t, 2> ThumbnailIds{2, 0};
constexpr std::array<std::uint64_t, 2> BodyUnknowns{7, 5};
constexpr std::array<std::uint64_t, 2> BodyIds{6, 4};
constexpr std::array<std::uint64_t, 1> Zero{0};

constexpr std::array<Property, 2> Thumbnail{
    {{.Flags = 0, .Name = 0, .Type = Types::Identifier, .Extra = 0, .Values = ThumbnailIds},
     {.Flags = 0, .Name = 1, .Type = Types::Array, .Extra = 0, .Values = {}}}};
constexpr std::array<Property, 2> Bodies{
    {{.Flags = 0, .Name = 2, .Type = Types::Identifier, .Extra = 0, .Values = BodyIds},
     {.Flags = 0, .Name = 3, .Type = Types::Array, .Extra = 0, .Values = {}}}};
constexpr std::array<Property, 1> Treated{{{.Flags = 0, .Name = 4, .Type = Types::Flag, .Extra = 0, .Values = {}}}};
constexpr std::array<Property, 1> Legacy{{{.Flags = 0, .Name = 5, .Type = Types::Flag, .Extra = 0, .Values = {}}}};
constexpr std::array<Property, 1> Indexed{{{.Flags = 0, .Name = 6, .Type = Types::Flag, .Extra = 0, .Values = {}}}};
constexpr std::array<Property, 1> Attributed{
    {{.Flags = Extended, .Name = 7, .Type = Types::Byte, .Extra = 1, .Values = Zero}}};

constexpr std::array<Schema, 6> Schemas{{{.Unknowns = ThumbnailUnknowns, .Properties = Thumbnail},
                                         {.Unknowns = BodyUnknowns, .Properties = Bodies},
                                         {.Unknowns = {}, .Properties = Treated},
                                         {.Unknowns = {}, .Properties = Legacy},
                                         {.Unknowns = {}, .Properties = Indexed},
                                         {.Unknowns = {}, .Properties = Attributed}}};

constexpr std::array<std::string_view, 6> SchemaNames{
    "AcDb_Thumbnail_Schema", "AcDb3DSolid_ASM_Data",          "AcDbDs::TreatedAsObjectDataSchema",
    "AcDbDs::LegacySchema",  "AcDbDs::IndexedPropertySchema", "AcDbDs::HandleAttributeSchema"};

constexpr std::array<std::string_view, 8> PropertyNames{"AcDbDs::ID",
                                                        "Thumbnail_Data",
                                                        "AcDbDs::ID",
                                                        "ASM_Data",
                                                        "AcDbDs::TreatedAsObjectData",
                                                        "AcDbDs::Legacy",
                                                        "AcDs:Indexable",
                                                        "AcDbDs::HandleAttribute"};

/**
 *  The unknown properties: a size of eight and a flag each, and the schema each belongs to.
 */
constexpr std::array<std::uint32_t, 8> UnknownFlags{0, 1, 1, 1, 0, 1, 1, 1};
constexpr std::array<std::uint32_t, 8> UnknownOwners{5, 2, 4, 3, 5, 3, 4, 2};
constexpr std::uint32_t UnknownSize = 8;
/**
 *  The 64-bit value between the schema index's two lists.
 */
constexpr std::uint64_t Between = 0x0AF10C;

/**
 *  The sizes of a value of each type, by type.
 */
constexpr std::array<std::size_t, 16> TypeSizes{0, 0, 2, 1, 2, 4, 8, 1, 2, 4, 8, 4, 8, 0, 0, 0};

void WriteNames(Bits::Writer &out, std::span<const std::string_view> names)
{
  out.RL(static_cast<std::uint32_t>(names.size()));
  for (const std::string_view name : names)
  {
    for (const char one : name)
    {
      out.RC(static_cast<std::uint8_t>(one));
    }
    out.RC(0);
  }
}

/**
 *  Where each schema starts in the schema data, after the unknown properties.
 */
auto Starts() -> std::array<std::uint32_t, 6>
{
  std::array<std::uint32_t, 6> starts{};
  auto at = static_cast<std::uint32_t>(UnknownFlags.size() * 2 * 4);
  for (std::size_t schema = 0; schema < Schemas.size(); ++schema)
  {
    starts.at(schema) = at;
    at += 2 + static_cast<std::uint32_t>(Schemas.at(schema).Unknowns.size() * 8) + 2;
    for (const Property &property : Schemas.at(schema).Properties)
    {
      at += 12 + (property.Flags == Extended ? 4 : 0) + 2 +
            static_cast<std::uint32_t>(property.Values.size() * TypeSizes.at(property.Type));
    }
  }
  return starts;
}

/**
 *  The offset in sixteens, from the segment's start, of names at that place of its data.
 */
auto Sixteens(std::size_t at) -> std::uint32_t
{
  return static_cast<std::uint32_t>((HeaderSize + at) / Paragraph);
}
} // namespace

auto SchemaData() -> Contents
{
  Bits::Writer out;
  for (const std::uint32_t flag : UnknownFlags)
  {
    out.RL(UnknownSize);
    out.RL(flag);
  }
  for (const Schema &schema : Schemas)
  {
    out.RS(static_cast<std::uint16_t>(schema.Unknowns.size()));
    for (const std::uint64_t unknown : schema.Unknowns)
    {
      out.RLL(unknown);
    }
    out.RS(static_cast<std::uint16_t>(schema.Properties.size()));
    for (const Property &property : schema.Properties)
    {
      out.RL(property.Flags);
      out.RL(property.Name);
      out.RL(property.Type);
      if (property.Flags == Extended)
      {
        out.RL(property.Extra);
      }
      out.RS(static_cast<std::uint16_t>(property.Values.size()));
      for (const std::uint64_t value : property.Values)
      {
        for (std::size_t byte = 0; byte < TypeSizes.at(property.Type); ++byte)
        {
          out.RC(static_cast<std::uint8_t>(value >> (8 * byte)));
        }
      }
    }
  }
  while ((HeaderSize + out.Bytes().size()) % Paragraph != 0)
  {
    out.RC(NameFill);
  }
  const std::uint32_t system = Sixteens(out.Bytes().size());
  WriteNames(out, PropertyNames);
  return Contents{.Bytes = out.Take(), .System = system};
}

auto SchemaIndex() -> Contents
{
  Bits::Writer out;
  const std::array<std::uint32_t, 6> starts = Starts();
  out.RL(static_cast<std::uint32_t>(Schemas.size()));
  out.RL(0);
  for (std::size_t schema = 0; schema < Schemas.size(); ++schema)
  {
    out.RL(static_cast<std::uint32_t>(schema));
    out.RL(SchemaDataSegment);
    out.RL(starts.at(schema));
  }
  out.RLL(Between);
  out.RL(static_cast<std::uint32_t>(UnknownOwners.size()));
  out.RL(0);
  for (std::size_t unknown = 0; unknown < UnknownOwners.size(); ++unknown)
  {
    out.RL(SchemaDataSegment);
    out.RL(static_cast<std::uint32_t>(unknown * 2 * 4));
    out.RL(UnknownOwners.at(unknown));
  }
  const std::uint32_t system = Sixteens(out.Bytes().size());
  WriteNames(out, SchemaNames);
  return Contents{.Bytes = out.Take(), .System = system};
}
} // namespace DwgIO::Store
