/***********************************************************************************************************************
 *  @file      source/entities/shapes/procedural.hpp
 *  @brief     The fields of the extruded, lofted, revolved and swept surfaces.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_PROCEDURAL_HPP
#define DWGIO_ENTITIES_SHAPES_PROCEDURAL_HPP

#include "bits/byte.hpp"
#include "entities/shapes/count.hpp"
#include "fields/codes.hpp"
#include "fields/context.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/nested.hpp"
#include "fields/opaque.hpp"

#include "dwgio/entities/procedural.hpp"
#include "dwgio/opaque.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace DwgIO::Entities::Shapes
{
/**
 *  Sixteen BD, row by row.
 */
template <typename KindT>
struct MatrixField
{
  Matrix KindT::*At{nullptr};

  auto Read(KindT &kind, Fields::Input &in) const -> bool
  {
    for (double &one : kind.*At)
    {
      one = in.In->BD();
    }
    return !in.In->HasFailed();
  }

  auto Write(const KindT &kind, Fields::Output &to) const -> bool
  {
    for (const double one : kind.*At)
    {
      to.To->BD(one);
    }
    return true;
  }
};

template <typename KindT>
auto TransformMatrix(Matrix KindT::*at) -> MatrixField<KindT>
{
  return {.At = at};
}

/**
 *  An entity a surface was made from: its type code as a BL, the count of its data bits as a BL, and the bits.
 */
inline auto ReadProfile(Profile &profile, const Fields::Input &in) -> bool
{
  profile.Type = in.In->BL();
  const std::optional<std::size_t> size = CheckCount(in.In->BL(), in);
  if (!size)
  {
    return false;
  }
  profile.Data = BitString{.Bytes = {}, .Size = *size};
  for (std::size_t done = 0; done < *size; done += Bits::Byte)
  {
    const int count = static_cast<int>(std::min<std::size_t>(Bits::Byte, *size - done));
    profile.Data.Bytes.push_back(
        static_cast<std::byte>(in.In->Bits(count) << (Bits::Byte - static_cast<std::size_t>(count))));
  }
  return !in.In->HasFailed();
}

inline void WriteProfile(const Profile &profile, const Fields::Output &to)
{
  to.To->BL(profile.Type);
  to.To->BL(static_cast<std::int32_t>(profile.Data.Size));
  Fields::Put(profile.Data, *to.To);
}

template <typename KindT>
struct ProfileField
{
  Profile KindT::*At{nullptr};

  auto Read(KindT &kind, Fields::Input &in) const -> bool { return ReadProfile(kind.*At, in); }

  auto Write(const KindT &kind, Fields::Output &to) const -> bool
  {
    WriteProfile(kind.*At, to);
    return true;
  }
};

template <typename KindT>
auto SweepProfile(Profile KindT::*at) -> ProfileField<KindT>
{
  return {.At = at};
}

inline auto OptionsLayout()
{
  using O = SweepOptions;
  return Fields::Layout(Fields::Double(&O::Draft), Fields::Double(&O::StartDraft), Fields::Double(&O::EndDraft),
                        Fields::Double(&O::Twist), Fields::Double(&O::Scale), Fields::Double(&O::Align),
                        Fields::Bit(&O::Solid), Fields::Short(&O::Alignment), Fields::Short(&O::Miter),
                        Fields::Bit(&O::AlignStart), Fields::Bit(&O::Bank), Fields::Bit(&O::BasePoint),
                        Fields::Point3(&O::Reference), Fields::Bit(&O::ProfileComputed), Fields::Bit(&O::PathComputed),
                        TransformMatrix(&O::ProfileTransform), TransformMatrix(&O::PathTransform));
}

inline auto ExtrusionLayout()
{
  return Fields::Layout(Fields::Part(&Extrusion::Options, OptionsLayout()), Fields::Point3(&Extrusion::Direction),
                        TransformMatrix(&Extrusion::Transform), SweepProfile(&Extrusion::Profile));
}

inline auto SweepLayout()
{
  return Fields::Layout(Fields::Part(&Sweep::Options, OptionsLayout()), TransformMatrix(&Sweep::ProfileTransform),
                        TransformMatrix(&Sweep::PathTransform), SweepProfile(&Sweep::Profile),
                        SweepProfile(&Sweep::Path));
}

