/***********************************************************************************************************************
 *  @file      tests/dwgio/acis/convert.cpp
 *  @brief     ACIS bodies converted between versions: edge parameters, unknown records and B-spline counts.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "acis/convert.hpp"

#include "acis/model.hpp"
#include "acis/read.hpp"
#include "acis/write.hpp"
#include "octets.hpp"

#include "dwgio/entities/geometry.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <vector>

namespace DwgIO::Acis
{
namespace
{
/**
 *  A SAT body of ACIS 4.00 of those records.
 */
auto Sat(const std::vector<std::string> &records) -> Entities::Acis
{
  std::string text = std::format("400 {} 1 0\n16 Autodesk AutoCAD 21 ASM 231.0.0.65535 OSX 24 Thu Nov 12 08:55:05 "
                                 "2026 \n1 9.999999999999999547e-07 1.000000000000000036e-10 \n",
                                 records.size());
  for (const std::string &record : records)
  {
    text += record + "\n";
  }
  return Entities::Acis{.Version = 1, .Textual = true, .Data = BytesOf(text)};
}

/**
 *  An edge from one point to another along a curve, with its two vertices and points, as SAT of ACIS 4.00.
 */
auto MakeEdge(std::string_view curve, std::string_view start, std::string_view end) -> std::vector<std::string>
{
  return {"edge $-1 $1 $2 $-1 $3 forward #",
          "vertex $-1 $0 $4 #",
          "vertex $-1 $0 $5 #",
          std::string{curve},
          std::format("point $-1 {} #", start),
          std::format("point $-1 {} #", end)};
}

/**
 *  An intcurve along a B-spline of degree 1 with those knots and control points, of that sense.
 */
auto Spline(std::string_view sense, std::string_view knots, std::string_view points = "0 0 0 1 0 0") -> std::string
{
  return std::format("intcurve-curve $-1 {} {{ exactcur nubs 1 open {} {} 0 null_surface null_surface nullbs "
                     "nullbs I I 0 0 0 F 1 F 0 }} I I #",
                     sense, knots, points);
}

/**
 *  The parameters ACIS 20800 gives the edge, or why not.
 */
auto Parameters(const std::vector<std::string> &records) -> std::expected<std::string, std::string>
{
  const std::expected<Entities::Acis, std::string> upgraded = Convert(Sat(records), Version::R2004, false);
  if (!upgraded)
  {
    return std::unexpected(upgraded.error());
  }
  const std::expected<Model, std::string> model = Read(*upgraded);
  REQUIRE(model);
  const Record &edge = model->Records.at(1); // After the header record.
  REQUIRE(edge.Name == "edge");
  return std::format("{} {}", edge.Values.at(4).Reals[0], edge.Values.at(6).Reals[0]);
}
} // namespace

TEST_CASE("acis/convert: upgrading from ACIS 4.00 gives an edge the parameters of its vertices along its curve")
{
  CHECK(Parameters(MakeEdge("straight-curve $-1 0 0 0 2 0 0 I I #", "0 0 0", "1 0 0")) == "0 0.5");
  CHECK(Parameters(MakeEdge("ellipse-curve $-1 0 0 0 0 0 1 1 0 0 1 I I #", "0 1 0", "1 0 0")) ==
        std::format("{} {}", 1.5707963267948966, 6.283185307179586));
  CHECK(Parameters(MakeEdge("ellipse-curve $-1 0 0 0 0 0 1 1 0 0 1 I I #", "-1 0 0", "-1 0 0")) ==
        std::format("{} {}", -3.141592653589793, 3.141592653589793));
  CHECK(Parameters(MakeEdge(Spline("forward", "2 0 1 1 1"), "0 0 0", "1 0 0")) == "0 1");
  CHECK(Parameters(MakeEdge(Spline("forward", "2 0 1 1 1"), "0 0 0", "0 0 0")) == "0 1");
  std::vector<std::string> apex = MakeEdge("straight-curve $-1 0 0 0 1 0 0 I I #", "0 0 0", "1 0 0");
  apex[0] = "edge $-1 $1 $1 $-1 $-1 forward #";
  CHECK(Parameters(apex) == "1 0");
}

