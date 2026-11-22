/***********************************************************************************************************************
 *  @file      source/entities/shapes/geometry.hpp
 *  @brief     The ACIS data of a region, solid or body: SAT blocks or SAB, chapter 20.4.41.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_GEOMETRY_HPP
#define DWGIO_ENTITIES_SHAPES_GEOMETRY_HPP

#include "bits/byte.hpp"
#include "encoding/guid.hpp"
#include "entities/shapes/count.hpp"
#include "fields/codes.hpp"
#include "fields/context.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/list.hpp"
#include "fields/together.hpp"
#include "octets.hpp"

#include "dwgio/entities/geometry.hpp"
#include "dwgio/version.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace DwgIO::Entities::Shapes
{
namespace Formats
{
/**
 *  SAT text, in blocks of encrypted characters.
 */
constexpr int Sat = 1;
/**
 *  SAB binary, which runs to its end marker.
 */
constexpr int Sab = 2;
/**
 *  Never in a file: what the reader leaves in a geometry it could not read, so the object stays as bytes.
 */
constexpr int Unread = -1;
/**
 *  The most characters AutoCAD writes in one SAT block; a block of zero ends them.
 */
constexpr std::size_t Block = 4096;
/**
 *  SAT encryption maps these characters to `0x9F` less themselves; the others stay as they are.
 */
constexpr std::uint8_t Lowest = 0x21;
constexpr std::uint8_t Highest = 0x7E;
constexpr std::uint8_t Mirror = 0x9F;
/**
 *  The tokens that end a SAB body: `End-of-ACIS-data`, or `End-of-ASM-data` from ACIS 21800.
 */
constexpr std::array<std::string_view, 2> Ends{std::string_view{"\x0E\x03"
                                                                "End"
                                                                "\x0E\x02"
                                                                "of"
                                                                "\x0E\x04"
                                                                "ACIS"
                                                                "\x0D\x04"
                                                                "data"},
                                               std::string_view{"\x0E\x03"
                                                                "End"
                                                                "\x0E\x02"
                                                                "of"
                                                                "\x0E\x03"
                                                                "ASM"
                                                                "\x0D\x04"
                                                                "data"}};
} // namespace Formats

/**
 *  The same map encrypts and decrypts.
 */
inline auto DecryptByte(std::uint8_t one) -> char
{
  const bool mapped = one >= Formats::Lowest && one <= Formats::Highest;
  return static_cast<char>(mapped ? static_cast<std::uint8_t>(Formats::Mirror - one) : one);
}

inline auto ReadSat(std::string &acis, const Fields::Input &in) -> bool
{
  acis.clear();
  while (true)
  {
    const std::optional<std::size_t> size = CheckCount(in.In->BL(), in);
    if (!size)
    {
      return false;
    }
    if (*size == 0)
    {
      return true;
    }
    for (std::size_t at = 0; at < *size; ++at)
    {
      acis.push_back(DecryptByte(in.In->RC()));
    }
  }
}

inline void WriteSat(const std::string &acis, const Fields::Output &to)
{
  for (std::size_t at = 0; at < acis.size(); at += Formats::Block)
  {
    const std::size_t size = std::min(Formats::Block, acis.size() - at);
    to.To->BL(static_cast<std::int32_t>(size));
    for (std::size_t one = at; one < at + size; ++one)
    {
      to.To->RC(static_cast<std::uint8_t>(DecryptByte(static_cast<std::uint8_t>(acis[one]))));
    }
  }
  to.To->BL(0);
}

/**
 *  False when the data ends before the end marker.
 */
inline auto ReadSab(std::string &acis, const Fields::Input &in) -> bool
{
  acis.clear();
  const std::size_t end = in.DataEnd == 0 ? in.In->Size() : in.DataEnd;
  while (in.In->Position() + Bits::Byte <= end)
  {
    acis.push_back(static_cast<char>(in.In->RC()));
    if (std::ranges::any_of(Formats::Ends, [&acis](std::string_view marker) -> bool { return acis.ends_with(marker); }))
    {
      return true;
    }
  }
  return false;
}

/**
 *  The empty bit, and when clear the unknown bit, the version and the SAT blocks or the SAB bytes.
 *
 *  A version the layout does not know or a SAB with no end is noted, and leaves the geometry unread.
 */
inline auto ReadAcis(Acis &acis, const Fields::Input &in) -> bool
{
  acis = Acis{};
  if (in.In->B())
  {
    return !in.In->HasFailed();
  }
  acis.Textual = in.In->B();
  acis.Version = in.In->BS();
  std::string data;
  if (acis.Version == Formats::Sat)
  {
    const bool read = ReadSat(data, in);
    acis.Data = BytesOf(data);
    return read;
  }
  if (acis.Version == Formats::Sab && ReadSab(data, in))
  {
    acis.Data = BytesOf(data);
    return true;
  }
  Fields::Note(in, IssueCode::Unknown, std::format("ACIS data of version {} not read", acis.Version));
  acis.Version = Formats::Unread;
  return false;
}

