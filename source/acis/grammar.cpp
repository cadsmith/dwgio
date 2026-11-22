/***********************************************************************************************************************
 *  @file      source/acis/grammar.cpp
 *  @brief     The fields of each ACIS record AutoCAD's files hold, release by release.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "acis/grammar.hpp"

#include "acis/fields.hpp"
#include "acis/model.hpp"
#include "acis/subtype.hpp"
#include "acis/visitor.hpp"
#include "acis/vocabulary.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <string_view>
#include <utility>

namespace DwgIO::Acis
{
namespace
{
/**
 *  The version the header record of ACIS 20800 names, which AutoCAD 2026 writes.
 */
constexpr std::string_view Modeler = "231.0.0.65535";

/**
 *  The attribute pointer and, from ACIS 20800, the history long: a transform's and the header's prefix.
 */
void Bare(const Fields &fields)
{
  static_cast<void>(fields.Pointer());
  static_cast<void>(fields.Long(Since::R2004, -1));
}

/**
 *  An entity's prefix: the attribute pointer, and from ACIS 20800 the history long and a pointer.
 */
void Entity(const Fields &fields)
{
  Bare(fields);
  static_cast<void>(fields.Pointer(Since::R2004));
}

/**
 *  An attribute's prefix: its own attributes, from ACIS 20800 the history long, then the next and previous attributes
 *  and the owner.
 */
void Attribute(const Fields &fields)
{
  Bare(fields);
  for (int pointer = 0; pointer < 3; ++pointer)
  {
    static_cast<void>(fields.Pointer());
  }
}

void Pointers(const Fields &fields, int count)
{
  for (int pointer = 0; pointer < count; ++pointer)
  {
    static_cast<void>(fields.Pointer());
  }
}

void Bounds(const Fields &fields, int count)
{
  for (int bound = 0; bound < count; ++bound)
  {
    fields.Bound();
  }
}

void Header(const Fields &fields)
{
  Bare(fields);
  static_cast<void>(fields.Text(Always, Modeler));
}

/**
 *  A body, a lump and a loop: three pointers after the prefix.
 */
void Topology(const Fields &fields)
{
  Entity(fields);
  Pointers(fields, 3);
}

/**
 *  A shell: its next, subshell, face, from ACIS 4.00 its wire, and its lump.
 */
void Shell(const Fields &fields)
{
  Entity(fields);
  Pointers(fields, 3);
  static_cast<void>(fields.Pointer(Since::R2000));
  static_cast<void>(fields.Pointer());
}

/**
 *  A face: its next, loop, shell, subshell and surface, its sense and sides, and which side of a double-sided face is
 *  inside.
 */
void Face(const Fields &fields)
{
  Entity(fields);
  Pointers(fields, 5);
  static_cast<void>(fields.Logical(Vocabulary::FaceSense));
  if (fields.Logical(Vocabulary::Sides))
  {
    static_cast<void>(fields.Logical(Vocabulary::Containment));
  }
}

/**
 *  A coedge: its next, previous, partner and edge, its sense, its loop, from ACIS 21800 a long, and its parameter
 *  curve.
 */
void Coedge(const Fields &fields)
{
  Entity(fields);
  Pointers(fields, 4);
  static_cast<void>(fields.Logical(Vocabulary::Sense));
  static_cast<void>(fields.Pointer());
  static_cast<void>(fields.Long(Since::R2013));
  static_cast<void>(fields.Pointer());
}

/**
 *  An edge: its start vertex and, from ACIS 20800, its parameter there, its end vertex and parameter, its coedge, curve
 *  and sense, and from 20800 its convexity. A parameter a release lacks is found from the geometry.
 */
void Edge(const Fields &fields)
{
  Entity(fields);
  static_cast<void>(fields.Pointer());
  static_cast<void>(fields.Real(Since::R2004));
  static_cast<void>(fields.Pointer());
  static_cast<void>(fields.Real(Since::R2004));
  Pointers(fields, 2);
  static_cast<void>(fields.Logical(Vocabulary::Sense));
  static_cast<void>(fields.Text(Since::R2004, "unknown"));
}