TEST_CASE("acis/convert: an edge whose parameters cannot be derived stays below ACIS 20800")
{
  const std::string failed = "parameters of edge 0 not found";
  const std::string line = "straight-curve $-1 0 0 0 1 0 0 I I #";
  CHECK(Parameters(MakeEdge(line, "0 0 0", "0 0 0")).error() == failed);
  CHECK(Parameters(MakeEdge("straight-curve $-1 0 0 0 0 0 0 I I #", "0 0 0", "1 0 0")).error() == failed);
  CHECK(Parameters(MakeEdge("plane-surface $-1 0 0 0 0 0 1 1 0 0 forward_v I I I I #", "0 0 0", "1 0 0")).error() ==
        failed);
  CHECK(Parameters(MakeEdge(Spline("reversed", "2 0 1 1 1"), "0 0 0", "1 0 0")).error() == failed);
  CHECK(Parameters(MakeEdge(Spline("forward", "2 0 1 1 1"), "1 0 0", "0 0 0")).error() == failed);
  CHECK(Parameters(MakeEdge(Spline("forward", "2 0 1 1 1"), "0 0 0", "2 0 0")).error() == failed);
  CHECK(Parameters(MakeEdge(Spline("forward", "0", ""), "0 0 0", "1 0 0")).error() == failed);
  CHECK(Parameters(MakeEdge(Spline("forward", "2 0 0 1 0", ""), "0 0 0", "1 0 0")).error() == failed);
  CHECK(Parameters(MakeEdge("intcurve-curve $-1 forward { ref 0 } I I #", "0 0 0", "1 0 0")).error() == failed);
  std::vector<std::string> reversed = MakeEdge(line, "0 0 0", "1 0 0");
  reversed[0] = "edge $-1 $1 $2 $-1 $3 reversed #";
  CHECK(Parameters(reversed).error() == failed);
  for (const std::string_view vertex : {"$-1", "$9", "$4"})
  {
    std::vector<std::string> lost = MakeEdge(line, "0 0 0", "1 0 0");
    lost[0] = std::format("edge $-1 {} $2 $-1 $3 forward #", vertex);
    CHECK(Parameters(lost).error() == failed);
  }
  std::vector<std::string> endless = MakeEdge(line, "0 0 0", "1 0 0");
  endless[0] = "edge $-1 $1 $9 $-1 $3 forward #";
  CHECK(Parameters(endless).error() == failed);
  std::vector<std::string> pointless = MakeEdge(line, "0 0 0", "1 0 0");
  pointless[1] = "vertex $-1 $0 $3 #";
  CHECK(Parameters(pointless).error() == failed);
}

TEST_CASE("acis/convert: an unseen or broken record or subtype is not converted")
{
  const auto why = [](const std::vector<std::string> &records) -> std::string
  {
    return Convert(Sat(records), Version::R2004, false).error();
  };
  CHECK(why({"blend-surface $-1 #"}) == "record blend-surface not known");
  CHECK(why({"point $-1 0 0 0 1 #"}) == "record point holds more than known");
  CHECK(why({"point $-1 0 0 #"}) == "value of point not read");
  CHECK(why({"pcurve $-1 1 forward $3 0 0 #"}) == "parameter curve of another curve not known");
  CHECK(why({"intcurve-curve $-1 forward exactcur #"}) == "subtype of intcurve-curve not opened");
  CHECK(why({"intcurve-curve $-1 forward { ref 0 I I #"}) == "subtype of intcurve-curve not closed");
  CHECK(why({"intcurve-curve $-1 forward { offsetcur } I I #"}) == "subtype offsetcur not known");
  CHECK(why({"intcurve-curve $-1 forward { exactcur bezier } I I #"}) == "B-spline of form bezier not known");
  CHECK(why({"intcurve-curve $-1 forward { exactcur nubs 1 open -1 } I I #"}) == "count of -1 not understood");
  CHECK(why({"intcurve-curve $-1 forward { exactcur nubs 1 open 3 0 1 1 99999999 2 1 } I I #"}) ==
        "count of 99999999 not understood");
  CHECK(why({"intcurve-curve $-1 forward {"}) == "word of intcurve-curve not read");
  CHECK(why({"intcurve-curve $-1 sideways { ref 0 } I I #"}) == "value of intcurve-curve not read");
  CHECK(why({"persubent-acadSolidHistory-attrib $-1 $-1 $-1 $4 x #"}) ==
        "value of persubent-acadSolidHistory-attrib not read");
  CHECK(why({"intcurve-curve $-1 forward { exactcur nubs 1 open 2 0 1 1 1 0 0 0 1 0 0 0 surface null_surface "
             "nullbs nullbs I I 0 0 0 F 1 F 0 } I I #"}) == "surface in place of null_surface not known");
  CHECK(why({"pcurve $-1 0 forward { exppc nubs 1 open 2 0 1 1 1 0 0 1 0 0 plane forward { ref 0 } I I I I } 0 0 #"}) ==
        "parameter curve on a plane surface not known");
  CHECK(why({"spline-surface $-1 forward { exactsur nubs 1 1 open open none none 1 1 0 0 0 1 } I I I I #"}) ==
        "count of -1 not understood");
  CHECK(why({"spline-surface $-1 forward { exactsur nubs 1 1 open open none none 2 1 0 1 1 1 0 0 } I I I I #"}) ==
        "count of -1 not understood");
  CHECK(why({"spline-surface $-1 forward { exactsur cone } I I I I #"}) == "B-spline of form cone not known");
  // A body of no records, a pcurve's subtype by reference, and a spline surface's: read and written.
  CHECK(Convert(Sat({}), Version::R2004, false));
  CHECK(Convert(Sat({"pcurve $-1 0 forward { ref 0 } 0 0 #"}), Version::R2018, false));
  CHECK(Convert(Sat({"spline-surface $-1 forward { ref 0 } I I I I #"}), Version::R2013, false));
  CHECK(
      Convert(Entities::Acis{.Version = 2, .Textual = false, .Data = BytesOf("ACIS")}, Version::R2000, false).error() ==
      "head not read");
  // A release before every field's, whose records would read none: a history's longs would never end.
  CHECK(Convert(Entities::Acis{.Version = 1,
                               .Textual = true,
                               .Data = BytesOf("-1 1 1 0 persubent-acadSolidHistory-attrib 7 #")},
                Version::R2004, false)
            .error() == "head not read");
}

