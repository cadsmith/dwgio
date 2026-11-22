/***********************************************************************************************************************
 *  @file      source/entities/shapes/dimension.hpp
 *  @brief     How each kind of dimension lies in the file, the arc and large radial ones among them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_DIMENSION_HPP
#define DWGIO_ENTITIES_SHAPES_DIMENSION_HPP

#include "entities/shapes/lettering.hpp"
#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/nested.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/dimension.hpp"
#include "dwgio/version.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <variant>

namespace DwgIO::Entities::Shapes
{
/**
 *  The bits of group 70 the two flag bytes carry: the first byte's bit 0 is set when the text was not moved, its bit 1
 *  is the 32 bit; an ordinate's second byte's bit 0 is the 64 bit.
 */
namespace Dim
{
constexpr unsigned Unique = 32;
constexpr unsigned AlongX = 64;
constexpr unsigned Moved = 128;
constexpr std::uint8_t Unmoved = 1;
constexpr std::uint8_t UniqueBit = 2;
constexpr std::uint8_t AlongXBit = 1;
constexpr unsigned Named = Unmoved | UniqueBit;
} // namespace Dim

/**
 *  Chapter 20.4.22: what every kind of dimension opens with. The first flag byte's unnamed bits are kept.
 */
template <typename KindT>
auto ReadMeasure(KindT &dimension, const Fields::Input &in) -> bool
{
  Measure &measure = dimension.Measure;
  if (in.With.Release >= Version::R2010)
  {
    measure.Version = in.In->RC();
  }
  measure.Normal = in.In->Point3();
  measure.TextMiddle = ReadPair(in);
  const double elevation = in.In->BD();
  const std::uint8_t flags = in.In->RC();
  measure.Flags = static_cast<int>(((flags & Dim::Unmoved) != 0 ? 0U : Dim::Moved) |
                                   ((flags & Dim::UniqueBit) != 0 ? Dim::Unique : 0U));
  measure.Unknown1 = static_cast<int>(flags & ~Dim::Named);
  measure.Text = Fields::TextCode::Read(in);
  measure.TextRotation = in.In->BD();
  measure.HorizontalDirection = in.In->BD();
  measure.Scale = in.In->Point3();
  measure.BlockRotation = in.In->BD();
  if (in.With.Release >= Version::R2000)
  {
    measure.Attachment = in.In->BS();
    measure.LineSpacingStyle = in.In->BS();
    measure.LineSpacingFactor = in.In->BD();
    measure.Measured = in.In->BD();
  }
  if (in.With.Release >= Version::R2007)
  {
    measure.Unknown2 = in.In->B();
    measure.FlipFirst = in.In->B();
    measure.FlipSecond = in.In->B();
  }
  measure.CloneInsertion = ReadPair(in);
  measure.TextMiddle.Z = elevation;
  measure.CloneInsertion.Z = elevation;
  return !in.In->HasFailed();
}

template <typename KindT>
auto WriteMeasure(const KindT &dimension, const Fields::Output &to) -> bool
{
  const Measure &measure = dimension.Measure;
  const auto bits = static_cast<unsigned>(measure.Flags);
  if (to.With.Release >= Version::R2010)
  {
    to.To->RC(static_cast<std::uint8_t>(measure.Version));
  }
  to.To->Point3(measure.Normal);
  WritePair(measure.TextMiddle, to);
  to.To->BD(measure.TextMiddle.Z);
  to.To->RC(static_cast<std::uint8_t>(((bits & Dim::Moved) != 0 ? 0U : Dim::Unmoved) |
                                      ((bits & Dim::Unique) != 0 ? Dim::UniqueBit : 0U) |
                                      (static_cast<unsigned>(measure.Unknown1) & ~Dim::Named)));
  Fields::TextCode::Write(measure.Text, to);
  to.To->BD(measure.TextRotation);
  to.To->BD(measure.HorizontalDirection);
  to.To->Point3(measure.Scale);
  to.To->BD(measure.BlockRotation);
  if (to.With.Release >= Version::R2000)
  {
    to.To->BS(measure.Attachment);
    to.To->BS(measure.LineSpacingStyle);
    to.To->BD(measure.LineSpacingFactor);
    to.To->BD(measure.Measured);
  }
  if (to.With.Release >= Version::R2007)
  {
    to.To->B(measure.Unknown2);
    to.To->B(measure.FlipFirst);
    to.To->B(measure.FlipSecond);
  }
  WritePair(measure.CloneInsertion, to);
  return true;
}

/**
 *  Chapters 20.4.23 to 20.4.29: each kind's points in its order, the definition point among them.
 */
