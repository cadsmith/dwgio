/***********************************************************************************************************************
 *  @file      tests/dwgio/acis/sab.cpp
 *  @brief     The SAB reader and writer: every kind of value behind each release's head, and hostile bytes.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "acis/model.hpp"
#include "acis/sab/reader.hpp"
#include "acis/sab/writer.hpp"
#include "acis/spec.hpp"
#include "acis/vocabulary.hpp"

#include <doctest/doctest.h>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace DwgIO::Acis::Sab
{
namespace
{
/**
 *  A head as AutoCAD's SAB of a release gives it.
 */
auto MakeSabHead(int release) -> Head
{
  return Head{.Release = release,
              .Records = 0,
              .Bodies = 2,
              .Flags = 12,
              .Named = true,
              .Product = "Autodesk AutoCAD",
              .Modeler = "ASM 231.0.0.65535 OSX",
              .Date = "Thu Nov 12 08:55:05 2026",
              .Tolerances = {1, 1e-6, 1e-10}};
}

/**
 *  One field of each kind, and a value for it.
 */
struct Field
{
  Spec Asked;
  Value Given;
};

auto Fields() -> std::vector<Field>
{
  return {
      {.Asked = {.Kind = Kind::Pointer}, .Given = {.Kind = Kind::Pointer, .Integer = -1}},
      {.Asked = {.Kind = Kind::Long}, .Given = {.Kind = Kind::Long, .Integer = 23100}},
      {.Asked = {.Kind = Kind::Real}, .Given = {.Kind = Kind::Real, .Reals = {-0.5, 0, 0}}},
      {.Asked = {.Kind = Kind::Position}, .Given = {.Kind = Kind::Position, .Reals = {1, 2, 3}}},
      {.Asked = {.Kind = Kind::Direction}, .Given = {.Kind = Kind::Direction, .Reals = {0, 0, 1}}},
      {.Asked = {.Kind = Kind::Text}, .Given = {.Kind = Kind::Text, .Text = "unknown"}},
      {.Asked = {.Kind = Kind::Text}, .Given = {.Kind = Kind::Text, .Text = std::string(300, 'x')}},
      {.Asked = {.Kind = Kind::Logical, .Words = &Vocabulary::Sense}, .Given = {.Kind = Kind::Logical, .Integer = 1}},
      {.Asked = {.Kind = Kind::Logical, .Words = &Vocabulary::Sense}, .Given = {.Kind = Kind::Logical, .Integer = 0}},
      {.Asked = {.Kind = Kind::Choice, .Table = &Vocabulary::Closure}, .Given = {.Kind = Kind::Choice, .Integer = 2}},
      {.Asked = {.Kind = Kind::Bound}, .Given = {.Kind = Kind::Bound, .Integer = 0}},
      {.Asked = {.Kind = Kind::Bound}, .Given = {.Kind = Kind::Bound, .Integer = 1, .Reals = {10, 0, 0}}},
      {.Asked = {.Kind = Kind::Word}, .Given = {.Kind = Kind::Word, .Text = "nubs"}},
      {.Asked = {.Kind = Kind::Huge}, .Given = {.Kind = Kind::Huge, .Integer = -(std::int64_t{1} << 40)}},
  };
}

/**
 *  A body of one record holding every field, written as that release.
 */
auto Written(int release) -> std::string
{
  Writer writer;
  writer.WriteHead(MakeSabHead(release));
  writer.Name("int64_attrib-name_attrib-gen-attrib");
  for (const Field &field : Fields())
  {
    CHECK(writer.Write(field.Asked, field.Given));
  }
  writer.Brace(true);
  writer.Break();
  writer.Brace(false);
  writer.End();
  return writer.Finish();
}

auto Little(std::uint64_t value, std::size_t size) -> std::string
{
  std::string bytes;
  for (std::size_t at = 0; at < size; ++at)
  {
    bytes.push_back(static_cast<char>((value >> (8 * at)) & 0xFF));
  }
  return bytes;
}

/**
 *  A head by hand, then what follows it.
 */
auto PrependHead(const std::string &tail) -> std::string
{
  std::string bytes{"ACIS BinaryFile"};
  for (const std::uint64_t count : {21500U, 0U, 2U, 24U})
  {
    bytes += Little(count, 4);
  }
  for (const char *text : {"P", "M", "D"})
  {
    bytes += std::string{"\x07\x01"} + text;
  }
  for (int tolerance = 0; tolerance < 3; ++tolerance)
  {
    bytes += "\x06" + Little(std::bit_cast<std::uint64_t>(1.0), 8);
  }
  return bytes + tail;
}
} // namespace