/**
 *  A vertex: its edge, from ACIS 21200 a long, and its point.
 */
void Vertex(const Fields &fields)
{
  Entity(fields);
  static_cast<void>(fields.Pointer());
  static_cast<void>(fields.Long(Since::R2007, 2));
  static_cast<void>(fields.Pointer());
}

void Point(const Fields &fields)
{
  Entity(fields);
  static_cast<void>(fields.Triple(Kind::Position));
}

/**
 *  A transform: its three axes and translation, its scale, and whether it rotates, reflects and shears.
 */
void Transform(const Fields &fields)
{
  Bare(fields);
  for (int row = 0; row < 4; ++row)
  {
    static_cast<void>(fields.Triple(Kind::Direction));
  }
  static_cast<void>(fields.Real());
  static_cast<void>(fields.Logical(Vocabulary::Rotation));
  static_cast<void>(fields.Logical(Vocabulary::Reflection));
  static_cast<void>(fields.Logical(Vocabulary::Shearing));
}

/**
 *  A straight line: its root and direction, and its bounds.
 */
void Straight(const Fields &fields)
{
  Entity(fields);
  static_cast<void>(fields.Triple(Kind::Position));
  static_cast<void>(fields.Triple(Kind::Direction));
  Bounds(fields, 2);
}

/**
 *  An ellipse: its centre, normal and major axis, its ratio, and its bounds.
 */
void Ellipse(const Fields &fields)
{
  Entity(fields);
  static_cast<void>(fields.Triple(Kind::Position));
  static_cast<void>(fields.Triple(Kind::Direction));
  static_cast<void>(fields.Triple(Kind::Direction));
  static_cast<void>(fields.Real());
  Bounds(fields, 2);
}

/**
 *  A plane: its root, normal and parameter direction, its sense and its bounds.
 */
void Plane(const Fields &fields)
{
  Entity(fields);
  static_cast<void>(fields.Triple(Kind::Position));
  static_cast<void>(fields.Triple(Kind::Direction));
  static_cast<void>(fields.Triple(Kind::Direction));
  static_cast<void>(fields.Logical(Vocabulary::SurfaceSense));
  Bounds(fields, 4);
}

/**
 *  A sphere: its centre and radius, its parameter direction and pole, its sense and its bounds.
 */
void Sphere(const Fields &fields)
{
  Entity(fields);
  static_cast<void>(fields.Triple(Kind::Position));
  static_cast<void>(fields.Real());
  static_cast<void>(fields.Triple(Kind::Direction));
  static_cast<void>(fields.Triple(Kind::Direction));
  static_cast<void>(fields.Logical(Vocabulary::SurfaceSense));
  Bounds(fields, 4);
}

/**
 *  A torus: its centre and normal, its major and minor radii, its parameter direction, its sense and its bounds.
 */
void Torus(const Fields &fields)
{
  Entity(fields);
  static_cast<void>(fields.Triple(Kind::Position));
  static_cast<void>(fields.Triple(Kind::Direction));
  static_cast<void>(fields.Real());
  static_cast<void>(fields.Real());
  static_cast<void>(fields.Triple(Kind::Direction));
  static_cast<void>(fields.Logical(Vocabulary::SurfaceSense));
  Bounds(fields, 4);
}

/**
 *  A cone: its base ellipse with its bounds, the sine and cosine of its half angle, from ACIS 4.00 the scale of its
 *  parameter, the length of the major axis, then its sense and bounds.
 */
