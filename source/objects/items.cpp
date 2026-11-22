/***********************************************************************************************************************
 *  @file      source/objects/items.cpp
 *  @brief     DXF groups read and written as an xrecord stores them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "objects/items.hpp"

#include "bits/byte.hpp"
#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "encoding/units.hpp"
#include "objects/groups.hpp"
#include "octets.hpp"

#include "dwgio/version.hpp"

#include <algorithm>
#include <climits>
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace DwgIO::Objects
{
namespace
{
using Bits::Byte;

constexpr std::size_t CodeBits = 16;

constexpr std::size_t MostChunk = 255;

constexpr std::size_t MostText = 65535;

/**
 *  A closed range.
 */
constexpr auto Within(int code, int first, int last) -> bool
{
  return code >= first && code <= last;
}

/**
 *  False when the value does not fit the bytes left or the code has no kind.
 */
struct Reader
{
  Bits::Reader *In{};
  const Fields::Context *With{};
  std::size_t End{};
  std::vector<DwgIO::Tag> *Groups{};

  [[nodiscard]] auto Remaining() const -> std::size_t { return End - In->Position(); }

  [[nodiscard]] auto Fits(std::size_t bits) const -> bool { return bits <= Remaining(); }

  void Add(int code, Content value) const { Groups->push_back(DwgIO::Tag{.Code = code, .Value = std::move(value)}); }

  [[nodiscard]] auto Text(int code) const -> bool
  {
    if (With->Release >= Version::R2007)
    {
      return Wide(code);
    }
    if (!Fits(std::size_t{3} * Byte))
    {
      return false;
    }
    const std::uint16_t count = In->RS();
    const int page = In->RC();
    if (!Fits(static_cast<std::size_t>(count) * Byte))
    {
      return false;
    }
    std::string raw;
    for (const std::byte one : In->Bytes(count))
    {
      raw.push_back(static_cast<char>(one));
    }
    Add(code, Encoding::Decode(SchemeOf(page, *With), raw));
    return true;
  }

  /**
   *  From R2007: the count of UTF-16 code units, then the units.
   */
  [[nodiscard]] auto Wide(int code) const -> bool
  {
    if (!Fits(std::size_t{2} * Byte))
    {
      return false;
    }
    const std::uint16_t count = In->RS();
    if (!Fits(static_cast<std::size_t>(count) * CodeBits))
    {
      return false;
    }
    std::u16string units;
    for (std::uint16_t at = 0; at < count; ++at)
    {
      units.push_back(In->RS());
    }
    Add(code, Encoding::FromUnits(units));
    return true;
  }

  [[nodiscard]] auto Chunk(int code) const -> bool
  {
    if (!Fits(Byte))
    {
      return false;
    }
    const std::uint8_t count = In->RC();
    if (!Fits(static_cast<std::size_t>(count) * Byte))
    {
      return false;
    }
    Add(code, DwgIO::Chunk{.Bytes = In->Bytes(count)});
    return true;
  }

  [[nodiscard]] auto HasRoom(int code, Kind kind, std::size_t bits) const -> bool
  {
    if (!Fits(bits))
    {
      return false;
    }
    switch (kind)
    {
    case Kind::Point:
      Add(code, In->RD());
      Add(code + Row, In->RD());
      Add(code + (2 * Row), In->RD());
      break;
    case Kind::Real:
      Add(code, In->RD());
      break;
    case Kind::Short:
      Add(code, std::int64_t{static_cast<std::int16_t>(In->RS())});
      break;
    case Kind::Long:
      Add(code, std::int64_t{static_cast<std::int32_t>(In->RL())});
      break;
    case Kind::LongLong:
      Add(code, static_cast<std::int64_t>(In->RLL()));
      break;
    case Kind::Byte:
      Add(code, std::int64_t{In->RC()});
      break;
    case Kind::Bool:
      Add(code, In->RC() != 0);
      break;
    default:
      Add(code, DwgIO::Handle{In->RLL()});
      break;
    }
    return true;
  }

  [[nodiscard]] auto One(int code) const -> bool
  {
    switch (KindOf(code))
    {
    case Kind::Text:
      return Text(code);
    case Kind::Point:
      return HasRoom(code, Kind::Point, std::size_t{3} * Byte * sizeof(double));
    case Kind::Real:
      return HasRoom(code, Kind::Real, Byte * sizeof(double));
    case Kind::Short:
      return HasRoom(code, Kind::Short, std::size_t{2} * Byte);
    case Kind::Long:
      return HasRoom(code, Kind::Long, std::size_t{4} * Byte);
    case Kind::LongLong:
      return HasRoom(code, Kind::LongLong, Byte * sizeof(std::int64_t));
    case Kind::Byte:
    case Kind::Bool:
      return HasRoom(code, KindOf(code), Byte);
    case Kind::Chunk:
      return Chunk(code);
    case Kind::Handle:
      return HasRoom(code, Kind::Handle, Byte * sizeof(std::uint64_t));
    default:
      return false;
    }
  }
};

