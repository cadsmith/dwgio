/***********************************************************************************************************************
 *  @file      tests/dwgio/acis/sat.cpp
 *  @brief     The SAT reader and writer: heads of ACIS 1.06 and 4.00, words, numbers and grammars cut short.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "acis/grammar.hpp"
#include "acis/model.hpp"
#include "acis/reader.hpp"
#include "acis/sat/reader.hpp"
#include "acis/sat/writer.hpp"
#include "acis/spec.hpp"
#include "acis/vocabulary.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <string>
#include <utility>

namespace DwgIO::Acis::Sat
{
namespace
{
constexpr const char *Named = "400 7 1 0           \n"
                              "16 Autodesk AutoCAD 21 ASM 231.0.0.65535 OSX 24 Thu Nov 12 08:55:05 2026 \n"
                              "1 9.999999999999999547e-07 1.000000000000000036e-10 \n";

auto Read(Reader &reader, Kind kind) -> Value
{
  Value value;
  CHECK(reader.Read(Spec{.Kind = kind}, value));
  return value;
}
} // namespace

TEST_CASE("acis/sat: the ACIS 1.06 head is one line and the 4.00 head three with values read word by word")
{
  Head head;
  const std::string older = "106 3 1 0          \r\nbody $-1 $1 $-1 $-1 #";
  Reader old{older};
  REQUIRE(old.ReadHead(head));
  CHECK(head.Release == 106);
  CHECK_FALSE(head.Named);
  CHECK(old.Name() == "body");
  CHECK(Read(old, Kind::Pointer).Integer == -1);
  CHECK(Read(old, Kind::Pointer).Integer == 1);
  const std::string text = std::string{Named} +
                           "x\t$-1 -7 1.5 -0 0 1 @7 unknown 12 a b c v open closed reversed 1 0 I F 10 nubs 42 { } "
                           "#\nEnd-of-ACIS-data";
  Reader reader{text};
  REQUIRE(reader.ReadHead(head));
  CHECK(head.Named);
  CHECK(head.Product == "Autodesk AutoCAD");
  CHECK(head.Date == "Thu Nov 12 08:55:05 2026");
  CHECK(head.Tolerances[1] == 1e-6);
  CHECK(reader.Name() == "x");
  CHECK(Read(reader, Kind::Pointer).Integer == -1);
  CHECK(Read(reader, Kind::Long).Integer == -7);
  CHECK(Read(reader, Kind::Real).Reals[0] == 1.5);
  CHECK(Read(reader, Kind::Position).Reals == std::array<double, 3>{-0.0, 0, 1});
  CHECK(Read(reader, Kind::Text).Text == "unknown");
  CHECK(Read(reader, Kind::Text).Text == "a b c v open");
  Value value;
  CHECK(reader.Read(Spec{.Kind = Kind::Choice, .Table = &Vocabulary::Closure}, value));
  CHECK(value.Integer == 1);
  CHECK(reader.Read(Spec{.Kind = Kind::Logical, .Words = &Vocabulary::Sense}, value));
  CHECK(value.Integer == 1);
  // R14 writes some logicals and enumerations as numbers.
  CHECK(reader.Read(Spec{.Kind = Kind::Logical, .Words = &Vocabulary::Sense}, value));
  CHECK(value.Integer == 1);
  CHECK(reader.Read(Spec{.Kind = Kind::Choice, .Table = &Vocabulary::Split}, value));
  CHECK(value.Integer == 0);
  CHECK(Read(reader, Kind::Bound).Integer == 0);
  const Value bound = Read(reader, Kind::Bound);
  CHECK(bound.Integer == 1);
  CHECK(bound.Reals[0] == 10);
  CHECK(Read(reader, Kind::Word).Text == "nubs");
  CHECK(Read(reader, Kind::Huge).Integer == 42);
  CHECK(reader.Brace(true));
  CHECK(reader.Brace(false));
  CHECK(reader.ReadEnd());
  CHECK(reader.End());
  CHECK(reader.Name().empty());
  CHECK(reader.Name().empty());
}

TEST_CASE("acis/sat: a head or word the grammar does not expect fails")
{
  Head head;
  CHECK_FALSE(Reader{"four hundred"}.ReadHead(head));
  CHECK_FALSE(Reader{"400 1 1 0\n16 Autodesk"}.ReadHead(head));
  CHECK_FALSE(Reader{"400 1 1 0\n1 a 1 b 1 c\n1 x 1"}.ReadHead(head));
  const auto fails = [](const std::string &words, const Spec &spec) -> bool
  {
    const std::string text = "106 1 1 0\n" + words;
    Reader reader{text};
    Head opened;
    REQUIRE(reader.ReadHead(opened));
    Value value;
    return !reader.Read(spec, value);
  };
  CHECK(fails("1", Spec{.Kind = Kind::Pointer}));
  CHECK(fails("$x", Spec{.Kind = Kind::Pointer}));
  CHECK(fails("12x", Spec{.Kind = Kind::Long}));
  CHECK(fails("x", Spec{.Kind = Kind::Long}));
  CHECK(fails("", Spec{.Kind = Kind::Long}));
  CHECK(fails("1.5x", Spec{.Kind = Kind::Real}));
  CHECK(fails("x", Spec{.Kind = Kind::Real}));
  CHECK(fails("1", Spec{.Kind = static_cast<Kind>(99)}));
  CHECK(fails("", Spec{.Kind = Kind::Real}));
  CHECK(fails("1 2", Spec{.Kind = Kind::Direction}));
  CHECK(fails("@x abc", Spec{.Kind = Kind::Text}));
  CHECK(fails("-1 abc", Spec{.Kind = Kind::Text}));
  CHECK(fails("9 abc", Spec{.Kind = Kind::Text}));
  CHECK(fails("sideways", Spec{.Kind = Kind::Logical, .Words = &Vocabulary::FaceSense}));
  CHECK(fails("1", Spec{.Kind = Kind::Logical, .Words = &Vocabulary::FaceSense}));
  CHECK(fails("forward", Spec{.Kind = Kind::Logical, .Words = &Vocabulary::FaceSense}) == false);
  CHECK(fails("ajar", Spec{.Kind = Kind::Choice, .Table = &Vocabulary::Closure}));
  CHECK(fails("", Spec{.Kind = Kind::Choice, .Table = &Vocabulary::Closure}));
  CHECK(fails("F", Spec{.Kind = Kind::Bound}));
  CHECK(fails("X", Spec{.Kind = Kind::Bound}));
  CHECK(fails("", Spec{.Kind = Kind::Word}));
  Reader reader{"106 1 1 0\n} {"};
  REQUIRE(reader.ReadHead(head));
  CHECK_FALSE(reader.Brace(true));
  CHECK_FALSE(reader.End());
}

TEST_CASE("acis/sat: R14 writes logicals and enumerations as numbers and R2000 as words")
{
  for (const int release : {Releases::R14, Releases::R2000})
  {
    CAPTURE(release);
    Writer writer;
    writer.WriteHead(Head{.Release = release,
                          .Records = 0,
                          .Bodies = 1,
                          .Flags = 0,
                          .Named = true,
                          .Product = "Autodesk AutoCAD",
                          .Modeler = "ASM 231.0.0.65535 OSX",
                          .Date = "Thu Nov 12 08:55:05 2026",
                          .Tolerances = {1, 1e-6, 1e-10}});
    writer.Name("coedge");
    CHECK(writer.Write(Spec{.Kind = Kind::Pointer}, Value{.Integer = 3}));
    CHECK(writer.Write(Spec{.Kind = Kind::Logical, .Words = &Vocabulary::Sense}, Value{.Integer = 1}));
    CHECK(writer.Write(Spec{.Kind = Kind::Logical, .Words = &Vocabulary::FaceSense}, Value{.Integer = 0}));
    CHECK(writer.Write(Spec{.Kind = Kind::Choice, .Table = &Vocabulary::Merge}, Value{.Integer = 4}));
    CHECK(writer.Write(Spec{.Kind = Kind::Choice, .Table = &Vocabulary::Closure}, Value{.Integer = 2}));
    CHECK(writer.Write(Spec{.Kind = Kind::Text}, Value{.Text = "tangent"}));
    CHECK(writer.Write(Spec{.Kind = Kind::Bound}, Value{.Integer = 1, .Reals = {5, 0, 0}}));
    CHECK(writer.Write(Spec{.Kind = Kind::Bound}, Value{}));
    CHECK(writer.Write(Spec{.Kind = Kind::Word}, Value{.Text = "nubs"}));
    CHECK(writer.Write(Spec{.Kind = Kind::Huge}, Value{.Integer = 7}));
    CHECK(writer.Write(Spec{.Kind = Kind::Direction}, Value{.Reals = {0.1, -0.0, 1}}));
    writer.Brace(true);
    writer.Break();
    writer.Brace(false);
    writer.End();
    CHECK(writer.Write(Spec{.Kind = static_cast<Kind>(99)}, Value{}));
    // An enumeration's value with no word: past the words, or below them.
    CHECK_FALSE(writer.Write(Spec{.Kind = Kind::Choice, .Table = &Vocabulary::Closure}, Value{.Integer = 3}));
    CHECK_FALSE(writer.Write(Spec{.Kind = Kind::Choice, .Table = &Vocabulary::Closure}, Value{.Integer = -1}));
    CHECK(writer.Write(Spec{.Kind = Kind::Choice, .Table = &Vocabulary::Split}, Value{.Integer = 0}) ==
          (release == Releases::R14));
    const std::string text = writer.Finish();
    if (release == Releases::R14)
    {
      CHECK(text == "106 1 1 0           \ncoedge $3 1 forward 4 periodic 7 tangent F 5 I nubs 7 "
                    "0.1000000000000000056 -0 1 { \n\t} #\n0 ");
    }
    else
    {
      CHECK(text == std::string{Named}.replace(4, 1, "1") +
                        "coedge $3 reversed forward keep_all periodic 7 tangent F 5 I nubs 7 "
                        "0.1000000000000000056 -0 1 { \n\t} #\n");
    }
  }
}

TEST_CASE("acis/sat: a B-spline's knot and control point counts stop at the end of the body")
{
  // The values a walk of the record's grammar gives, and why it stopped.
  const auto walked = [](const std::string &record) -> std::pair<std::size_t, std::string>
  {
    const std::string text = std::string{Named} + record;
    Reader source{text};
    Head head;
    REQUIRE(source.ReadHead(head));
    const std::string name = source.Name();
    Record read{.Name = name, .Values = {}};
    FieldReader reading{source, head.Release, read};
    REQUIRE(Walk(reading, name));
    return {read.Values.size(), reading.Failure()};
  };
  const auto knots = walked("intcurve-curve $-1 forward { exactcur nubs 1 open 16777216 0 1");
  CHECK(knots.first < 64); // The rest of its grammar, as defaults.
  CHECK(knots.second == "value of intcurve-curve not read");
  const auto points = walked("intcurve-curve $-1 forward { exactcur nubs 1 open 1 0 16777216 0 0");
  CHECK(points.first < 64); // The rest of its grammar, as defaults.
  CHECK(points.second == "value of intcurve-curve not read");
  const auto surface = walked("spline-surface $-1 forward { exactsur nubs 1 1 open open none none 1 1 0 4096 0 4096");
  CHECK(surface.first < 64); // The rest of its grammar, as defaults.
  CHECK(surface.second == "value of spline-surface not read");
}
} // namespace DwgIO::Acis::Sat
