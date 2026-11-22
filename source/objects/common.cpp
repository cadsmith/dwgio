/***********************************************************************************************************************
 *  @file      source/objects/common.cpp
 *  @brief     The common entity data and handles written, chapters 20.4.1 and 20.4.2.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "objects/common.hpp"

#include "bits/reference.hpp"
#include "bits/writer.hpp"

#include "dwgio/version.hpp"

#include <array>
#include <cstdint>
#include <utility>

namespace DwgIO::Objects
{
namespace
{
constexpr int SoftPointer = 4;

constexpr int HardOwner = 3;

constexpr int HardPointer = 5;

constexpr int Named = 3;
} // namespace

namespace
{
struct Shared
{
  const Common *Of{};
  const Base *Owned{};
  Version Release{};
  bool Older{};
  bool Newer{};
  bool Dictionary{};
  bool Assumed{};

  [[nodiscard]] auto Styles() const -> std::array<Handle, 3> { return {Of->FullStyle, Of->FaceStyle, Of->EdgeStyle}; }
};

/**
 *  Chapter 20.4.1, after the proxy graphics.
 */
void WriteData(const Shared &shared, Bits::Writer &body)
{
  const Common &common = *shared.Of;
  const Version release = shared.Release;
  body.BB(std::to_underlying(common.Space));
  body.BL(static_cast<std::int32_t>(shared.Owned->Reactors.size()));
  // One bit: through R2000 whether the links are the handles next to the entity's own; from R2004, which has no
  // links, whether no dictionary handle follows the reactors; then from R2013 the data store bit.
  if (shared.Newer)
  {
    body.B(!shared.Dictionary);
  }
  if (release >= Version::R2013)
  {
    body.B(shared.Owned->Stored);
  }
  if (shared.Older)
  {
    body.B(common.LinetypeOrigin ==
           Origin::ByLayer); // R13 and R14 say ByLayer in a bit and name every other linetype's record.
  }
  if (!shared.Newer)
  {
    body.B(shared.Assumed);
  }
  if (shared.Older)
  {
    body.CMC(common.Color, release);
  }
  else
  {
    body.ENC(common.Color, release);
  }
  body.BD(common.LinetypeScale);
  if (!shared.Older)
  {
    body.BB(std::to_underlying(common.LinetypeOrigin));
    body.BB(std::to_underlying(common.PlotstyleOrigin));
  }
  if (release >= Version::R2007)
  {
    body.BB(std::to_underlying(common.MaterialOrigin));
    body.RC(static_cast<std::uint8_t>(common.ShadowFlags));
  }
  for (const Handle style : shared.Styles())
  {
    if (release >= Version::R2010)
    {
      body.B(style != Handle{});
    }
  }
  body.BS(common.Invisible ? 1 : 0);
  if (!shared.Older)
  {
    body.RC(std::to_underlying(common.Lineweight.value_or(Lineweight::ByLayer)));
  }
}

void Pointer(Bits::Writer &handles, bool stored, Handle value)
{
  if (stored)
  {
    handles.H(Bits::Reference{.Code = HardPointer, .Value = value});
  }
}

void WriteHandles(const Shared &shared, Bits::Writer &handles)
{
  const Common &common = *shared.Of;
  const Base &base = *shared.Owned;
  const Version release = shared.Release;
  const Handle self = base.Self;
  if (common.Space == Space::Owner)
  {
    handles.H(Bits::Relative(base.Owner, self));
  }
  for (const Handle reactor : base.Reactors)
  {
    handles.H(Bits::Reference{.Code = SoftPointer, .Value = reactor});
  }
  if (shared.Dictionary)
  {
    handles.H(Bits::Reference{.Code = HardOwner, .Value = base.Dictionary});
  }
  const auto links = [&handles, &common, self] -> void
  {
    handles.H(Bits::Relative(common.Previous, self));
    handles.H(Bits::Relative(common.Next, self));
  };
  if (!shared.Older && !shared.Newer && !shared.Assumed)
  {
    links();
  }
  handles.H(Bits::Reference{.Code = HardPointer, .Value = common.Layer});
  Pointer(handles, shared.Newer && common.Color.Reference, common.Color.Reference.value_or(Handle{}));
  Pointer(handles, shared.Older ? common.LinetypeOrigin != Origin::ByLayer : common.LinetypeOrigin == Origin::Record,
          common.Linetype);
  Pointer(handles, release >= Version::R2007 && common.MaterialOrigin == Origin::Record, common.Material);
  Pointer(handles, release >= Version::R2007 && common.ShadowFlags == Named, common.Shadow);
  Pointer(handles, !shared.Older && common.PlotstyleOrigin == Origin::Record, common.Plotstyle);
  for (const Handle style : shared.Styles())
  {
    Pointer(handles, release >= Version::R2010 && style != Handle{}, style);
  }
  if (shared.Older && !shared.Assumed)
  {
    links();
  }
}
} // namespace

void WriteCommon(const Common &common, const Base &base, const Fields::Output &to)
{
  const Version release = to.With.Release;
  const bool newer = release >= Version::R2004;
  const Shared shared{.Of = &common,
                      .Owned = &base,
                      .Release = release,
                      .Older = release < Version::R2000,
                      .Newer = newer,
                      .Dictionary = !newer || base.Dictionary != Handle{},
                      .Assumed =
                          common.Previous == Handle{base.Self.Value - 1} && common.Next == Handle{base.Self.Value + 1}};
  WriteData(shared, *to.To);
  WriteHandles(shared, *to.Handles);
}
} // namespace DwgIO::Objects
