/***********************************************************************************************************************
 *  @file      source/objects/types.cpp
 *  @brief     The fixed type codes of chapter 20.3 and their names.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "objects/types.hpp"

#include <array>
#include <cstddef>

namespace DwgIO::Objects
{
namespace
{
struct Fixed
{
  std::string_view Name;
  bool Entity{};
};

constexpr auto E(std::string_view name) -> Fixed
{
  return Fixed{.Name = name, .Entity = true};
}

constexpr auto O(std::string_view name) -> Fixed
{
  return Fixed{.Name = name, .Entity = false};
}

constexpr std::array Table{
    O(""),
    E("TEXT"),
    E("ATTRIB"),
    E("ATTDEF"),
    E("BLOCK"),
    E("ENDBLK"),
    E("SEQEND"),
    E("INSERT"),
    E("MINSERT"),
    O(""),
    E("VERTEX_2D"),
    E("VERTEX_3D"),
    E("VERTEX_MESH"),
    E("VERTEX_PFACE"),
    E("VERTEX_PFACE_FACE"),
    E("POLYLINE_2D"),
    E("POLYLINE_3D"),
    E("ARC"),
    E("CIRCLE"),
    E("LINE"),
    E("DIMENSION_ORDINATE"),
    E("DIMENSION_LINEAR"),
    E("DIMENSION_ALIGNED"),
    E("DIMENSION_ANG3PT"),
    E("DIMENSION_ANG2LN"),
    E("DIMENSION_RADIUS"),
    E("DIMENSION_DIAMETER"),
    E("POINT"),
    E("3DFACE"),
    E("POLYLINE_PFACE"),
    E("POLYLINE_MESH"),
    E("SOLID"),
    E("TRACE"),
    E("SHAPE"),
    E("VIEWPORT"),
    E("ELLIPSE"),
    E("SPLINE"),
    E("REGION"),
    E("3DSOLID"),
    E("BODY"),
    E("RAY"),
    E("XLINE"),
    O("DICTIONARY"),
    E("OLEFRAME"),
    E("MTEXT"),
    E("LEADER"),
    E("TOLERANCE"),
    E("MLINE"),
    O("BLOCK_CONTROL"),
    O("BLOCK_HEADER"),
    O("LAYER_CONTROL"),
    O("LAYER"),
    O("STYLE_CONTROL"),
    O("STYLE"),
    O(""),
    O(""),
    O("LTYPE_CONTROL"),
    O("LTYPE"),
    O(""),
    O(""),
    O("VIEW_CONTROL"),
    O("VIEW"),
    O("UCS_CONTROL"),
    O("UCS"),
    O("VPORT_CONTROL"),
    O("VPORT"),
    O("APPID_CONTROL"),
    O("APPID"),
    O("DIMSTYLE_CONTROL"),
    O("DIMSTYLE"),
    O("VP_ENT_HDR_CONTROL"),
    O("VP_ENT_HDR"),
    O("GROUP"),
    O("MLINESTYLE"),
    E("OLE2FRAME"),
    O("DUMMY"),
    O("LONG_TRANSACTION"),
    E("LWPOLYLINE"),
    E("HATCH"),
    O("XRECORD"),
    O("ACDBPLACEHOLDER"),
    O("VBA_PROJECT"),
    O("LAYOUT"),
};

constexpr std::uint16_t ProxyEntity = 0x1F2;

constexpr std::uint16_t ProxyObject = 0x1F3;

auto Lookup(std::uint16_t type) -> Fixed
{
  if (type == ProxyEntity)
  {
    return E("ACAD_PROXY_ENTITY");
  }
  if (type == ProxyObject)
  {
    return O("ACAD_PROXY_OBJECT");
  }
  return type < Table.size() ? Table.at(type) : O("");
}
} // namespace

auto TypeNameOf(std::uint16_t type) -> std::string_view
{
  return Lookup(type).Name;
}

auto IsEntity(std::uint16_t type) -> bool
{
  return Lookup(type).Entity;
}
} // namespace DwgIO::Objects