void Cone(const Fields &fields)
{
  Entity(fields);
  static_cast<void>(fields.Triple(Kind::Position));
  static_cast<void>(fields.Triple(Kind::Direction));
  const std::array<double, 3> major = fields.Triple(Kind::Direction);
  static_cast<void>(fields.Real());
  Bounds(fields, 2);
  static_cast<void>(fields.Real());
  static_cast<void>(fields.Real());
  static_cast<void>(
      fields.Real(Since::R2000, std::sqrt((major[0] * major[0]) + (major[1] * major[1]) + (major[2] * major[2]))));
  static_cast<void>(fields.Logical(Vocabulary::Sense));
  Bounds(fields, 4);
}

/**
 *  AutoCAD's attribute of a solid's history: longs to the end.
 */
void History(const Fields &fields)
{
  Attribute(fields);
  while (!fields.AtEnd())
  {
    static_cast<void>(fields.Long());
  }
}

/**
 *  A named 64-bit attribute: what becomes of it on a split, a merge and a transform, from ACIS 20800 on a copy, its
 *  name and its value.
 */
void NamedAttribute(const Fields &fields)
{
  constexpr int Copy = 2;
  Attribute(fields);
  static_cast<void>(fields.Choice(Vocabulary::Split));
  static_cast<void>(fields.Choice(Vocabulary::Merge));
  static_cast<void>(fields.Choice(Vocabulary::Transformed));
  static_cast<void>(fields.Choice(Vocabulary::Copied, Since::R2004, Copy));
  static_cast<void>(fields.Text(Always, ""));
  static_cast<void>(fields.Huge());
}

/**
 *  An intcurve: its sense, its subtype, and its bounds.
 */
void Intcurve(const Fields &fields)
{
  Entity(fields);
  static_cast<void>(fields.Logical(Vocabulary::Sense));
  fields.Open();
  Subtype::Curve(fields);
  fields.Close();
  Bounds(fields, 2);
}

/**
 *  A parameter curve: its type, 0 for one of its own, its sense, its subtype on a line of its own, and two parameters.
 */
void Pcurve(const Fields &fields)
{
  Entity(fields);
  const auto type = fields.Long();
  static_cast<void>(fields.Logical(Vocabulary::Sense));
  if (type != 0)
  {
    fields.Fail("parameter curve of another curve not known");
    return;
  }
  fields.Open();
  Subtype::Parameter(fields);
  fields.Close();
  static_cast<void>(fields.Real());
  static_cast<void>(fields.Real());
}

/**
 *  A spline surface: its sense, its subtype, and its bounds.
 */
void Spline(const Fields &fields)
{
  Entity(fields);
  static_cast<void>(fields.Logical(Vocabulary::Sense));
  fields.Open();
  Subtype::Surface(fields);
  fields.Close();
  Bounds(fields, 4);
}

using Grammar = void (*)(const Fields &);

/**
 *  The records AutoCAD's files hold, by name.
 */
constexpr std::array<std::pair<std::string_view, Grammar>, 22> Grammars{{
    {"asmheader", Header},
    {"body", Topology},
    {"lump", Topology},
    {"shell", Shell},
    {"face", Face},
    {"loop", Topology},
    {"coedge", Coedge},
    {"edge", Edge},
    {"vertex", Vertex},
    {"point", Point},
    {"transform", Transform},
    {"straight-curve", Straight},
    {"ellipse-curve", Ellipse},
    {"intcurve-curve", Intcurve},
    {"pcurve", Pcurve},
    {"plane-surface", Plane},
    {"sphere-surface", Sphere},
    {"cone-surface", Cone},
    {"torus-surface", Torus},
    {"spline-surface", Spline},
    {"persubent-acadSolidHistory-attrib", History},
    {"int64_attrib-name_attrib-gen-attrib", NamedAttribute},
}};
} // namespace

auto Walk(Visitor &visitor, std::string_view name) -> bool
{
  const auto found = std::ranges::find(Grammars, name, &std::pair<std::string_view, Grammar>::first);
  if (found == Grammars.end())
  {
    return false;
  }
  const Fields fields{visitor};
  found->second(fields);
  return true;
}
} // namespace DwgIO::Acis