struct Writer : GroupWriter
{
  void Text(const DwgIO::Tag &group) const
  {
    const auto *text = As<std::string>(group, "text");
    if (text != nullptr && To->With.Release >= Version::R2007)
    {
      std::u16string units = Encoding::ToUnits(*text);
      if (units.size() > MostText)
      {
        Note(*To, IssueCode::OutOfRange,
             std::format("XRECORD text of {} code units cut to {}", units.size(), MostText));
        units.resize(MostText);
      }
      Out->RS(static_cast<std::uint16_t>(group.Code));
      Out->RS(static_cast<std::uint16_t>(units.size()));
      for (const char16_t unit : units)
      {
        Out->RS(unit);
      }
      return;
    }
    if (text != nullptr)
    {
      const Encoding::Scheme scheme = To->With.Text != nullptr ? *To->With.Text : Encoding::Utf8();
      const std::string raw = Truncate(Encoding::Encode(scheme, *text), MostText, "text");
      Out->RS(static_cast<std::uint16_t>(group.Code));
      Out->RS(static_cast<std::uint16_t>(raw.size()));
      Out->RC(static_cast<std::uint8_t>(Encoding::NumberOf(scheme)));
      Bytes(raw);
    }
  }

  /**
   *  Returns how many groups the point took.
   */
  [[nodiscard]] auto Point(std::span<const DwgIO::Tag> groups, std::size_t at) const -> std::size_t
  {
    const DwgIO::Tag &group = groups[at];
    const auto *first = As<double>(group, "real");
    if (first == nullptr)
    {
      return 1;
    }
    const auto [values, taken] = PointAt(groups, at, *first);
    Out->RS(static_cast<std::uint16_t>(group.Code));
    for (const double value : values)
    {
      Out->RD(value);
    }
    return taken;
  }

  /**
   *  A value out of range gives the nearest, with a note.
   */
  [[nodiscard]] auto Clamp(std::int64_t value, std::pair<std::int64_t, std::int64_t> range, std::string_view kind) const
      -> std::int64_t
  {
    const std::int64_t fitted = std::clamp(value, range.first, range.second);
    if (fitted != value)
    {
      Note(*To, IssueCode::OutOfRange, std::format("XRECORD {} {} out of range, {} written", kind, value, fitted));
    }
    return fitted;
  }

  void Whole(const DwgIO::Tag &group, Kind kind) const
  {
    const auto *value = As<std::int64_t>(group, "number");
    if (value == nullptr)
    {
      return;
    }
    Out->RS(static_cast<std::uint16_t>(group.Code));
    if (kind == Kind::Short)
    {
      Out->RS(static_cast<std::uint16_t>(Clamp(*value, {INT16_MIN, INT16_MAX}, "short")));
    }
    else if (kind == Kind::Long)
    {
      Out->RL(static_cast<std::uint32_t>(Clamp(*value, {INT32_MIN, INT32_MAX}, "long")));
    }
    else if (kind == Kind::LongLong)
    {
      Out->RLL(static_cast<std::uint64_t>(*value));
    }
    else
    {
      Out->RC(static_cast<std::uint8_t>(Clamp(*value, {0, UINT8_MAX}, "byte")));
    }
  }