inline auto ReadForm(Dimension &dimension, const Fields::Input &in) -> bool
{
  Measure &measure = dimension.Measure;
  std::visit(
      [&](auto &form) -> void
      {
        using FormT = std::remove_cvref_t<decltype(form)>;
        if constexpr (std::is_same_v<FormT, Forms::Ordinate>)
        {
          measure.DefinitionPoint = in.In->Point3();
          form.Feature = in.In->Point3();
          form.Leader = in.In->Point3();
          const std::uint8_t flags = in.In->RC();
          measure.Flags = static_cast<int>(static_cast<unsigned>(measure.Flags) |
                                           ((flags & Dim::AlongXBit) != 0 ? Dim::AlongX : 0U));
        }
        else if constexpr (std::is_same_v<FormT, Forms::Linear>)
        {
          form.Start = in.In->Point3();
          form.End = in.In->Point3();
          measure.DefinitionPoint = in.In->Point3();
          form.Oblique = in.In->BD();
          form.Angle = in.In->BD();
        }
        else if constexpr (std::is_same_v<FormT, Forms::Aligned>)
        {
          form.Start = in.In->Point3();
          form.End = in.In->Point3();
          measure.DefinitionPoint = in.In->Point3();
          form.Oblique = in.In->BD();
        }
        else if constexpr (std::is_same_v<FormT, Forms::Angular3Point>)
        {
          measure.DefinitionPoint = in.In->Point3();
          form.Start = in.In->Point3();
          form.End = in.In->Point3();
          form.Vertex = in.In->Point3();
        }
        else if constexpr (std::is_same_v<FormT, Forms::Angular2Line>)
        {
          form.ArcPoint = ReadPair(in);
          form.ArcPoint.Z = measure.TextMiddle.Z;
          form.FirstStart = in.In->Point3();
          form.FirstEnd = in.In->Point3();
          form.SecondStart = in.In->Point3();
          measure.DefinitionPoint = in.In->Point3();
        }
        else if constexpr (std::is_same_v<FormT, Forms::Radial>)
        {
          measure.DefinitionPoint = in.In->Point3();
          form.Chord = in.In->Point3();
          form.Leader = in.In->BD();
        }
        else
        {
          form.Chord = in.In->Point3();
          measure.DefinitionPoint = in.In->Point3();
          form.Leader = in.In->BD();
        }
      },
      dimension.Form);
  return !in.In->HasFailed();
}

inline auto WriteForm(const Dimension &dimension, const Fields::Output &to) -> bool
{
  const Measure &measure = dimension.Measure;
  std::visit(
      [&](const auto &form) -> void
      {
        using FormT = std::remove_cvref_t<decltype(form)>;
        if constexpr (std::is_same_v<FormT, Forms::Ordinate>)
        {
          to.To->Point3(measure.DefinitionPoint);
          to.To->Point3(form.Feature);
          to.To->Point3(form.Leader);
          to.To->RC((static_cast<unsigned>(measure.Flags) & Dim::AlongX) != 0 ? Dim::AlongXBit : 0);
        }
        else if constexpr (std::is_same_v<FormT, Forms::Linear>)
        {
          to.To->Point3(form.Start);
          to.To->Point3(form.End);
          to.To->Point3(measure.DefinitionPoint);
          to.To->BD(form.Oblique);
          to.To->BD(form.Angle);
        }
        else if constexpr (std::is_same_v<FormT, Forms::Aligned>)
        {
          to.To->Point3(form.Start);
          to.To->Point3(form.End);
          to.To->Point3(measure.DefinitionPoint);
          to.To->BD(form.Oblique);
        }
        else if constexpr (std::is_same_v<FormT, Forms::Angular3Point>)
        {
          to.To->Point3(measure.DefinitionPoint);
          to.To->Point3(form.Start);
          to.To->Point3(form.End);
          to.To->Point3(form.Vertex);
        }
        else if constexpr (std::is_same_v<FormT, Forms::Angular2Line>)
        {
          WritePair(form.ArcPoint, to);
          to.To->Point3(form.FirstStart);
          to.To->Point3(form.FirstEnd);
          to.To->Point3(form.SecondStart);
          to.To->Point3(measure.DefinitionPoint);
        }
        else if constexpr (std::is_same_v<FormT, Forms::Radial>)
        {
          to.To->Point3(measure.DefinitionPoint);
          to.To->Point3(form.Chord);
          to.To->BD(form.Leader);
        }
        else
        {
          to.To->Point3(form.Chord);
          to.To->Point3(measure.DefinitionPoint);
          to.To->BD(form.Leader);
        }
      },
      dimension.Form);
  return true;
}

/**
 *  Default-made; an index beyond the last gives the last.
 */
