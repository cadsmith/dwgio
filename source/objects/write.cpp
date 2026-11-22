/***********************************************************************************************************************
 *  @file      source/objects/write.cpp
 *  @brief     One object written in a release: its streams, size and CRC.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "objects/write.hpp"

#include "bits/crc.hpp"
#include "bits/reference.hpp"
#include "bits/strings.hpp"
#include "bits/writer.hpp"
#include "fields/layout.hpp"
#include "fields/opaque.hpp"
#include "objects/common.hpp"
#include "objects/eed.hpp"
#include "objects/kinds.hpp"

#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/version.hpp"

#include <cstddef>
#include <cstdint>
#include <format>
#include <type_traits>
#include <variant>

namespace DwgIO::Objects
{
namespace
{
constexpr int SelfCode = 0;

constexpr int SoftPointer = 4;

constexpr int HardOwner = 3;

constexpr std::size_t SizeBits = 32;

/**
 *  The head holds the handle, the extended data and the proxy graphics; the strings are from R2007.
 */
struct Streams
{
  Bits::Writer Head;
  Bits::Writer Body;
  Bits::Writer Handles;
  Bits::Writer Strings;
};

/**
 *  A bit, and when set the size, an RL up to R2007 and a BLL from R2010, and the bytes.
 */
void WriteGraphics(const Common &common, Version release, Bits::Writer &head)
{
  head.B(!common.Graphics.empty());
  if (common.Graphics.empty())
  {
    return;
  }
  if (release >= Version::R2010)
  {
    head.BLL(static_cast<std::int64_t>(common.Graphics.size()));
  }
  else
  {
    head.RL(static_cast<std::uint32_t>(common.Graphics.size()));
  }
  head.Raw(common.Graphics);
}

/**
 *  The reactor count, from R2004 the bit set when no dictionary handle follows, from R2013 the data store bit; then the
 *  owner, reactors and dictionary in the handle stream.
 */
void WriteOwned(const Base &base, Version release, Streams &streams)
{
  streams.Body.BL(static_cast<std::int32_t>(base.Reactors.size()));
  const bool dictionary = release < Version::R2004 || base.Dictionary != Handle{};
  if (release >= Version::R2004)
  {
    streams.Body.B(!dictionary);
  }
  if (release >= Version::R2013)
  {
    streams.Body.B(base.Stored);
  }
  streams.Handles.H(Bits::Relative(base.Owner, base.Self));
  for (const Handle reactor : base.Reactors)
  {
    streams.Handles.H(Bits::Reference{.Code = SoftPointer, .Value = reactor});
  }
  if (dictionary)
  {
    streams.Handles.H(Bits::Reference{.Code = HardOwner, .Value = base.Dictionary});
  }
}

/**
 *  The size, from R2010 the handle stream's size, then the type, the head, the data and strings and the handles filled
 *  to the byte, then the CRC. The bit size follows the type from R2000 to R2007; R13 and R14 put it after the head.
 */
auto Frame(std::uint16_t code, Version release, Streams &streams) -> std::vector<std::byte>
{
  const bool older = release < Version::R2000;
  const bool sized = release < Version::R2010;
  if (release >= Version::R2007)
  {
    streams.Body.Append(Bits::WriteStringTail(streams.Strings));
  }
  Bits::Writer data;
  data.OT(code, release);
  const auto size = static_cast<std::uint32_t>(data.Position() + streams.Head.Position() + (sized ? SizeBits : 0) +
                                               streams.Body.Position());
  if (sized && !older)
  {
    data.RL(size);
  }
  data.Append(streams.Head);
  if (older)
  {
    data.RL(size);
  }
  data.Append(streams.Body);
  data.Append(streams.Handles);
  data.Fill();
  Bits::Writer whole;
  whole.MS(data.Bytes().size());
  if (!sized)
  {
    whole.UMC(data.Position() - size);
  }
  whole.Raw(data.Bytes());
  whole.RS(Bits::Crc(whole.Bytes(), Bits::Seed));
  return whole.Take();
}

