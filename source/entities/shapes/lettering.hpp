/***********************************************************************************************************************
 *  @file      source/entities/shapes/lettering.hpp
 *  @brief     The text fields TEXT, ATTRIB and ATTDEF share, packed from R2000.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_LETTERING_HPP
#define DWGIO_ENTITIES_SHAPES_LETTERING_HPP

#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/since.hpp"

#include "dwgio/entities/lettering.hpp"
#include "dwgio/vector.hpp"
#include "dwgio/version.hpp"

#include <cstdint>
#include <format>
#include <utility>

namespace DwgIO::Entities::Shapes
{
/**
 *  From R2000 each data flag bit says its value is left out at its default.
 */
namespace Left
{
constexpr std::uint8_t Elevation = 0x01;
constexpr std::uint8_t Alignment = 0x02;
constexpr std::uint8_t Oblique = 0x04;
constexpr std::uint8_t Rotation = 0x08;
constexpr std::uint8_t Width = 0x10;
constexpr std::uint8_t Generation = 0x20;
constexpr std::uint8_t Horizontal = 0x40;
constexpr std::uint8_t Vertical = 0x80;
} // namespace Left

inline auto ReadPair(const Fields::Input &in) -> Vector
{
  Vector pair;
  pair.X = in.In->RD();
  pair.Y = in.In->RD();
  return pair;
}

inline void WritePair(const Vector &pair, const Fields::Output &to)
{
  to.To->RD(pair.X);
  to.To->RD(pair.Y);
}

inline auto ReadOlderLettering(Lettering &lettering, const Fields::Input &in) -> bool
{
  const double elevation = in.In->BD();
  lettering.Position = ReadPair(in);
  lettering.Alignment = ReadPair(in);
  lettering.Position.Z = elevation;
  lettering.Alignment.Z = elevation;
  lettering.Extrusion = in.In->Point3();
  lettering.Thickness = in.In->BD();
  lettering.Oblique = in.In->BD();
  lettering.Rotation = in.In->BD();
  lettering.Height = in.In->BD();
  lettering.Width = in.In->BD();
  lettering.Value = Fields::TextCode::Read(in);
  lettering.Generation = in.In->BS();
  lettering.Horizontal = static_cast<HorizontalAlignment>(in.In->BS());
  lettering.Vertical = static_cast<VerticalAlignment>(in.In->BS());
  // A text left on its baseline has no alignment point; R14 keeps a placeholder pair where R2000 keeps nothing.
  if (lettering.Horizontal == HorizontalAlignment::Left && lettering.Vertical == VerticalAlignment::Baseline)
  {
    lettering.Alignment = lettering.Position;
  }
  return !in.In->HasFailed();
}

inline auto WriteOlderLettering(const Lettering &lettering, const Fields::Output &to) -> bool
{
  to.To->BD(lettering.Position.Z);
  WritePair(lettering.Position, to);
  // A text left on its baseline keeps a zero placeholder for the pair the reader takes as its position.
  const bool baseline =
      lettering.Horizontal == HorizontalAlignment::Left && lettering.Vertical == VerticalAlignment::Baseline;
  WritePair(baseline ? Vector{} : lettering.Alignment, to);
  to.To->Point3(lettering.Extrusion);
  to.To->BD(lettering.Thickness);
  to.To->BD(lettering.Oblique);
  to.To->BD(lettering.Rotation);
  to.To->BD(lettering.Height);
  to.To->BD(lettering.Width);
  Fields::TextCode::Write(lettering.Value, to);
  to.To->BS(lettering.Generation);
  to.To->BS(std::to_underlying(lettering.Horizontal));
  to.To->BS(std::to_underlying(lettering.Vertical));
  return true;
}

/**
 *  The alignment counts as at its default when it is the position.
 */
inline auto FlagsOf(const Lettering &lettering) -> std::uint8_t
{
  std::uint8_t flags = 0;
  flags |= lettering.Position.Z == 0 ? Left::Elevation : std::uint8_t{0};
  flags |= lettering.Alignment.X == lettering.Position.X && lettering.Alignment.Y == lettering.Position.Y
               ? Left::Alignment
               : std::uint8_t{0};
  flags |= lettering.Oblique == 0 ? Left::Oblique : std::uint8_t{0};
  flags |= lettering.Rotation == 0 ? Left::Rotation : std::uint8_t{0};
  flags |= lettering.Width == 1 ? Left::Width : std::uint8_t{0};
  flags |= lettering.Generation == 0 ? Left::Generation : std::uint8_t{0};
  flags |= lettering.Horizontal == HorizontalAlignment::Left ? Left::Horizontal : std::uint8_t{0};
  flags |= lettering.Vertical == VerticalAlignment::Baseline ? Left::Vertical : std::uint8_t{0};
  return flags;
}