inline auto RevolutionLayout()
{
  using R = Revolution;
  return Fields::Layout(Fields::Double(&R::Draft), Fields::Double(&R::StartDraft), Fields::Double(&R::EndDraft),
                        Fields::Double(&R::Twist), Fields::Bit(&R::Solid), Fields::Bit(&R::Close),
                        Fields::Point3(&R::Point), Fields::Point3(&R::Axis), Fields::Double(&R::Angle),
                        Fields::Double(&R::Start), TransformMatrix(&R::Transform), SweepProfile(&R::Profile));
}

/**
 *  A lofted surface's options, between its counts and its entities.
 */
inline auto LoftLayout()
{
  using L = Loft;
  return Fields::Layout(Fields::Double(&L::StartDraft), Fields::Double(&L::EndDraft),
                        Fields::Double(&L::StartMagnitude), Fields::Double(&L::EndMagnitude),
                        Fields::Bit(&L::ArcLength), Fields::Bit(&L::NoTwist), Fields::Bit(&L::AlignDirection),
                        Fields::Bit(&L::Simple), Fields::Bit(&L::Closed), Fields::Bit(&L::Solid),
                        Fields::Bit(&L::Ruled), Fields::Bit(&L::VirtualGuide), Fields::Short(&L::Normal));
}

inline auto ReadProfiles(std::vector<Profile> &profiles, std::size_t count, const Fields::Input &in) -> bool
{
  profiles.assign(count, Profile{});
  return std::ranges::all_of(profiles, [&in](Profile &profile) -> bool { return ReadProfile(profile, in); });
}

/**
 *  The transform, the counts of sections and guides, a bit set when there is a path, the options, then the sections,
 *  the guides and the path.
 */
inline auto ReadLoft(Loft &loft, const Fields::Input &in) -> bool
{
  Fields::Input reading = in;
  if (!TransformMatrix(&Loft::Transform).Read(loft, reading))
  {
    return false;
  }
  const std::optional<std::size_t> sections = CheckCount(in.In->BL(), in);
  const std::optional<std::size_t> guides = CheckCount(in.In->BL(), in);
  const bool path = in.In->B();
  if (!sections || !guides || !Fields::Read(LoftLayout(), loft, reading) ||
      !ReadProfiles(loft.Sections, *sections, in) || !ReadProfiles(loft.Guides, *guides, in))
  {
    return false;
  }
  loft.Path.reset();
  return !path || ReadProfile(loft.Path.emplace(), in);
}

inline auto WriteLoft(const Loft &loft, const Fields::Output &to) -> bool
{
  Fields::Output writing = to;
  static_cast<void>(TransformMatrix(&Loft::Transform).Write(loft, writing));
  to.To->BL(static_cast<std::int32_t>(loft.Sections.size()));
  to.To->BL(static_cast<std::int32_t>(loft.Guides.size()));
  to.To->B(loft.Path.has_value());
  static_cast<void>(Fields::Write(LoftLayout(), loft, writing));
  for (const Profile &profile : loft.Sections)
  {
    WriteProfile(profile, to);
  }
  for (const Profile &profile : loft.Guides)
  {
    WriteProfile(profile, to);
  }
  if (loft.Path)
  {
    WriteProfile(*loft.Path, to);
  }
  return true;
}

inline auto ReadNurbs(NurbSurfaceData &tail, const Fields::Input &in) -> bool
{
  tail.Hull = in.In->BS();
  tail.Unknown1 = in.In->B();
  for (Vector &point : tail.Points)
  {
    point = in.In->Point3();
  }
  return !in.In->HasFailed();
}

inline auto WriteNurbs(const NurbSurfaceData &tail, const Fields::Output &to) -> bool
{
  to.To->BS(tail.Hull);
  to.To->B(tail.Unknown1);
  for (const Vector &point : tail.Points)
  {
    to.To->Point3(point);
  }
  return true;
}
} // namespace DwgIO::Entities::Shapes

#endif /* DWGIO_ENTITIES_SHAPES_PROCEDURAL_HPP */