TEST_CASE("acis/convert: a B-spline's multiplicities and degree must be counts")
{
  // A multiplicity, a sum of them, or a degree that is no count fails the walk: none sums past what a count holds,
  // and the control points never number fewer than the knots say, which the parameters of an upgraded edge rely on.
  const auto why = [](const std::vector<std::string> &records) -> std::string
  {
    return Convert(Sat(records), Version::R2004, false).error();
  };
  CHECK(why({"intcurve-curve $-1 forward { exactcur nubs 1 open 2 0 -1 1 1 } I I #"}) == "count of -1 not understood");
  CHECK(why({"intcurve-curve $-1 forward { exactcur nubs 1 open 2 0 16777216 1 1 } I I #"}) ==
        "count of 16777217 not understood");
  CHECK(why({"intcurve-curve $-1 forward { exactcur nubs -9223372036854775807 open 1 0 5 } I I #"}) ==
        "count of -9223372036854775807 not understood");
  CHECK(why({"spline-surface $-1 forward { exactsur nubs -5 1 open open none none 1 1 0 1 0 1 } I I I I #"}) ==
        "count of -5 not understood");
  const std::vector<std::string> edged =
      MakeEdge("intcurve-curve $-1 forward { exactcur nubs 16777216 open 100 0 16777217 0 0 0 1 1 1 1e-6 null_surface "
               "null_surface nullbs nullbs I I 0 0 0 I I } I I #",
               "0 0 0", "1 1 1");
  CHECK(why(edged) == "count of 16777217 not understood");
}

TEST_CASE("acis/convert: a body already in the target ACIS version comes back unchanged")
{
  const Entities::Acis none{};
  CHECK(Convert(none, Version::R2013, false) == none);
  const Entities::Acis sat = Sat({"point $-1 0 0 0 #"});
  CHECK(Convert(sat, Version::R2000, false) == sat);
  const Entities::Acis newer = Convert(sat, Version::R2013, false).value();
  CHECK(newer.Version == 2);
  CHECK_FALSE(newer.Textual);
  CHECK(Convert(newer, Version::R2018, false) == newer);
  CHECK(Convert(newer, Version::R2013, false) == newer);
  const Entities::Acis back = Convert(newer, Version::R2000, false).value();
  CHECK(back.Version == 1);
  CHECK(back.Textual);
  Entities::Acis other = sat;
  std::ranges::copy(BytesOf("700"), other.Data.begin());
  CHECK(Convert(other, Version::R2004, false).error() == "ACIS 700 not known");
  CHECK(ReleaseOf(Version::R14) == Releases::R14);
  CHECK(ReleaseOf(Version::R2018) == Releases::R2018);
}

