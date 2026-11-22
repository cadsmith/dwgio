/***********************************************************************************************************************
 *  @file      source/classes/catalog.cpp
 *  @brief     The classes AutoCAD declares, and those a save must add.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "classes/catalog.hpp"

#include "making/records.hpp"
#include "objects/kinds.hpp"
#include "objects/storage.hpp"
#include "objects/types.hpp"

#include "dwgio/class.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/object.hpp"
#include "dwgio/version.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace DwgIO::Classes
{
namespace
{
/**
 *  A class as AutoCAD declares it: its DXF name, its C++ name, its proxy flags and whether its objects are entities.
 */
struct Known
{
  std::string_view Dxf;
  std::string_view Cpp;
  int ProxyFlags;
  bool Entity;
  std::string_view Application{"ObjectDBX Classes"};
};

/**
 *  The application AutoCAD declares the image classes under.
 */
constexpr std::string_view Ism = "ISM";

/**
 *  The application AutoCAD declares the wipeout's class under.
 */
constexpr std::string_view WipeoutEnabler = "WipeOut|Product Desc: Object Enabler for WipeOut entity | Company: "
                                            "Autodesk, Inc. | WEB Address: www.autodesk.com";

/**
 *  The application AutoCAD declares the wipeout variables' class under, quotes and spaces as it writes them.
 */
constexpr std::string_view WipeoutApplication = "\"WipeOut|Product Desc:     WipeOut Dbx Application|Company:          "
                                                "Autodesk, Inc.|WEB Address:      www.autodesk.com\"";

constexpr std::array<Known, 45> Table{
    {{.Dxf = "ACDBDICTIONARYWDFLT", .Cpp = "AcDbDictionaryWithDefault", .ProxyFlags = 0, .Entity = false},
     {.Dxf = "ACDBPLACEHOLDER", .Cpp = "AcDbPlaceHolder", .ProxyFlags = 0, .Entity = false},
     {.Dxf = "LAYOUT", .Cpp = "AcDbLayout", .ProxyFlags = 0, .Entity = false},
     {.Dxf = "DICTIONARYVAR", .Cpp = "AcDbDictionaryVar", .ProxyFlags = 0, .Entity = false},
     {.Dxf = "SCALE", .Cpp = "AcDbScale", .ProxyFlags = 1153, .Entity = false},
     {.Dxf = "SORTENTSTABLE", .Cpp = "AcDbSortentsTable", .ProxyFlags = 0, .Entity = false},
     {.Dxf = "XRECORD", .Cpp = "AcDbXrecord", .ProxyFlags = 0, .Entity = false},
     {.Dxf = "LWPOLYLINE", .Cpp = "AcDbPolyline", .ProxyFlags = 0, .Entity = true},
     {.Dxf = "HATCH", .Cpp = "AcDbHatch", .ProxyFlags = 0, .Entity = true},
     {.Dxf = "ARC_DIMENSION", .Cpp = "AcDbArcDimension", .ProxyFlags = 1025, .Entity = true},
     {.Dxf = "LARGE_RADIAL_DIMENSION", .Cpp = "AcDbRadialDimensionLarge", .ProxyFlags = 1025, .Entity = true},
     {.Dxf = "PLOTSETTINGS", .Cpp = "AcDbPlotSettings", .ProxyFlags = 0, .Entity = false},
     {.Dxf = "IMAGE", .Cpp = "AcDbRasterImage", .ProxyFlags = 2175, .Entity = true, .Application = Ism},
     {.Dxf = "IMAGEDEF", .Cpp = "AcDbRasterImageDef", .ProxyFlags = 0, .Entity = false, .Application = Ism},
     {.Dxf = "IMAGEDEF_REACTOR",
      .Cpp = "AcDbRasterImageDefReactor",
      .ProxyFlags = 1,
      .Entity = false,
      .Application = Ism},
     {.Dxf = "RASTERVARIABLES", .Cpp = "AcDbRasterVariables", .ProxyFlags = 0, .Entity = false, .Application = Ism},
     {.Dxf = "WIPEOUT", .Cpp = "AcDbWipeout", .ProxyFlags = 2175, .Entity = true, .Application = WipeoutEnabler},
     {.Dxf = "WIPEOUTVARIABLES",
      .Cpp = "AcDbWipeoutVariables",
      .ProxyFlags = 0,
      .Entity = false,
      .Application = WipeoutApplication},
     {.Dxf = "PDFUNDERLAY", .Cpp = "AcDbPdfReference", .ProxyFlags = 4095, .Entity = true},
     {.Dxf = "DWFUNDERLAY", .Cpp = "AcDbDwfReference", .ProxyFlags = 1153, .Entity = true},
     {.Dxf = "DGNUNDERLAY", .Cpp = "AcDbDgnReference", .ProxyFlags = 1153, .Entity = true},
     {.Dxf = "PDFDEFINITION", .Cpp = "AcDbPdfDefinition", .ProxyFlags = 1153, .Entity = false},
     {.Dxf = "DWFDEFINITION", .Cpp = "AcDbDwfDefinition", .ProxyFlags = 1153, .Entity = false},
     {.Dxf = "DGNDEFINITION", .Cpp = "AcDbDgnDefinition", .ProxyFlags = 1153, .Entity = false},
     {.Dxf = "MULTILEADER", .Cpp = "AcDbMLeader", .ProxyFlags = 1025, .Entity = true},
     {.Dxf = "MLEADERSTYLE", .Cpp = "AcDbMLeaderStyle", .ProxyFlags = 4095, .Entity = false},
     {.Dxf = "ACAD_TABLE", .Cpp = "AcDbTable", .ProxyFlags = 1025, .Entity = true},
     {.Dxf = "TABLESTYLE", .Cpp = "AcDbTableStyle", .ProxyFlags = 4095, .Entity = false},
     {.Dxf = "DATATABLE", .Cpp = "AcDbDataTable", .ProxyFlags = 0, .Entity = false},
     {.Dxf = "TABLECONTENT", .Cpp = "AcDbTableContent", .ProxyFlags = 1152, .Entity = false},
     {.Dxf = "TABLEGEOMETRY", .Cpp = "AcDbTableGeometry", .ProxyFlags = 1152, .Entity = false},
     {.Dxf = "CELLSTYLEMAP", .Cpp = "AcDbCellStyleMap", .ProxyFlags = 1152, .Entity = false},
     {.Dxf = "HELIX", .Cpp = "AcDbHelix", .ProxyFlags = 4095, .Entity = true},
     {.Dxf = "MESH", .Cpp = "AcDbSubDMesh", .ProxyFlags = 0, .Entity = true},
     {.Dxf = "LIGHT", .Cpp = "AcDbLight", .ProxyFlags = 1025, .Entity = true, .Application = "SCENEOE"},
     {.Dxf = "SECTIONOBJECT", .Cpp = "AcDbSection", .ProxyFlags = 1025, .Entity = true},
     {.Dxf = "MATERIAL", .Cpp = "AcDbMaterial", .ProxyFlags = 1153, .Entity = false},
     {.Dxf = "VISUALSTYLE", .Cpp = "AcDbVisualStyle", .ProxyFlags = 4095, .Entity = false},
     {.Dxf = "SURFACE", .Cpp = "AcDbSurface", .ProxyFlags = 4095, .Entity = true},
     {.Dxf = "PLANESURFACE", .Cpp = "AcDbPlaneSurface", .ProxyFlags = 4095, .Entity = true},
     {.Dxf = "NURBSURFACE", .Cpp = "AcDbNurbSurface", .ProxyFlags = 0, .Entity = true},
     {.Dxf = "EXTRUDEDSURFACE", .Cpp = "AcDbExtrudedSurface", .ProxyFlags = 0, .Entity = true},
     {.Dxf = "LOFTEDSURFACE", .Cpp = "AcDbLoftedSurface", .ProxyFlags = 0, .Entity = true},
     {.Dxf = "REVOLVEDSURFACE", .Cpp = "AcDbRevolvedSurface", .ProxyFlags = 0, .Entity = true},
     {.Dxf = "SWEPTSURFACE", .Cpp = "AcDbSweptSurface", .ProxyFlags = 0, .Entity = true}}};

