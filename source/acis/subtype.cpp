/***********************************************************************************************************************
 *  @file      source/acis/subtype.cpp
 *  @brief     The spline subtypes of intcurves, parameter curves and spline surfaces.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "acis/subtype.hpp"

#include "acis/fields.hpp"
#include "acis/model.hpp"
#include "acis/vocabulary.hpp"

#include <array>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>

namespace DwgIO::Acis::Subtype
{
namespace
{
/**
 *  More knots or control points than any body holds, which only a broken one gives.
 */
constexpr std::int64_t Most = 0x1000000;

/**
 *  The version ACIS 21200 and later give a spline subtype read from a release without it.
 */
constexpr std::int64_t SplineVersion = 20900;

/**
 *  Whether a count is one a body can hold, failing the walk when not.
 */
auto CheckCount(const Fields &fields, std::int64_t count) -> bool
{
  if (count < 0 || count > Most)
  {
    fields.Fail(std::format("count of {} not understood", count));
    return false;
  }
  return true;
}

/**
 *  The knots: each a double and its multiplicity; gives the multiplicities' sum.
 *
 *  Each multiplicity and the sum so far must be counts a body holds, and a failed walk stops, so that a broken body
 *  walks no further than it reads.
 */
auto Knots(const Fields &fields, std::int64_t count) -> std::int64_t
{
  std::int64_t sum = 0;
  for (std::int64_t knot = 0; CheckCount(fields, count) && knot < count && !fields.HasFailed(); ++knot)
  {
    static_cast<void>(fields.Real());
    const std::int64_t multiplicity = fields.Long();
    if (!CheckCount(fields, multiplicity) || !CheckCount(fields, sum + multiplicity))
    {
      break;
    }
    sum += multiplicity;
  }
  return sum;
}

/**
 *  A degree, which must be a count a body holds for the control points' count to be found from it; 0 otherwise.
 */
auto Degree(const Fields &fields) -> std::int64_t
{
  const std::int64_t degree = fields.Long();
  return CheckCount(fields, degree) ? degree : 0;
}

/**
 *  The control points, each of that many doubles on a line of its own; a failed walk stops.
 */
void Points(const Fields &fields, std::int64_t count, int dimensions)
{
  for (std::int64_t point = 0; CheckCount(fields, count) && point < count && !fields.HasFailed(); ++point)
  {
    fields.Break();
    for (int coordinate = 0; coordinate < dimensions; ++coordinate)
    {
      static_cast<void>(fields.Real());
    }
  }
}

/**
 *  The form of a B-spline, `nubs` or `nurbs` with a weight to each control point; false and a failed walk otherwise.
 */
auto Form(const Fields &fields, bool &rational) -> bool
{
  const std::string form = fields.Word();
  rational = form == "nurbs";
  if (!rational && form != "nubs")
  {
    fields.Fail(std::format("B-spline of form {} not known", form));
    return false;
  }
  return true;
}

/**
 *  A B-spline curve in that many dimensions: its form, degree, closure and knots, then its control points.
 */
void Spline(const Fields &fields, int dimensions)
{
  bool rational = false;
  if (!Form(fields, rational))
  {
    return;
  }
  const std::int64_t degree = Degree(fields);
  static_cast<void>(fields.Choice(Vocabulary::Closure));
  const std::int64_t knots = fields.Long();
  fields.Break();
  const std::int64_t sum = Knots(fields, knots);
  Points(fields, sum - degree + 1, dimensions + (rational ? 1 : 0));
}

/**
 *  A B-spline surface: its form, its degrees, whether rational along a direction, its closures and singularities, its
 *  knots along each direction, then its control points.
 */
void Sheet(const Fields &fields)
{
  bool rational = false;
  if (!Form(fields, rational))
  {
    return;
  }
  const std::int64_t across = Degree(fields);
  const std::int64_t along = Degree(fields);
  if (rational)
  {
    static_cast<void>(fields.Word());
  }
  static_cast<void>(fields.Choice(Vocabulary::Closure));
  static_cast<void>(fields.Choice(Vocabulary::Closure));
  static_cast<void>(fields.Choice(Vocabulary::Singularity));
  static_cast<void>(fields.Choice(Vocabulary::Singularity));
  const std::int64_t knotsAcross = fields.Long();
  const std::int64_t knotsAlong = fields.Long();
  fields.Break();
  const std::int64_t sumAcross = Knots(fields, knotsAcross);
  fields.Break();
  const std::int64_t sumAlong = Knots(fields, knotsAlong);
  const std::int64_t rows = sumAcross - across + 1;
  const std::int64_t columns = sumAlong - along + 1;
  Points(fields, rows > 0 && columns > 0 ? rows * columns : -1, rational ? 4 : 3);
}