TEST_CASE("acis/convert: a value with no SAT word or values that break the grammar are not written")
{
  Model attributed;
  attributed.Head.Release = Releases::R2004;
  attributed.Head.Named = true;
  attributed.Records.push_back(
      Record{.Name = "int64_attrib-name_attrib-gen-attrib",
             .Values = {Value{.Kind = Kind::Pointer, .Integer = -1}, Value{.Kind = Kind::Long, .Integer = -1},
                        Value{.Kind = Kind::Pointer, .Integer = -1}, Value{.Kind = Kind::Pointer, .Integer = -1},
                        Value{.Kind = Kind::Pointer, .Integer = -1}, Value{.Kind = Kind::Choice, .Integer = 0},
                        Value{.Kind = Kind::Choice, .Integer = 4}, Value{.Kind = Kind::Choice, .Integer = 1},
                        Value{.Kind = Kind::Choice, .Integer = 2}, Value{.Kind = Kind::Text, .Text = "x"},
                        Value{.Kind = Kind::Huge, .Integer = 0}}});
  CHECK(Write(attributed, Releases::R2000, false).error() ==
        "0 of int64_attrib-name_attrib-gen-attrib has no word in SAT");
  CHECK(Write(attributed, Releases::R14, false));
  Model broken;
  broken.Head.Release = Releases::R2004;
  broken.Records.push_back(Record{.Name = "point", .Values = {Value{.Kind = Kind::Pointer, .Integer = -1}}});
  CHECK(Write(broken, Releases::R2010, false).error() == "point holds other values");
  broken.Records.front().Values.push_back(Value{.Kind = Kind::Real});
  CHECK(Write(broken, Releases::R2010, false).error() == "point holds other values");
  // A record whose walk goes on past its failure, to its end or to a failure of its own.
  Model history;
  history.Head.Release = Releases::R2004;
  history.Records.push_back(Record{.Name = "persubent-acadSolidHistory-attrib",
                                   .Values = {Value{.Kind = Kind::Pointer}, Value{.Kind = Kind::Real}}});
  history.Records.push_back(Record{.Name = "intcurve-curve", .Values = {Value{.Kind = Kind::Pointer}}});
  CHECK(Write(history, Releases::R2010, false).error() == "persubent-acadSolidHistory-attrib holds other values");
  history.Records.erase(history.Records.begin());
  CHECK(Write(history, Releases::R2010, false).error() == "intcurve-curve holds other values");
}

TEST_CASE("acis/convert: upgrading a body AutoCAD remakes gives its vertices ends and its seams tangency")
{
  // Two faces' loops and coedges, the second edge a seam on the first face, the third along no coedge.
  const std::vector<std::string> records{"edge $-1 $3 $4 $5 $-1 forward #",
                                         "edge $-1 $4 $3 $6 $-1 forward #",
                                         "edge $-1 $4 $4 $-1 $-1 forward #",
                                         "vertex $-1 $0 $9 #",
                                         "vertex $-1 $0 $10 #",
                                         "coedge $-1 $5 $5 $-1 $0 forward $7 $-1 #",
                                         "coedge $-1 $6 $6 $11 $1 forward $7 $-1 #",
                                         "loop $-1 $-1 $5 $12 #",
                                         "loop $-1 $-1 $11 $13 #",
                                         "point $-1 0 0 0 #",
                                         "point $-1 1 0 0 #",
                                         "coedge $-1 $11 $11 $6 $1 reversed $7 $-1 #",
                                         "vertex $-1 $-1 $9 #",
                                         "vertex $-1 $5 $9 #",
                                         "vertex $-1 $99 $9 #",
                                         "edge $-1 $3 $3 $16 $-1 forward #",
                                         "coedge $-1 $16 $16 $-1 $15 forward $99 $-1 #"};
  const Entities::Acis upgraded = Convert(Sat(records), Version::R2007, true).value();
  const Model model = Read(upgraded).value();
  // Past the header record: the edges' convexities, then the vertices' longs.
  CHECK(model.Records.at(1).Values.at(10).Text == "unknown");
  CHECK(model.Records.at(2).Values.at(10).Text == "tangent");
  CHECK(model.Records.at(3).Values.at(10).Text == "unknown");
  CHECK(model.Records.at(4).Values.at(4).Integer == 0);
  CHECK(model.Records.at(5).Values.at(4).Integer == 1);
  CHECK(model.Records.at(13).Values.at(4).Integer == 1);
  CHECK(model.Records.at(14).Values.at(4).Integer == 1);
  CHECK(model.Records.at(15).Values.at(4).Integer == 1);
  CHECK(model.Records.at(16).Values.at(10).Text == "unknown");
  const Model kept = Read(Convert(Sat(records), Version::R2007, false).value()).value();
  CHECK(kept.Records.at(2).Values.at(10).Text == "unknown");
  CHECK(kept.Records.at(4).Values.at(4).Integer == 2);
}
} // namespace DwgIO::Acis
