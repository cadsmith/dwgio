/***********************************************************************************************************************
 *  @file      source/objects/read.cpp
 *  @brief     One object read from its offset: the header, the common data and its kind's fields.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "objects/read.hpp"

#include "bits/byte.hpp"
#include "bits/crc.hpp"
#include "bits/reader.hpp"
#include "bits/reference.hpp"
#include "bits/strings.hpp"
#include "fields/context.hpp"
#include "fields/layout.hpp"
#include "fields/opaque.hpp"
#include "objects/eed.hpp"
#include "objects/kinds.hpp"
#include "objects/types.hpp"

#include <array>
#include <expected>
#include <format>
#include <string>
#include <type_traits>
#include <utility>

namespace DwgIO::Objects
{
namespace
{
using Bits::Byte;

constexpr std::size_t Crc = 2;

constexpr int Named = 3;

struct Entity
{
  int Mode{};
  std::int32_t Reactors{};
  bool ByLayerLinetype{true};
  bool NoLinks{true};
  bool NoDictionary{}; /**< From R2004: 1 when no dictionary handle follows the reactors. */
  int LinetypeFlags{};
  int PlotstyleFlags{};
  std::array<bool, 3> Styles{}; /**< From R2010, which visual styles follow in the handles: full, face, edge. */
  std::size_t Handles{};
};

/**
 *  What an object and an entity share: the reactor count, from R2004 the bit set when no dictionary handle follows, and
 *  from R2013 the data store bit.
 */
auto ReadOwned(Bits::Reader &in, Base &base, Version release) -> Entity
{
  Entity entity;
  entity.Reactors = in.BL();
  if (release >= Version::R2004)
  {
    entity.NoDictionary = in.B();
  }
  if (release >= Version::R2013)
  {
    base.Stored = in.B();
  }
  return entity;
}

/**
 *  Chapter 20.4.1, from after the extended data to the entity's own data.
 */
auto ReadEntity(Bits::Reader &in, Unknown &out, Version release) -> Entity
{
  Entity entity;
  if (in.B())
  {
    const auto graphics = release >= Version::R2010 ? static_cast<std::uint64_t>(in.BLL()) : in.RL();
    out.Common.Graphics = in.Bytes(static_cast<std::size_t>(graphics));
  }
  if (release < Version::R2000)
  {
    entity.Handles = in.RL();
  }
  entity.Mode = in.BB();
  out.Common.Space = static_cast<Space>(entity.Mode);
  const Entity owned = ReadOwned(in, out.Base, release);
  entity.Reactors = owned.Reactors;
  entity.NoDictionary = owned.NoDictionary;
  if (release < Version::R2000)
  {
    entity.ByLayerLinetype = in.B();
    out.Common.LinetypeOrigin = entity.ByLayerLinetype ? Origin::ByLayer : Origin::Record;
  }
  // One bit follows the reactors: the links bit through R2000; from R2004, when there are no links, the bit that
  // says no dictionary handle follows. Chapter 20.4.1 lists both; AutoCAD's files hold one.
  if (release < Version::R2004)
  {
    entity.NoLinks = in.B();
  }
  out.Common.Color = release >= Version::R2000 ? in.ENC(release) : in.CMC(release);
  out.Common.LinetypeScale = in.BD();
  if (release >= Version::R2000)
  {
    entity.LinetypeFlags = in.BB();
    entity.PlotstyleFlags = in.BB();
    out.Common.LinetypeOrigin = static_cast<Origin>(entity.LinetypeFlags);
    out.Common.PlotstyleOrigin = static_cast<Origin>(entity.PlotstyleFlags);
  }
  if (release >= Version::R2007)
  {
    out.Common.MaterialOrigin = static_cast<Origin>(in.BB());
    out.Common.ShadowFlags = in.RC();
  }
  if (release >= Version::R2010)
  {
    for (bool &style : entity.Styles)
    {
      style = in.B();
    }
  }
  out.Common.Invisible = (static_cast<unsigned>(in.BS()) & 1U) != 0;
  if (release >= Version::R2000)
  {
    out.Common.Lineweight = static_cast<Lineweight>(in.RC());
  }
  return entity;
}

struct Handles
{
  Bits::Reader *In{};
  Unknown *Out{};
  Version Release{};

  [[nodiscard]] auto Take() const -> Handle { return Bits::Resolve(In->H(), Out->Base.Self); }

