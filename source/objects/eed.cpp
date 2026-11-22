/***********************************************************************************************************************
 *  @file      source/objects/eed.cpp
 *  @brief     An object's extended data read and written, chapter 28.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "objects/eed.hpp"

#include "bits/byte.hpp"
#include "encoding/encoding.hpp"
#include "encoding/units.hpp"
#include "objects/groups.hpp"
#include "objects/items.hpp"
#include "octets.hpp"

#include "dwgio/tag.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace DwgIO::Objects
{
namespace
{
using Bits::Byte;

constexpr int Group1000 = 1000;

constexpr std::uint8_t StringCode = 0;

constexpr std::uint8_t BraceCode = 2;

constexpr std::uint8_t LayerCode = 3;

constexpr std::uint8_t BinaryCode = 4;

constexpr std::uint8_t EntityCode = 5;

constexpr std::uint8_t FirstPoint = 10;

constexpr std::uint8_t LastPoint = 13;

constexpr std::uint8_t FirstReal = 40;

constexpr std::uint8_t LastReal = 42;

constexpr std::uint8_t ShortCode = 70;

constexpr std::uint8_t LongCode = 71;

struct Item
{
  Bits::Reader *In{};
  const Fields::Context *With{};
  std::vector<DwgIO::Tag> *Groups{};

  void Add(int code, Content value) const
  {
    Groups->push_back(DwgIO::Tag{.Code = Group1000 + code, .Value = std::move(value)});
  }

  void String() const
  {
    if (With->Release >= Version::R2007)
    {
      const std::uint16_t count = In->RS();
      std::u16string units;
      for (std::uint16_t at = 0; at < count && !In->HasFailed(); ++at)
      {
        units.push_back(In->RS());
      }
      Add(StringCode, Encoding::FromUnits(units));
      return;
    }
    const std::uint8_t count = In->RC();
    const auto high = static_cast<unsigned>(In->RC());
    const auto page = static_cast<int>((high << Byte) | static_cast<unsigned>(In->RC()));
    Add(StringCode, Encoding::Decode(Scheme(page), Text(count)));
  }

  /**
   *  The code page a string item names, else the file's, else UTF-8.
   */
  [[nodiscard]] auto Scheme(int page) const -> Encoding::Scheme { return SchemeOf(page, *With); }

  void Handle(std::uint8_t code) const { Add(code, DwgIO::Handle{std::byteswap(In->RLL())}); }

  void Binary() const { Add(BinaryCode, Chunk{BytesOf(Text(In->RC()))}); }

  /**
   *  Fewer characters when the reader fails.
   */
  [[nodiscard]] auto Text(std::uint8_t count) const -> std::string
  {
    std::string text;
    for (const std::byte one : In->Bytes(count))
    {
      text.push_back(static_cast<char>(one));
    }
    return text;
  }

  void Point(std::uint8_t code) const
  {
    Add(code, In->RD());
    Add(code + Row, In->RD());
    Add(code + (2 * Row), In->RD());
  }

  [[nodiscard]] auto One() const -> bool
  {
    const std::uint8_t code = In->RC();
    if (code == StringCode)
    {
      String();
    }
    else if (code == BraceCode)
    {
      Add(BraceCode, std::string{In->RC() == 0 ? "{" : "}"});
    }
    else if (code == LayerCode || code == EntityCode)
    {
      Handle(code);
    }
    else if (code == BinaryCode)
    {
      Binary();
    }
    else if (code >= FirstPoint && code <= LastPoint)
    {
      Point(code);
    }
    else if (code >= FirstReal && code <= LastReal)
    {
      Add(code, In->RD());
    }
    else if (code == ShortCode)
    {
      Add(code, std::int64_t{static_cast<std::int16_t>(In->RS())});
    }
    else if (code == LongCode)
    {
      Add(code, std::int64_t{static_cast<std::int32_t>(In->RL())});
    }
    else
    {
      Note(Fields::Input{.In = In, .With = *With}, IssueCode::Unknown,
           std::format("extended data item code {} unknown", code));
      return false;
    }
    return true;
  }
};
} // namespace

auto ReadEed(Bits::Reader &in, const Fields::Context &with) -> std::vector<Xdata>
{
  std::vector<Xdata> records;
  for (;;)
  {
    const int size = in.BS();
    if (in.HasFailed() || size <= 0)
    {
      return records;
    }
    Xdata record;
    record.AppId = in.H().Value;
    const std::size_t end = in.Position() + (static_cast<std::size_t>(size) * Byte);
    if (end > in.Size())
    {
      in.Seek(in.Size() + 1);
      return records;
    }
    const Item item{.In = &in, .With = &with, .Groups = &record.Tags};
    while (in.Position() < end && item.One())
    {
    }
    if (!in.HasFailed() && in.Position() != end)
    {
      if (in.Position() > end)
      {
        Note(Fields::Input{.In = &in, .With = with}, IssueCode::Damaged,
             std::format("extended data of {} bytes misread", size));
      }
      in.Seek(end);
    }
    records.push_back(std::move(record));
  }
}

namespace
{
constexpr std::size_t MostBytes = 255;

constexpr int LastCode = 255;

// A record's size is a BS read as signed, and one of zero or less ends the extended data.
constexpr std::size_t MostRecord = 0x7FFF;

constexpr int ApplicationCode = 5;

/**
 *  Each item as chapter 28 stores it.
 */
struct Items : GroupWriter
{
  void Note(IssueCode code, std::string what) const { Fields::Note(*To, code, std::move(what)); }