/**
 *  The item class of an object's class, the entities' being 0x1F2.
 */
constexpr int ObjectClass = 0x1F3;
} // namespace

auto FindDeclared(std::string_view dxf) -> std::optional<Class>
{
  const auto known = std::ranges::find(Table, dxf, &Known::Dxf);
  if (known == Table.end())
  {
    return std::nullopt;
  }
  Class made = Making::ClassOf(0, std::string{known->Cpp}, std::string{known->Dxf});
  made.ProxyFlags = known->ProxyFlags;
  made.Application = std::string{known->Application};
  made.ItemClass = known->Entity ? Objects::EntityClass : ObjectClass;
  return made;
}

auto CollectNeeded(const Drawing &drawing, Version release) -> std::vector<Class>
{
  std::vector<Class> needed;
  const auto declared = [&drawing, &needed](std::string_view name) -> bool
  {
    const auto named = [name](const Class &one) -> bool
    {
      return one.DxfName == name;
    };
    return std::ranges::any_of(drawing.Classes, named) || std::ranges::any_of(needed, named);
  };
  for (const Object &object : drawing.Objects)
  {
    const std::string_view name = Objects::ClassOf(object, release);
    if (name.empty() || declared(name))
    {
      continue;
    }
    if (std::optional<Class> made = FindDeclared(name))
    {
      made->Number = static_cast<int>(Objects::FirstClass + drawing.Classes.size() + needed.size());
      needed.push_back(std::move(*made));
    }
  }
  return needed;
}

void FillCounts(std::vector<Class> &classes, const Drawing &drawing)
{
  // AutoCAD's own records carry the version of the library that wrote each class; a class it declares but never
  // uses may carry none, so only a class in use gets the ObjectDBX version when it has none.
  constexpr int ObjectDbxVersion = 22;
  constexpr int ObjectDbxMaintenance = 42;
  for (Class &one : classes)
  {
    if (one.Instances == 0)
    {
      for (const Object &object : drawing.Objects)
      {
        if (Objects::ClassOf(object) == one.DxfName)
        {
          ++one.Instances;
        }
      }
    }
    if (one.Instances > 0 && one.DwgVersion == 0 && one.Maintenance == 0)
    {
      one.DwgVersion = ObjectDbxVersion;
      one.Maintenance = ObjectDbxMaintenance;
    }
  }
}
} // namespace DwgIO::Classes