  void Common(const Entity &entity) const
  {
    if (!Out->Entity || entity.Mode == 0)
    {
      Out->Base.Owner = Take();
    }
    for (std::int32_t at = 0; at < entity.Reactors && !In->HasFailed(); ++at)
    {
      Out->Base.Reactors.push_back(Take());
    }
    if (!entity.NoDictionary)
    {
      Out->Base.Dictionary = Take();
    }
  }

  /**
   *  As stored, or as the file assumes them: the handles next to the entity's own; none from R2004.
   */
  void Links(const Objects::Entity &entity) const
  {
    if (Release >= Version::R2004)
    {
      return; // No links: the block record lists its entities.
    }
    if (!entity.NoLinks)
    {
      Out->Common.Previous = Take();
      Out->Common.Next = Take();
      return;
    }
    Out->Common.Previous = Handle{Out->Base.Self.Value - 1};
    Out->Common.Next = Handle{Out->Base.Self.Value + 1};
  }

  /**
   *  R13 and R14 put the layer and linetype before the links, R2000 after them, as chapter 20.4.2 lists them.
   */
  void Entity(const Objects::Entity &entity) const
  {
    if (Release >= Version::R2000)
    {
      Links(entity);
    }
    Out->Common.Layer = Take();
    if (Release >= Version::R2004 && Out->Common.Color.Reference)
    {
      Out->Common.Color.Reference = Take();
    }
    const bool linetype = Release >= Version::R2000 ? entity.LinetypeFlags == Named : !entity.ByLayerLinetype;
    if (linetype)
    {
      Out->Common.Linetype = Take();
    }
    if (Release >= Version::R2007 && Out->Common.MaterialOrigin == Origin::Record)
    {
      Out->Common.Material = Take();
    }
    if (Release >= Version::R2007 && Out->Common.ShadowFlags == Named)
    {
      Out->Common.Shadow = Take();
    }
    if (Release >= Version::R2000 && entity.PlotstyleFlags == Named)
    {
      Out->Common.Plotstyle = Take();
    }
    const std::array<Handle *, 3> styles{&Out->Common.FullStyle, &Out->Common.FaceStyle, &Out->Common.EdgeStyle};
    for (std::size_t at = 0; at < styles.size(); ++at)
    {
      if (entity.Styles.at(at))
      {
        *styles.at(at) = Take();
      }
    }
    if (Release < Version::R2000)
    {
      Links(entity);
    }
  }
};

struct Extent
{
  std::size_t Size{};
  std::size_t Start{};
  std::uint64_t Handles{}; /**< From R2010, the handle stream's size in bits. */
};

/**
 *  Nothing, with an issue, when the file ends first.
 *
 *  From R2010 the handle stream's size follows the object's, which does not count it.
 */
auto ReadExtent(std::span<const std::byte> file, std::size_t offset, const FileContext &with) -> std::optional<Extent>
{
  std::vector<Issue> *issues = with.Issues;
  const auto note = [issues, offset](std::string what) -> std::nullopt_t
  {
    if (issues != nullptr)
    {
      issues->push_back(Issue{.Code = IssueCode::Damaged, .Offset = offset, .What = std::move(what)});
    }
    return std::nullopt;
  };
  if (offset >= file.size())
  {
    return note(std::format("object at {} beyond the file", offset));
  }
  Bits::Reader sizer{file.subspan(offset)};
  const std::uint64_t size = sizer.MS();
  const std::uint64_t handles = with.Release >= Version::R2010 ? sizer.UMC() : 0;
  const std::size_t start = offset + (sizer.Position() / Byte);
  if (sizer.HasFailed() || size > file.size() - start || file.size() - start - size < Crc)
  {
    return note(std::format("object at {} runs past the end", offset));
  }
  return Extent{.Size = static_cast<std::size_t>(size), .Start = start, .Handles = handles};
}

/**
 *  An object opened as far as its common header: the readers stand at its kind's own data and handles.
 */
struct Opened
{
  Unknown Head;
  Bits::Reader *Data{};
  Bits::Reader *Stream{};
  Bits::Reader *Strings{};
  const Fields::Context *Context{};
  std::size_t DataEnd{};
  std::size_t StringsEnd{};