/**
 *  Common is null for an object.
 */
struct Header
{
  const DwgIO::Base *Base{};
  const DwgIO::Common *Common{};
};

void WriteHeader(const Header &header, const Fields::Output &writing, Streams &streams)
{
  const Base &base = *header.Base;
  streams.Head.H(Bits::Reference{.Code = SelfCode, .Value = base.Self});
  const Fields::Output heading{.To = &streams.Head, .Handles = &streams.Handles, .With = writing.With};
  WriteEed(base.Extended, heading);
  if (header.Common != nullptr)
  {
    WriteGraphics(*header.Common, writing.With.Release, streams.Head);
    WriteCommon(*header.Common, base, writing);
  }
  else
  {
    WriteOwned(base, writing.With.Release, streams);
  }
}

/**
 *  An untyped object of another release: its common header as this release holds it, then its own bits as they were,
 *  which the save does only for a class AutoCAD writes the same way in both releases.
 */
auto RewriteUnknown(const Unknown &unknown, const Opaque &own, const Fields::Context &with) -> std::vector<std::byte>
{
  const Version release = with.Release;
  Streams streams;
  Fields::Context context = with;
  context.Record = unknown.Base.Self;
  const Fields::Output writing{.To = &streams.Body,
                               .Handles = &streams.Handles,
                               .Strings = release >= Version::R2007 ? &streams.Strings : nullptr,
                               .With = context};
  WriteHeader(Header{.Base = &unknown.Base, .Common = unknown.Entity ? &unknown.Common : nullptr}, writing, streams);
  Fields::Put(own.Data, streams.Body);
  Fields::Put(own.Handles, streams.Handles);
  Fields::Put(own.Strings, streams.Strings);
  return Frame(unknown.Type, release, streams);
}

template <typename KindT>
auto WriteKind(const KindT &kind, const Fields::Context &with) -> std::vector<std::byte>
{
  const Base &base = BaseOfKind(kind);
  const Version release = with.Release;
  Streams streams;
  Fields::Context context = with;
  context.Record = base.Self;
  Fields::Output writing{.To = &streams.Body,
                         .Handles = &streams.Handles,
                         .Strings = release >= Version::R2007 ? &streams.Strings : nullptr,
                         .With = context};
  if constexpr (requires { kind.Common; })
  {
    WriteHeader(Header{.Base = &base, .Common = &CommonOfKind(kind)}, writing, streams);
  }
  else
  {
    WriteHeader(Header{.Base = &base, .Common = nullptr}, writing, streams);
  }
  static_cast<void>(Fields::Write(Shape<KindT>::Layout(), kind, writing));
  const std::uint16_t code = CodeOf(kind, context);
  return code == 0 ? std::vector<std::byte>{} : Frame(code, release, streams);
}
} // namespace

auto WriteObject(const Object &object, const Fields::Context &with) -> std::vector<std::byte>
{
  return Visit(
      [&with](const auto &kind) -> std::vector<std::byte>
      {
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(kind)>, Unknown>)
        {
          // Bytes as read, unless they are own bits of another release.
          if (!kind.Bytes.empty() && (!kind.Body || kind.Body->Release == with.Release))
          {
            return kind.Bytes;
          }
          if (kind.Body && kind.Body->Release == with.Release)
          {
            return RewriteUnknown(kind, *kind.Body, with);
          }
          if (with.Issues != nullptr)
          {
            with.Issues->push_back(
                Issue{.Code = IssueCode::NotKept,
                      .Subject = kind.Base.Self,
                      .What = std::format("untyped object {} of another release not written", HexOf(kind.Base.Self))});
          }
          return {};
        }
        else
        {
          return WriteKind(kind, with);
        }
      },
      object);
}
} // namespace DwgIO::Objects
