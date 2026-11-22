/***********************************************************************************************************************
 *  @file      source/entities/shapes/mline.hpp
 *  @brief     How an MLINE lies in the file, chapter 20.4.50.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_MLINE_HPP
#define DWGIO_ENTITIES_SHAPES_MLINE_HPP

#include "entities/shapes/count.hpp"
#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/mline.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <string_view>
#include <utility>

namespace DwgIO::Entities::Shapes
{
inline auto ReadParameters(std::vector<double> &values, const Fields::Input &in) -> bool
{
  const std::optional<std::size_t> count = ReadShortCount(in);
  if (!count)
  {
    return false;
  }
  values.assign(*count, 0.0);
  for (double &value : values)
  {
    value = in.In->BD();
  }
  return !in.In->HasFailed();
}

inline void WriteParameters(const std::vector<double> &values, const Fields::Output &to)
{
  to.To->BS(static_cast<int>(values.size()));
  for (const double value : values)
  {
    to.To->BD(value);
  }
}

/**
 *  The file counts the lines in a byte.
 */
constexpr int MostLines = 255;

/**
 *  A count beyond a byte is written as the most and noted.
 */
inline auto ReadLines(MLine &mline, const Fields::Input &in) -> bool
{
  mline.Lines = in.In->RC();
  return !in.In->HasFailed();
}

inline auto WriteLines(const MLine &mline, const Fields::Output &to) -> bool
{
  const int fitted = std::clamp(mline.Lines, 0, MostLines);
  if (fitted != mline.Lines)
  {
    Fields::Note(to, IssueCode::OutOfRange, std::format("lines {} out of range, {} written", mline.Lines, fitted));
  }
  to.To->RC(static_cast<std::uint8_t>(fitted));
  return true;
}

/**
 *  A BS count, then each joint's three points and one strand per style line, each strand two parameter lists.
 */
inline auto ReadJoints(MLine &mline, const Fields::Input &in) -> bool
{
  const std::optional<std::size_t> joints = ReadShortCount(in);
  if (!joints)
  {
    return false;
  }
  mline.Joints.assign(*joints, Joint{});
  for (Joint &joint : mline.Joints)
  {
    joint.Position = in.In->Point3();
    joint.Direction = in.In->Point3();
    joint.Miter = in.In->Point3();
    joint.Strands.assign(static_cast<std::size_t>(mline.Lines), Strand{});
    for (Strand &strand : joint.Strands)
    {
      if (!ReadParameters(strand.Spans, in) || !ReadParameters(strand.Fills, in))
      {
        return false;
      }
    }
  }
  return !in.In->HasFailed();
}

inline auto WriteJoints(const MLine &mline, const Fields::Output &to) -> bool
{
  to.To->BS(static_cast<int>(mline.Joints.size()));
  for (const Joint &joint : mline.Joints)
  {
    to.To->Point3(joint.Position);
    to.To->Point3(joint.Direction);
    to.To->Point3(joint.Miter);
    for (std::size_t at = 0; std::cmp_less(at, std::clamp(mline.Lines, 0, MostLines)); ++at)
    {
      const Strand strand = at < joint.Strands.size() ? joint.Strands[at] : Strand{};
      WriteParameters(strand.Spans, to);
      WriteParameters(strand.Fills, to);
    }
  }
  return true;
}
} // namespace DwgIO::Entities::Shapes

namespace DwgIO::Objects
{
template <>
struct Shape<Entities::MLine>
{
  static constexpr std::array<std::uint16_t, 1> Types{47};
  static constexpr std::string_view Name = "MLINE";

  static auto Layout()
  {
    using Entities::MLine;
    return Fields::Layout(Fields::Double(&MLine::Scale), Fields::Byte(&MLine::Justification),
                          Fields::Point3(&MLine::Start), Fields::Point3(&MLine::Normal), Fields::Short(&MLine::Flags),
                          Fields::Custom(Entities::Shapes::ReadLines, Entities::Shapes::WriteLines),
                          Fields::Custom(Entities::Shapes::ReadJoints, Entities::Shapes::WriteJoints),
                          Fields::Link(&MLine::Style, 5));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_MLINE_HPP */