/**
 *  A word that must be the null one given.
 */
void Null(const Fields &fields, std::string_view null)
{
  const std::string word = fields.Word();
  if (word != null)
  {
    fields.Fail(std::format("{} in place of {} not known", word, null));
  }
}

/**
 *  The subtype's name, one of the two the releases spell it with, or a reference to an earlier subtype, whose index it
 *  then takes: false when the subtype is a reference or unknown.
 */
auto ReadSubtypeName(const Fields &fields, std::string_view old, std::string_view renamed) -> bool
{
  const std::string name = fields.Word();
  if (name == "ref")
  {
    static_cast<void>(fields.Long());
    return false;
  }
  if (name != old && name != renamed)
  {
    fields.Fail(std::format("subtype {} not known", name));
    return false;
  }
  return true;
}
} // namespace

void Curve(const Fields &fields)
{
  if (!ReadSubtypeName(fields, "exactcur", "exact_int_cur"))
  {
    return;
  }
  static_cast<void>(fields.Long(Since::R2010, SplineVersion));
  static_cast<void>(fields.Choice(Vocabulary::Approximation, Since::R2004));
  Spline(fields, 3);
  fields.Break();
  static_cast<void>(fields.Real()); // The fit tolerance.
  fields.Break();
  for (const std::string_view null : {"null_surface", "null_surface", "nullbs", "nullbs"})
  {
    fields.Break();
    Null(fields, null);
  }
  fields.Break(Since::R2000);
  fields.Bound(Since::R2000);
  fields.Bound(Since::R2000);
  for (int one = 0; one < 3; ++one)
  {
    fields.Break(Since::R2000);
    static_cast<void>(fields.Long(Since::R2000));
  }
  static_cast<void>(fields.Long(Since::R2007));
  fields.Break();
  fields.Bound(Since::R2000, 1.0);
  fields.Bound(Since::R2000, 0.0);
  static_cast<void>(fields.Choice(Vocabulary::Extension, Since::R2010));
  static_cast<void>(fields.Choice(Vocabulary::Extension, Since::R2010));
}

void Parameter(const Fields &fields)
{
  if (!ReadSubtypeName(fields, "exppc", "exp_par_cur"))
  {
    return;
  }
  Spline(fields, 2);
  fields.Break();
  static_cast<void>(fields.Real()); // The fit tolerance.
  fields.Break();
  const std::string surface = fields.Word();
  if (surface != "spline")
  {
    fields.Fail(std::format("parameter curve on a {} surface not known", surface));
    return;
  }
  static_cast<void>(fields.Logical(Vocabulary::Sense));
  fields.Open();
  Surface(fields);
  fields.Close();
  for (int bound = 0; bound < 4; ++bound)
  {
    fields.Bound();
  }
  fields.Break();
}

void Surface(const Fields &fields)
{
  if (!ReadSubtypeName(fields, "exactsur", "exact_spl_sur"))
  {
    return;
  }
  static_cast<void>(fields.Long(Since::R2007, SplineVersion));
  static_cast<void>(fields.Choice(Vocabulary::Approximation, Since::R2004));
  Sheet(fields);
  fields.Break();
  static_cast<void>(fields.Real()); // The fit tolerance.
  fields.Break();
  for (int one = 0; one < 6; ++one)
  {
    fields.Break(Since::R2000);
    static_cast<void>(fields.Long(Since::R2000));
  }
  static_cast<void>(fields.Logical(Vocabulary::Unnamed, Since::R2010));
  fields.Break();
  for (const double bound : {1.0, 0.0, 1.0, 0.0})
  {
    fields.Bound(Since::R2000, bound);
  }
  static_cast<void>(fields.Choice(Vocabulary::Extension, Since::R2007));
}
} // namespace DwgIO::Acis::Subtype
