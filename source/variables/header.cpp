/***********************************************************************************************************************
 *  @file      source/variables/header.cpp
 *  @brief     The reader and writer of the header variables section.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/byte.hpp"
#include "bits/reader.hpp"
#include "bits/strings.hpp"
#include "bits/writer.hpp"
#include "variables/layout.hpp"

#include "dwgio/version.hpp"

#include <algorithm>
#include <cstdint>
#include <expected>
#include <format>
#include <optional>
#include <string>

namespace DwgIO::Variables
{
namespace
{
using Bits::Byte;

/**
 *  The bits of the RL that opens the variables from R2007.
 */
constexpr std::size_t LeadBits = 32;

/**
 *  The bits from the reader's position to the end, in bytes, the last filled with zeros.
 */
auto Rest(Bits::Reader &reader) -> std::vector<std::byte>
{
  std::vector<std::byte> rest;
  for (std::size_t left = reader.Size() - reader.Position(); left > 0;)
  {
    const auto count = static_cast<int>(std::min<std::size_t>(left, Byte));
    rest.push_back(static_cast<std::byte>(reader.Bits(count) << (Byte - static_cast<std::size_t>(count))));
    left -= static_cast<std::size_t>(count);
  }
  return rest;
}

/**
 *  Writes the kept bits, or fills to the byte when they are fewer than the bits the byte lacks.
 */
void Pad(Bits::Writer &writer, std::span<const std::byte> padding)
{
  const std::size_t over = writer.Position() % Byte;
  const std::size_t kept = padding.size() * Byte;
  if (kept < over)
  {
    writer.Fill();
    return;
  }
  for (std::size_t bit = 0; bit < kept - over; ++bit)
  {
    const auto mask = static_cast<std::byte>(0x80U >> (bit % Byte));
    writer.B((padding[bit / Byte] & mask) != std::byte{0});
  }
}

/**
 *  From R2007: the bit the handles start at, then the variables with their text in the string stream and their handles,
 *  but the seed, in the handle stream; the padding is what follows the handles.
 */
auto ReadStreamed(std::span<const std::byte> body, const Fields::Context &with) -> DwgIO::Header
{
  DwgIO::Header header;
  Bits::Reader reader{body};
  Fields::Input in{.In = &reader, .With = with};
  const std::size_t handles = reader.RL();
  const std::expected<std::optional<Bits::Strung>, std::string> found = Bits::FindStrings(body, handles);
  if (!found)
  {
    Note(in, IssueCode::Damaged, std::format("header {}", found.error()));
  }
  std::optional<Bits::Reader> strings = Bits::MakeStringReader(body, found.value_or(std::nullopt));
  Bits::Reader stream{body};
  stream.Seek(handles);
  in.Handles = &stream;
  in.Strings = strings ? &*strings : nullptr;
  if (!Fields::Read(Layout(), header, in))
  {
    Note(in, IssueCode::Truncated, "header variables cut short");
    return header;
  }
  header.Padding = Rest(stream);
  return header;
}
} // namespace

auto Read(std::span<const std::byte> body, const Fields::Context &with) -> DwgIO::Header
{
  if (with.Release >= Version::R2007)
  {
    return ReadStreamed(body, with);
  }
  DwgIO::Header header;
  Bits::Reader reader{body};
  Fields::Input in{.In = &reader, .With = with};
  if (!Fields::Read(Layout(), header, in))
  {
    Note(in, IssueCode::Truncated, "header variables cut short");
    return header;
  }
  header.Padding = Rest(reader);
  return header;
}

auto Write(const DwgIO::Header &header, const Fields::Context &with) -> std::vector<std::byte>
{
  Bits::Writer writer;
  if (with.Release >= Version::R2007)
  {
    Bits::Writer data;
    Bits::Writer handles;
    Bits::Writer strings;
    Fields::Output to{.To = &data, .Handles = &handles, .Strings = &strings, .With = with};
    static_cast<void>(Fields::Write(Layout(), header, to));
    data.Append(Bits::WriteStringTail(strings));
    writer.RL(static_cast<std::uint32_t>(LeadBits + data.Position()));
    writer.Append(data);
    writer.Append(handles);
    Pad(writer, header.Padding);
    return writer.Take();
  }
  Fields::Output to{.To = &writer, .With = with};
  static_cast<void>(Fields::Write(Layout(), header, to));
  Pad(writer, header.Padding);
  return writer.Take();
}
} // namespace DwgIO::Variables