  void String(const std::string &text) const
  {
    Out->RC(StringCode);
    if (To->With.Release >= Version::R2007)
    {
      std::u16string units = Encoding::ToUnits(text);
      if (units.size() > MostBytes)
      {
        Note(IssueCode::OutOfRange, std::format("extended data text of {} units cut to {}", units.size(), MostBytes));
        units.resize(MostBytes);
      }
      Out->RS(static_cast<std::uint16_t>(units.size()));
      for (const char16_t unit : units)
      {
        Out->RS(unit);
      }
      return;
    }
    const Encoding::Scheme scheme = To->With.Text != nullptr ? *To->With.Text : Encoding::Utf8();
    const std::string raw = Truncate(Encoding::Encode(scheme, text), MostBytes, "text");
    const auto page = static_cast<unsigned>(Encoding::NumberOf(scheme));
    Out->RC(static_cast<std::uint8_t>(raw.size()));
    Out->RC(static_cast<std::uint8_t>(page >> Byte));
    Out->RC(static_cast<std::uint8_t>(page & 0xFFU));
    Bytes(raw);
  }

  /**
   *  Returns how many groups the point took.
   */
  [[nodiscard]] auto Point(std::span<const DwgIO::Tag> groups, std::size_t at, double first) const -> std::size_t
  {
    const int code = groups[at].Code;
    const auto [values, taken] = PointAt(groups, at, first);
    Out->RC(static_cast<std::uint8_t>(code - Group1000));
    for (const double value : values)
    {
      Out->RD(value);
    }
    return taken;
  }

  /**
   *  False for a code that is not a handle, a chunk, a real, a short or a long.
   */
  [[nodiscard]] auto Scalar(const DwgIO::Tag &group, std::uint8_t code) const -> bool
  {
    if (code == LayerCode || code == EntityCode)
    {
      if (const auto *handle = As<DwgIO::Handle>(group, "handle"))
      {
        Out->RC(code);
        Out->RLL(std::byteswap(handle->Value));
      }
    }
    else if (code == BinaryCode)
    {
      if (const auto *chunk = As<Chunk>(group, "bytes"))
      {
        const std::string raw = Truncate(TextOf(chunk->Bytes), MostBytes, "chunk");
        Out->RC(BinaryCode);
        Out->RC(static_cast<std::uint8_t>(raw.size()));
        Bytes(raw);
      }
    }
    else if (code >= FirstReal && code <= LastReal)
    {
      if (const auto *real = As<double>(group, "real"))
      {
        Out->RC(code);
        Out->RD(*real);
      }
    }
    else if (code == ShortCode || code == LongCode)
    {
      if (const auto *whole = As<std::int64_t>(group, "number"))
      {
        Out->RC(code);
        Whole(code, *whole);
      }
    }
    else
    {
      return false;
    }
    return true;
  }

  void Whole(std::uint8_t code, std::int64_t value) const
  {
    if (code == ShortCode)
    {
      Out->RS(static_cast<std::uint16_t>(value));
    }
    else
    {
      Out->RL(static_cast<std::uint32_t>(value));
    }
  }

  /**
   *  Returns how many groups it took, two more for a point.
   */
  [[nodiscard]] auto One(std::span<const DwgIO::Tag> groups, std::size_t at) const -> std::size_t
  {
    const DwgIO::Tag &group = groups[at];
    if (group.Code < Group1000 || group.Code > Group1000 + LastCode)
    {
      Note(IssueCode::NotKept, std::format("extended data group {} not stored", group.Code));
      return 1;
    }
    const auto code = static_cast<std::uint8_t>(group.Code - Group1000);
    if (code == StringCode)
    {
      if (const auto *text = As<std::string>(group, "text"))
      {
        String(*text);
      }
    }
    else if (code == BraceCode)
    {
      if (const auto *text = As<std::string>(group, "text"))
      {
        Out->RC(BraceCode);
        Out->RC(*text == "{" ? 0 : 1);
      }
    }
    else if (code >= FirstPoint && code <= LastPoint)
    {
      if (const auto *first = As<double>(group, "real"))
      {
        return Point(groups, at, *first);
      }
    }
    else if (!Scalar(group, code))
    {
      Note(IssueCode::NotKept, std::format("extended data group {} not stored", group.Code));
    }
    return 1;
  }
};
} // namespace

void WriteEed(const std::vector<Xdata> &records, const Fields::Output &to)
{
  for (const Xdata &record : records)
  {
    Bits::Writer items;
    const Items writer{{.Out = &items, .To = &to, .Subject = "extended data"}};
    const std::span<const DwgIO::Tag> groups{record.Tags};
    for (std::size_t at = 0; at < groups.size();)
    {
      at += writer.One(groups, at);
    }
    if (items.Position() == 0)
    {
      // A size of zero ends the extended data, so a record with nothing in it has no way into the file.
      Note(to, IssueCode::NotKept, std::format("extended data of {} with no groups left out", HexOf(record.AppId)));
      continue;
    }
    if (items.Position() / Byte > MostRecord)
    {
      Note(to, IssueCode::NotKept,
           std::format("extended data of {} of {} bytes left out", HexOf(record.AppId), items.Position() / Byte));
      continue;
    }
    to.To->BS(static_cast<int>(items.Position() / Byte));
    to.To->H(Bits::Reference{.Code = ApplicationCode, .Value = record.AppId});
    to.To->Raw(items.Bytes());
  }
  to.To->BS(0);
}
} // namespace DwgIO::Objects