inline void WriteAcis(const Acis &acis, const Fields::Output &to)
{
  to.To->B(acis.Version == 0);
  if (acis.Version == 0)
  {
    return;
  }
  to.To->B(acis.Textual);
  to.To->BS(acis.Version);
  if (acis.Version == Formats::Sat)
  {
    WriteSat(TextOf(acis.Data), to);
    return;
  }
  for (const std::byte one : acis.Data)
  {
    to.To->RC(std::to_integer<std::uint8_t>(one));
  }
}

/**
 *  A bit set when the transform follows, then the three axes, the translation, the scale and three bits.
 */
inline auto ReadPlacement(Wire &wire, const Fields::Input &in) -> bool
{
  wire.Transform.reset();
  if (!in.In->B())
  {
    return !in.In->HasFailed();
  }
  Transform &transform = wire.Transform.emplace();
  transform.XAxis = in.In->Point3();
  transform.YAxis = in.In->Point3();
  transform.ZAxis = in.In->Point3();
  transform.Translation = in.In->Point3();
  transform.Scale = in.In->BD();
  transform.Rotation = in.In->B();
  transform.Reflection = in.In->B();
  transform.Shear = in.In->B();
  return !in.In->HasFailed();
}

inline auto WritePlacement(const Wire &wire, const Fields::Output &to) -> bool
{
  to.To->B(wire.Transform.has_value());
  if (const std::optional<Transform> &transform = wire.Transform)
  {
    to.To->Point3(transform->XAxis);
    to.To->Point3(transform->YAxis);
    to.To->Point3(transform->ZAxis);
    to.To->Point3(transform->Translation);
    to.To->BD(transform->Scale);
    to.To->B(transform->Rotation);
    to.To->B(transform->Reflection);
    to.To->B(transform->Shear);
  }
  return true;
}

inline auto WireLayout()
{
  return Fields::Layout(Fields::Byte(&Wire::Type), Fields::Long(&Wire::Marker), Fields::Short(&Wire::Color),
                        Fields::Long(&Wire::Index),
                        Fields::List(&Wire::Points, Fields::Counted::Long, Fields::Point3()),
                        Fields::Custom(ReadPlacement, WritePlacement));
}

inline auto SilhouetteLayout()
{
  return Fields::Layout(Fields::Long(&Silhouette::Viewport), Fields::Point3(&Silhouette::Target),
                        Fields::Point3(&Silhouette::Direction), Fields::Point3(&Silhouette::Upward),
                        Fields::Bit(&Silhouette::Perspective),
                        Fields::List(&Silhouette::Wires, Fields::Counted::Long, Fields::Together(WireLayout())));
}

/**
 *  A bit set when the wireframe follows: the point behind a bit of its own, the isoline count, and behind a bit the
 *  wires; then the silhouettes.
 */
inline auto ReadFrame(Geometry &geometry, const Fields::Input &in) -> bool
{
  geometry.Point.reset();
  geometry.Isolines = 0;
  geometry.Wired = false;
  geometry.Wires.clear();
  geometry.Framed = in.In->B();
  Fields::Input reading = in;
  if (geometry.Framed)
  {
    if (in.In->B())
    {
      geometry.Point = in.In->Point3();
    }
    geometry.Isolines = in.In->BL();
    geometry.Wired = in.In->B();
    if (geometry.Wired &&
        !Fields::List(&Geometry::Wires, Fields::Counted::Long, Fields::Together(WireLayout())).Read(geometry, reading))
    {
      return false;
    }
  }
  return Fields::List(&Geometry::Silhouettes, Fields::Counted::Long, Fields::Together(SilhouetteLayout()))
      .Read(geometry, reading);
}

inline auto WriteFrame(const Geometry &geometry, const Fields::Output &to) -> bool
{
  Fields::Output writing = to;
  to.To->B(geometry.Framed);
  if (geometry.Framed)
  {
    to.To->B(geometry.Point.has_value());
    if (geometry.Point)
    {
      to.To->Point3(*geometry.Point);
    }
    to.To->BL(geometry.Isolines);
    to.To->B(geometry.Wired);
    if (geometry.Wired)
    {
      static_cast<void>(Fields::List(&Geometry::Wires, Fields::Counted::Long, Fields::Together(WireLayout()))
                            .Write(geometry, writing));
    }
  }
  return Fields::List(&Geometry::Silhouettes, Fields::Counted::Long, Fields::Together(SilhouetteLayout()))
      .Write(geometry, writing);
}