  /**
   *  Nothing for a type the library leaves or a kind whose data says it is of a form its layout does not read.
   */
  [[nodiscard]] auto Kind() const -> std::optional<Object>
  {
    std::optional<Object> result;
    const auto read = [this, &result](auto tag) -> void
    {
      using KindT = decltype(tag)::type;
      KindT kind{};
      BaseOfKind(kind) = Head.Base; // Before the layout, which may ask the extended data what it holds.
      if constexpr (requires { Shape<KindT>::Open(kind, Head.Type); })
      {
        Shape<KindT>::Open(kind, Head.Type);
      }
      if constexpr (requires { Shape<KindT>::Open(kind, Head.Class.View()); })
      {
        Shape<KindT>::Open(kind, Head.Class.View());
      }
      Fields::Input reading{.In = Data,
                            .Handles = Stream,
                            .Strings = Strings,
                            .With = *Context,
                            .DataEnd = DataEnd,
                            .StringsEnd = StringsEnd};
      static_cast<void>(Fields::Read(Shape<KindT>::Layout(), kind, reading));
      if constexpr (requires { Shape<KindT>::IsReadable(kind); })
      {
        if (!Shape<KindT>::IsReadable(kind)) // What the kind's layout cannot read stays the object's bytes.
        {
          return;
        }
      }
      if constexpr (requires { kind.Common; })
      {
        CommonOfKind(kind) = Head.Common;
      }
      result = std::move(kind);
    };
    static_cast<void>(Head.Type >= FirstClass ? VisitClass(Head.Class.View(), read) : VisitType(Head.Type, read));
    return result;
  }
};
} // namespace

namespace
{
/**
 *  An object or entity read up to its own data.
 */
struct UnknownSource
{
  Unknown *Out{};
  Bits::Reader *In{};
  const FileContext *With{};
  const Fields::Context *Context{};
  Extent Where{};

  void Note(IssueCode code, std::string what) const
  {
    DwgIO::Fields::Note(Fields::Input{.In = In, .With = *Context}, code, std::move(what));
  }

  /**
   *  Given by an RL from R2000 to R2007, counted back from the end from R2010.
   */
  [[nodiscard]] auto HandlesAt() const -> std::size_t
  {
    const std::size_t bits = Where.Size * Byte;
    if (With->Release >= Version::R2010)
    {
      return Where.Handles > bits ? bits + 1 : bits - static_cast<std::size_t>(Where.Handles);
    }
    return With->Release >= Version::R2000 ? In->RL() : 0;
  }

  void Classify() const
  {
    if (Out->Type < FirstClass)
    {
      Out->Entity = IsEntity(Out->Type);
      return;
    }
    const std::size_t index = Out->Type - FirstClass;
    if (index >= With->Classes.size())
    {
      Note(IssueCode::Damaged, std::format("type {} with no class", Out->Type));
      return;
    }
    Out->Class = Symbol{With->Classes[index].DxfName};
    Out->Entity = With->Classes[index].ItemClass == EntityClass;
  }

  /**
   *  An entity's, or an object's reactors; with the handle size up to R14.
   */
  [[nodiscard]] auto Common() const -> Entity
  {
    if (Out->Entity)
    {
      return ReadEntity(*In, *Out, With->Release);
    }
    const std::size_t handles = With->Release < Version::R2000 ? In->RL() : 0;
    Entity entity = ReadOwned(*In, Out->Base, With->Release);
    entity.Handles = handles;
    return entity;
  }
};

/**
 *  From R2007; a string stream that reaches before the data is noted.
 */
auto Strings(std::span<const std::byte> data, std::size_t handles, const UnknownSource &header)
    -> std::optional<Bits::Strung>
{
  if (header.With->Release < Version::R2007)
  {
    return std::nullopt;
  }
  const std::expected<std::optional<Bits::Strung>, std::string> found = Bits::FindStrings(data, handles);
  if (!found)
  {
    header.Note(IssueCode::Damaged, std::format("object {} {}", HexOf(header.Out->Base.Self), found.error()));
  }
  return found.value_or(std::nullopt);
}

/**
 *  At the string stream, else from R2007 at the bit before the handles that says there is none, else at the handles.
 */
auto DataEnds(std::size_t handles, const std::optional<Bits::Strung> &strung, Version release) -> std::size_t
{
  if (strung)
  {
    return strung->Start;
  }
  return release >= Version::R2007 && handles > 0 ? handles - 1 : handles;
}

/**
 *  The rest of the handle stream without the padding: handles are read while a byte remains, and what is left, fewer
 *  bits than a byte, is the padding. The whole rest when it does not read as handles to the end.
 */
auto TrimPadding(Bits::Reader &stream) -> BitString
{
  Bits::Reader ahead = stream;
  while (ahead.Size() - ahead.Position() >= Byte) // A handle that fails leaves the reader at its end.
  {
    static_cast<void>(ahead.H());
  }
  return Fields::ReadRest(stream, ahead.HasFailed() ? 0 : ahead.Position());
}

/**
 *  As Unknown::Body keeps them; the data, handle and string readers stand where they start.
 */
auto Own(const Fields::Input &where) -> Opaque
{
  Opaque own{
      .Release = where.With.Release, .Data = Fields::ReadRest(*where.In, where.DataEnd), .Handles = {}, .Strings = {}};
  own.Handles = TrimPadding(*where.Handles);
  if (where.Strings != nullptr)
  {
    own.Strings = Fields::ReadRest(*where.Strings, where.StringsEnd);
  }
  return own;
}
} // namespace