/**
 *  From R2000: the data flags, then each value its flag does not leave out, the position as doubles and the alignment
 *  as deltas against it.
 */
inline auto ReadLettering(Lettering &lettering, const Fields::Input &in) -> bool
{
  const std::uint8_t flags = in.In->RC();
  const auto kept = [flags](std::uint8_t bit) -> bool
  {
    return (flags & bit) == 0;
  };
  const double elevation = kept(Left::Elevation) ? in.In->RD() : 0.0;
  lettering.Position = ReadPair(in);
  lettering.Position.Z = elevation;
  lettering.Alignment = lettering.Position;
  if (kept(Left::Alignment))
  {
    lettering.Alignment.X = in.In->DD(lettering.Position.X);
    lettering.Alignment.Y = in.In->DD(lettering.Position.Y);
  }
  lettering.Extrusion = in.In->BE(in.With.Release);
  lettering.Thickness = in.In->BT(in.With.Release);
  lettering.Oblique = kept(Left::Oblique) ? in.In->RD() : 0.0;
  lettering.Rotation = kept(Left::Rotation) ? in.In->RD() : 0.0;
  lettering.Height = in.In->RD();
  lettering.Width = kept(Left::Width) ? in.In->RD() : 1.0;
  lettering.Value = Fields::TextCode::Read(in);
  lettering.Generation = kept(Left::Generation) ? in.In->BS() : std::uint8_t{0};
  lettering.Horizontal =
      kept(Left::Horizontal) ? static_cast<HorizontalAlignment>(in.In->BS()) : HorizontalAlignment::Left;
  lettering.Vertical = kept(Left::Vertical) ? static_cast<VerticalAlignment>(in.In->BS()) : VerticalAlignment::Baseline;
  return !in.In->HasFailed();
}

inline auto WriteLettering(const Lettering &lettering, const Fields::Output &to) -> bool
{
  const std::uint8_t flags = FlagsOf(lettering);
  const auto kept = [flags](std::uint8_t bit) -> bool
  {
    return (flags & bit) == 0;
  };
  to.To->RC(flags);
  if (kept(Left::Elevation))
  {
    to.To->RD(lettering.Position.Z);
  }
  WritePair(lettering.Position, to);
  if (kept(Left::Alignment))
  {
    to.To->DD(lettering.Alignment.X, lettering.Position.X);
    to.To->DD(lettering.Alignment.Y, lettering.Position.Y);
  }
  to.To->BE(lettering.Extrusion, to.With.Release);
  to.To->BT(lettering.Thickness, to.With.Release);
  if (kept(Left::Oblique))
  {
    to.To->RD(lettering.Oblique);
  }
  if (kept(Left::Rotation))
  {
    to.To->RD(lettering.Rotation);
  }
  to.To->RD(lettering.Height);
  if (kept(Left::Width))
  {
    to.To->RD(lettering.Width);
  }
  Fields::TextCode::Write(lettering.Value, to);
  if (kept(Left::Generation))
  {
    to.To->BS(lettering.Generation);
  }
  if (kept(Left::Horizontal))
  {
    to.To->BS(std::to_underlying(lettering.Horizontal));
  }
  if (kept(Left::Vertical))
  {
    to.To->BS(std::to_underlying(lettering.Vertical));
  }
  return true;
}

/**
 *  From R2018.
 *
 *  A multiline attribute, whose embedded text the library does not read, fails the read and is noted, and the object is
 *  kept as bytes.
 */
template <typename AttributeT>
inline auto ReadType(AttributeT &attribute, const Fields::Input &in) -> bool
{
  attribute.Type = in.In->RC();
  if (attribute.Type != 1)
  {
    Fields::Note(in, IssueCode::Unknown, std::format("multiline attribute of type {} not read", attribute.Type));
    in.In->Seek(in.In->Size() + 1); // Fails the reader, so no field after reads what follows as its own.
    return false;
  }
  return !in.In->HasFailed();
}

template <typename AttributeT>
inline auto WriteType(const AttributeT &attribute, const Fields::Output &to) -> bool
{
  to.To->RC(static_cast<std::uint8_t>(attribute.Type));
  return true;
}

/**
 *  Chapter 20.4.3's common text data.
 */
inline auto LetteringLayout()
{
  return Fields::Layout(Fields::Until(Version::R14, Fields::Custom(ReadOlderLettering, WriteOlderLettering)),
                        Fields::Since(Version::R2000, Fields::Custom(ReadLettering, WriteLettering)),
                        Fields::Link(&Lettering::Style, 5));
}
} // namespace DwgIO::Entities::Shapes

#endif /* DWGIO_ENTITIES_SHAPES_LETTERING_HPP */