  /**
   *  Returns how many groups it took.
   */
  [[nodiscard]] auto One(std::span<const DwgIO::Tag> groups, std::size_t at) const -> std::size_t
  {
    const DwgIO::Tag &group = groups[at];
    const Kind kind = KindOf(group.Code);
    if (kind == Kind::Text)
    {
      Text(group);
    }
    else if (kind == Kind::Point)
    {
      return Point(groups, at);
    }
    else if (kind == Kind::Real)
    {
      if (const auto *real = As<double>(group, "real"))
      {
        Out->RS(static_cast<std::uint16_t>(group.Code));
        Out->RD(*real);
      }
    }
    else if (kind == Kind::Short || kind == Kind::Long || kind == Kind::LongLong || kind == Kind::Byte)
    {
      Whole(group, kind);
    }
    else if (kind == Kind::Bool)
    {
      if (const auto *flag = As<bool>(group, "bool"))
      {
        Out->RS(static_cast<std::uint16_t>(group.Code));
        Out->RC(*flag ? 1 : 0);
      }
    }
    else if (kind == Kind::Chunk)
    {
      if (const auto *chunk = As<DwgIO::Chunk>(group, "bytes"))
      {
        const std::string raw = Truncate(TextOf(chunk->Bytes), MostChunk, "chunk");
        Out->RS(static_cast<std::uint16_t>(group.Code));
        Out->RC(static_cast<std::uint8_t>(raw.size()));
        Bytes(raw);
      }
    }
    else if (kind == Kind::Handle)
    {
      if (const auto *handle = As<DwgIO::Handle>(group, "handle"))
      {
        Out->RS(static_cast<std::uint16_t>(group.Code));
        Out->RLL(handle->Value);
      }
    }
    else
    {
      Note(*To, IssueCode::NotKept, std::format("XRECORD group {} of no known kind not stored", group.Code));
    }
    return 1;
  }
};
} // namespace

auto KindOf(int code) -> Kind
{
  if (Within(code, 0, 9) || Within(code, 100, 109) || Within(code, 300, 309) || Within(code, 410, 419) ||
      Within(code, 430, 439) || Within(code, 470, 479) || Within(code, 1000, 1003))
  {
    return Kind::Text;
  }
  if (Within(code, 10, 18) || Within(code, 210, 239) || Within(code, 1010, 1013))
  {
    return Kind::Point;
  }
  if (Within(code, 38, 59) || Within(code, 110, 149) || Within(code, 460, 469) || Within(code, 1040, 1042))
  {
    return Kind::Real;
  }
  if (Within(code, 60, 79) || Within(code, 170, 179) || Within(code, 270, 279) || Within(code, 370, 389) ||
      Within(code, 400, 409) || code == 1070)
  {
    return Kind::Short;
  }
  if (Within(code, 90, 99) || Within(code, 420, 429) || Within(code, 440, 459) || code == 1071)
  {
    return Kind::Long;
  }
  if (Within(code, 160, 169))
  {
    return Kind::LongLong;
  }
  if (Within(code, 280, 289))
  {
    return Kind::Byte;
  }
  if (Within(code, 290, 299))
  {
    return Kind::Bool;
  }
  if (Within(code, 310, 319) || code == 1004)
  {
    return Kind::Chunk;
  }
  if (Within(code, 320, 369) || Within(code, 390, 399) || Within(code, 480, 481) || code == 1005)
  {
    return Kind::Handle;
  }
  return Kind::None;
}

auto SchemeOf(int page, const Fields::Context &with) -> Encoding::Scheme
{
  if (const std::optional<Encoding::Scheme> named = Encoding::FindScheme(page))
  {
    return *named;
  }
  return with.Text != nullptr ? *with.Text : Encoding::Utf8();
}

auto ReadGroups(const Fields::Input &in, std::size_t bytes)
    -> std::pair<std::vector<DwgIO::Tag>, std::vector<std::byte>>
{
  std::vector<DwgIO::Tag> groups;
  std::vector<std::byte> rest;
  const std::size_t end = in.In->Position() + (bytes * Byte);
  const Reader reader{.In = in.In, .With = &in.With, .End = end, .Groups = &groups};
  while (in.In->Position() < end)
  {
    const std::size_t start = in.In->Position();
    const std::size_t before = groups.size();
    const bool coded = reader.Fits(CodeBits);
    const int code = coded ? static_cast<std::int16_t>(in.In->RS()) : -1;
    if (coded && reader.One(code) && !in.In->HasFailed())
    {
      continue;
    }
    groups.resize(before);
    if (!in.In->HasFailed())
    {
      const bool unknown = coded && KindOf(code) == Kind::None;
      Note(in, unknown ? IssueCode::Unknown : IssueCode::Truncated,
           unknown ? std::format("XRECORD group {} of no known kind", code)
                   : std::format("XRECORD group {} cut short", code));
      in.In->Seek(start);
      rest = in.In->Bytes((end - start) / Byte);
    }
    break;
  }
  return {std::move(groups), std::move(rest)};
}

void WriteGroups(std::span<const DwgIO::Tag> groups, std::span<const std::byte> rest, const Fields::Output &to)
{
  const Writer writer{{.Out = to.To, .To = &to, .Subject = "XRECORD"}};
  for (std::size_t at = 0; at < groups.size();)
  {
    at += writer.One(groups, at);
  }
  to.To->Raw(rest);
}
} // namespace DwgIO::Objects