inline auto FormAt(std::size_t index) -> Forms::Form
{
  static constexpr std::array<Forms::Form (*)(), 7> Makers{
      [] -> Forms::Form { return Forms::Ordinate{}; },     [] -> Forms::Form { return Forms::Linear{}; },
      [] -> Forms::Form { return Forms::Aligned{}; },      [] -> Forms::Form { return Forms::Angular3Point{}; },
      [] -> Forms::Form { return Forms::Angular2Line{}; }, [] -> Forms::Form { return Forms::Radial{}; },
      [] -> Forms::Form { return Forms::Diameter{}; }};
  return Makers.at(std::min(index, Makers.size() - 1))();
}
} // namespace DwgIO::Entities::Shapes

namespace DwgIO::Objects
{
template <>
struct Shape<Entities::Dimension>
{
  static constexpr std::array<std::uint16_t, 7> Types{20, 21, 22, 23, 24, 25, 26};
  static constexpr std::string_view Name = "DIMENSION_ORDINATE";

  static void Open(Entities::Dimension &dimension, std::uint16_t type)
  {
    dimension.Form = Entities::Shapes::FormAt(static_cast<std::size_t>(type - Types.front()));
  }

  static auto Code(const Entities::Dimension &dimension) -> std::uint16_t
  {
    return static_cast<std::uint16_t>(Types.front() + dimension.Form.index());
  }

  static auto NameOf(const Entities::Dimension &dimension) -> std::string_view
  {
    static constexpr std::array<std::string_view, 7> Names{
        "DIMENSION_ORDINATE", "DIMENSION_LINEAR", "DIMENSION_ALIGNED", "DIMENSION_ANG3PT",
        "DIMENSION_ANG2LN",   "DIMENSION_RADIUS", "DIMENSION_DIAMETER"};
    return Names.at(dimension.Form.index());
  }

  static auto Layout()
  {
    using Entities::Dimension;
    using namespace Entities::Shapes;
    return Fields::Layout(Fields::Custom(ReadMeasure<Dimension>, WriteMeasure<Dimension>),
                          Fields::Custom(ReadForm, WriteForm), Fields::Link(&Dimension::Style, 5),
                          Fields::Link(&Dimension::Block, 5));
  }
};
} // namespace DwgIO::Objects

namespace DwgIO::Objects
{
/**
 *  Chapter 20.4.19; stored under its class.
 */
template <>
struct Shape<Entities::ArcDimension>
{
  static constexpr std::array<std::uint16_t, 0> Types{};
  static constexpr std::string_view Name = "ARC_DIMENSION";

  static auto Layout()
  {
    using Entities::ArcDimension;
    using Entities::Measure;
    using namespace Entities::Shapes;
    return Fields::Layout(
        Fields::Custom(ReadMeasure<ArcDimension>, WriteMeasure<ArcDimension>),
        Fields::Part(&ArcDimension::Measure, Fields::Layout(Fields::Point3(&Measure::DefinitionPoint))),
        Fields::Point3(&ArcDimension::Start), Fields::Point3(&ArcDimension::End), Fields::Point3(&ArcDimension::Center),
        Fields::Bit(&ArcDimension::Partial), Fields::Double(&ArcDimension::StartAngle),
        Fields::Double(&ArcDimension::EndAngle), Fields::Bit(&ArcDimension::HasLeader),
        Fields::Point3(&ArcDimension::FirstLeaderPoint), Fields::Point3(&ArcDimension::SecondLeaderPoint),
        Fields::Link(&ArcDimension::Style, 5), Fields::Link(&ArcDimension::Block, 5));
  }
};

/**
 *  Chapter 20.4.30; stored under its class.
 */
template <>
struct Shape<Entities::LargeRadialDimension>
{
  static constexpr std::array<std::uint16_t, 0> Types{};
  static constexpr std::string_view Name = "LARGE_RADIAL_DIMENSION";

  static auto Layout()
  {
    using Entities::LargeRadialDimension;
    using Entities::Measure;
    using namespace Entities::Shapes;
    return Fields::Layout(
        Fields::Custom(ReadMeasure<LargeRadialDimension>, WriteMeasure<LargeRadialDimension>),
        Fields::Part(&LargeRadialDimension::Measure, Fields::Layout(Fields::Point3(&Measure::DefinitionPoint))),
        Fields::Point3(&LargeRadialDimension::Chord), Fields::Double(&LargeRadialDimension::JogAngle),
        Fields::Point3(&LargeRadialDimension::Center), Fields::Point3(&LargeRadialDimension::JogPoint),
        Fields::Link(&LargeRadialDimension::Style, 5), Fields::Link(&LargeRadialDimension::Block, 5));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_DIMENSION_HPP */