TEST_CASE("acis/sab: every kind of value written reads back after the release's head and name")
{
  for (const int release : {Releases::R2010, Releases::R2018})
  {
    CAPTURE(release);
    const std::string bytes = Written(release);
    CHECK(bytes.starts_with(release == Releases::R2018 ? "ASM BinaryFile4" : "ACIS BinaryFile"));
    CHECK(bytes.ends_with(release == Releases::R2018 ? std::string{"ASM\x0D\x04"
                                                                   "data"}
                                                     : std::string{"ACIS\x0D\x04"
                                                                   "data"}));
    Reader reader{bytes};
    Head head;
    REQUIRE(reader.ReadHead(head));
    CHECK(head.Release == release);
    CHECK(head.Bodies == 2);
    CHECK(head.Flags == 12);
    CHECK(head.Date == "Thu Nov 12 08:55:05 2026");
    CHECK(head.Tolerances[2] == 1e-10);
    CHECK(reader.Name() == "int64_attrib-name_attrib-gen-attrib");
    for (const Field &field : Fields())
    {
      Value value;
      REQUIRE(reader.Read(field.Asked, value));
      CHECK(value == field.Given);
    }
    CHECK(reader.Brace(true));
    CHECK_FALSE(reader.Brace(true));
    CHECK(reader.Brace(false));
    CHECK(reader.ReadEnd());
    CHECK(reader.End());
    CHECK_FALSE(reader.ReadEnd());
    CHECK(reader.Name().empty()); // The record that ends the body.
    CHECK_FALSE(reader.ReadEnd());
  }
}

TEST_CASE("acis/sab: a head or value with another tag or cut short fails the reader")
{
  Head head;
  CHECK_FALSE(Reader{"not SAB at all, not even close"}.ReadHead(head));
  const std::string cutHead = Written(Releases::R2013).substr(0, 40);
  CHECK_FALSE(Reader{cutHead}.ReadHead(head));
  // Each kind read where the end of a record lies.
  for (const Field &field : Fields())
  {
    CAPTURE(static_cast<int>(field.Asked.Kind));
    const std::string bytes = PrependHead("\x11");
    Reader reader{bytes};
    REQUIRE(reader.ReadHead(head));
    Value value;
    CHECK_FALSE(reader.Read(field.Asked, value));
    // A reader that has failed stays failed, whatever comes.
    CHECK_FALSE(reader.Read(Spec{.Kind = Kind::Logical, .Words = &Vocabulary::Sense}, value));
    CHECK(reader.ReadEnd()); // What it did not take is still there.
  }
  // A kind no grammar asks for reads nothing, and writes nothing.
  {
    const std::string bytes = PrependHead("\x11");
    Reader reader{bytes};
    REQUIRE(reader.ReadHead(head));
    Value value;
    CHECK(reader.Read(Spec{.Kind = static_cast<Kind>(99)}, value));
    Writer writer;
    writer.WriteHead(MakeSabHead(Releases::R2010));
    CHECK(writer.Write(Spec{.Kind = static_cast<Kind>(99)}, Value{}));
  }
  // A finite bound with no double; a long, a double and text cut short.
  for (const std::string &tail : {std::string{"\x0A\x04"}, std::string{"\x04\x01\x02"}, std::string{"\x06\x01"},
                                  std::string{"\x07\x05"
                                              "ab"},
                                  std::string{"\x08\x05"}})
  {
    const std::string bytes = PrependHead(tail);
    Reader reader{bytes};
    REQUIRE(reader.ReadHead(head));
    Value value;
    const Kind kind = tail[0] == '\x0A'   ? Kind::Bound
                      : tail[0] == '\x04' ? Kind::Long
                      : tail[0] == '\x06' ? Kind::Real
                                          : Kind::Text;
    CHECK_FALSE(reader.Read(Spec{.Kind = kind}, value));
  }
  // A name with parts and no last word, one cut short, and none at the end of the bytes.
  const std::string parts = PrependHead("\x0E\x03"
                                        "End\x04");
  Reader parted{parts};
  REQUIRE(parted.ReadHead(head));
  CHECK(parted.Name().empty());
  const std::string cutName = PrependHead("\x0D\x09"
                                          "body");
  Reader cut{cutName};
  REQUIRE(cut.ReadHead(head));
  CHECK(cut.Name().empty());
  const std::string empty = PrependHead("");
  Reader none{empty};
  REQUIRE(none.ReadHead(head));
  CHECK(none.Name().empty());
  CHECK_FALSE(none.End());
}
} // namespace DwgIO::Acis::Sab