auto ReadObject(std::span<const std::byte> file, std::size_t offset, const FileContext &with) -> std::optional<Object>
{
  const std::optional<Extent> sized = ReadExtent(file, offset, with);
  if (!sized)
  {
    return std::nullopt;
  }
  const auto [size, start, streamed] = *sized;
  Unknown out;
  const std::span<const std::byte> whole = file.subspan(offset, start - offset + size + Crc);
  const std::span<const std::byte> data = file.subspan(start, size);
  Bits::Reader in{data};
  Fields::Context context{.Release = with.Release, .Text = with.Text, .Issues = with.Issues, .Offset = start};
  const UnknownSource header{.Out = &out, .In = &in, .With = &with, .Context = &context, .Where = *sized};
  const auto kept = [&out, whole] -> Object
  {
    out.Bytes.assign(whole.begin(), whole.end());
    return out;
  };
  out.Type = static_cast<std::uint16_t>(in.OT(with.Release));
  std::size_t handles = header.HandlesAt();
  out.Base.Self = in.H().Value;
  context.Record = out.Base.Self;
  out.Base.Extended = ReadEed(in, context);
  header.Classify();
  const Entity entity = header.Common();
  handles = with.Release < Version::R2000 ? entity.Handles : handles;
  if (in.HasFailed() || handles > size * Byte)
  {
    header.Note(in.HasFailed() ? IssueCode::Truncated : IssueCode::Damaged,
                std::format("object {} {}", HexOf(out.Base.Self),
                            in.HasFailed() ? "data cut short" : "handles beyond its data"));
    return kept();
  }
  const std::optional<Bits::Strung> strung = Strings(data, handles, header);
  std::optional<Bits::Reader> strings = Bits::MakeStringReader(data, strung);
  Bits::Reader stream{data};
  stream.Seek(handles);
  const Handles reader{.In = &stream, .Out = &out, .Release = with.Release};
  reader.Common(entity);
  if (out.Entity)
  {
    reader.Entity(entity);
  }
  const Opened opened{.Head = out,
                      .Data = &in,
                      .Stream = &stream,
                      .Strings = strings ? &*strings : nullptr,
                      .Context = &context,
                      .DataEnd = DataEnds(handles, strung, with.Release),
                      .StringsEnd = strung ? strung->End : 0};
  const std::size_t dataAt = in.Position();
  const std::size_t handlesAt = stream.Position();
  const std::size_t stringsAt = strings ? strings->Position() : 0;
  std::optional<Object> kind = opened.Kind();
  if (!kind && !stream.HasFailed())
  {
    in.Seek(dataAt);
    stream.Seek(handlesAt);
    if (strings)
    {
      strings->Seek(stringsAt);
    }
    out.Body = Own(Fields::Input{.In = &in,
                                 .Handles = &stream,
                                 .Strings = strings ? &*strings : nullptr,
                                 .With = context,
                                 .DataEnd = opened.DataEnd,
                                 .StringsEnd = opened.StringsEnd});
  }
  if (kind && in.HasFailed())
  {
    header.Note(IssueCode::Truncated, std::format("object {} data cut short", HexOf(out.Base.Self)));
  }
  if (stream.HasFailed())
  {
    Note(Fields::Input{.In = &stream, .With = context}, IssueCode::Truncated,
         std::format("object {} handles cut short", HexOf(out.Base.Self)));
  }
  Bits::Reader tail{file.subspan(start + size, Crc)};
  if (with.Issues != nullptr && Bits::Crc(file.subspan(offset, start - offset + size), Bits::Seed) != tail.RS())
  {
    with.Issues->push_back(Issue{.Code = IssueCode::Checksum,
                                 .Offset = offset,
                                 .Subject = out.Base.Self,
                                 .What = std::format("object {} CRC mismatch", HexOf(out.Base.Self))});
  }
  return kind ? std::move(*kind) : kept();
}
} // namespace DwgIO::Objects