inline constexpr std::string_view NoGuid = "{00000000-0000-0000-0000-000000000000}";

/**
 *  A long, two shorts and eight bytes, kept as DXF's text.
 */
inline void ReadGuid(Geometry &geometry, const Fields::Input &in)
{
  Encoding::Guid guid{.Data1 = static_cast<std::uint32_t>(in.In->BL()),
                      .Data2 = static_cast<std::uint16_t>(in.In->BS()),
                      .Data3 = static_cast<std::uint16_t>(in.In->BS()),
                      .Data4 = {}};
  for (std::uint8_t &one : guid.Data4)
  {
    one = in.In->RC();
  }
  geometry.Guid = Encoding::FormatGuid(guid, false);
}

/**
 *  Nothing when they are not all hexadecimal digits.
 */
inline auto ParseHex(std::string_view text, std::size_t from, std::size_t count) -> std::optional<std::uint32_t>
{
  std::uint32_t value{};
  const std::string_view digits = text.substr(from, count);
  const auto [end, error] = std::from_chars(std::to_address(digits.begin()), std::to_address(digits.end()), value, 16);
  if (error != std::errc{} || end != std::to_address(digits.end()))
  {
    return std::nullopt;
  }
  return value;
}

inline auto WriteGuid(const Geometry &geometry, const Fields::Output &to) -> bool
{
  // Where DXF's form puts the digits: eight, four, four, then eight bytes of two.
  constexpr std::array<std::size_t, 11> Starts{1, 10, 15, 20, 22, 25, 27, 29, 31, 33, 35};
  constexpr std::array<std::size_t, 11> Counts{8, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2};
  const std::string_view text = geometry.Guid.empty() ? NoGuid : std::string_view{geometry.Guid};
  std::array<std::uint32_t, 11> parts{};
  bool whole = text.size() == NoGuid.size();
  for (std::size_t at = 0; whole && at < parts.size(); ++at)
  {
    const std::optional<std::uint32_t> part = ParseHex(text, Starts.at(at), Counts.at(at));
    whole = part.has_value();
    parts.at(at) = part.value_or(0);
  }
  if (!whole)
  {
    Fields::Note(to, IssueCode::Unknown, std::format("GUID {} not understood, zero written", geometry.Guid));
    parts.fill(0);
  }
  to.To->BL(static_cast<std::int32_t>(parts[0]));
  to.To->BS(static_cast<std::int16_t>(parts[1]));
  to.To->BS(static_cast<std::int16_t>(parts[2]));
  for (std::size_t at = 3; at < parts.size(); ++at)
  {
    to.To->RC(static_cast<std::uint8_t>(parts.at(at)));
  }
  return true;
}

/**
 *  From R2013 the first stream is gone, and a bit says whether the data store holds the body.
 */
inline auto ReadGeometry(Geometry &geometry, const Fields::Input &in) -> bool
{
  const Version release = in.With.Release;
  geometry.Body = Acis{};
  geometry.Stored = false;
  geometry.Guid.clear();
  geometry.Unknown1 = 0;
  geometry.Unknown2 = 0;
  if (release < Version::R2013 && !ReadAcis(geometry.Body, in))
  {
    return false;
  }
  if (!ReadFrame(geometry, in) || !ReadAcis(geometry.Second, in))
  {
    return false;
  }
  if (release >= Version::R2007)
  {
    geometry.Unknown1 = in.In->BL();
  }
  if (release >= Version::R2013)
  {
    geometry.Stored = in.In->B();
    ReadGuid(geometry, in);
    geometry.Unknown2 = in.In->BL();
  }
  return !in.In->HasFailed();
}

inline auto WriteGeometry(const Geometry &geometry, const Fields::Output &to) -> bool
{
  const Version release = to.With.Release;
  if (release < Version::R2013)
  {
    WriteAcis(geometry.Body, to);
  }
  WriteFrame(geometry, to);
  WriteAcis(geometry.Second, to);
  if (release >= Version::R2007)
  {
    to.To->BL(geometry.Unknown1);
  }
  if (release >= Version::R2013)
  {
    to.To->B(geometry.Stored);
    WriteGuid(geometry, to);
    to.To->BL(geometry.Unknown2);
  }
  return true;
}

/**
 *  An object whose geometry was not read whole stays as its bytes.
 */
inline auto IsWhole(const Geometry &geometry) -> bool
{
  return geometry.Body.Version != Formats::Unread && geometry.Second.Version != Formats::Unread;
}
} // namespace DwgIO::Entities::Shapes

#endif /* DWGIO_ENTITIES_SHAPES_GEOMETRY_HPP */
