/***********************************************************************************************************************
 *  @file      source/file/paged/names.cpp
 *  @brief     The sections AutoCAD writes in a paged file, and their names.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/paged/names.hpp"

#include "file/names.hpp"

#include <algorithm>
#include <array>

namespace DwgIO::File::Paged
{
namespace
{
constexpr std::uint32_t Small = 0x80;
constexpr std::uint32_t Summary = 0x80;
constexpr std::uint32_t LaterSummary = 0x100;
constexpr std::uint32_t Image = 0x800;
constexpr std::uint32_t Info = 0x300;
constexpr std::uint32_t History = 0x600;

constexpr std::array Table{
    Known{.Acdb = "", .Name = "", .Id = 0, .Compressed = true, .PageSize = UsualPage},
    Known{.Acdb = Names::AcdbHeader, .Name = Names::Header, .Id = 1, .Compressed = true, .PageSize = UsualPage},
    Known{.Acdb = Names::AcdbAuxHeader, .Name = Names::Auxiliary, .Id = 2, .Compressed = true, .PageSize = UsualPage},
    Known{.Acdb = Names::AcdbClasses, .Name = Names::Classes, .Id = 3, .Compressed = true, .PageSize = UsualPage},
    Known{.Acdb = Names::AcdbHandles, .Name = Names::Map, .Id = 4, .Compressed = true, .PageSize = UsualPage},
    Known{.Acdb = Names::AcdbTemplate, .Name = Names::Measurement, .Id = 5, .Compressed = true, .PageSize = UsualPage},
    Known{.Acdb = Names::ObjFreeSpace, .Name = Names::ObjFreeSpace, .Id = 6, .Compressed = true, .PageSize = UsualPage},
    Known{.Acdb = Names::AcdbObjects, .Name = Names::Objects, .Id = 7, .Compressed = true, .PageSize = UsualPage},
    Known{.Acdb = Names::RevHistory, .Name = Names::RevHistory, .Id = 8, .Compressed = true, .PageSize = UsualPage},
    Known{.Acdb = Names::SummaryInfo, .Name = Names::SummaryInfo, .Id = 9, .Compressed = false, .PageSize = Summary},
    Known{.Acdb = Names::AcdbPreview, .Name = Names::Preview, .Id = 10, .Compressed = false, .PageSize = Image},
    Known{.Acdb = Names::AppInfo, .Name = Names::AppInfo, .Id = 11, .Compressed = false, .PageSize = Info},
    Known{.Acdb = Names::AppInfoHistory,
          .Name = Names::AppInfoHistory,
          .Id = 12,
          .Compressed = false,
          .PageSize = History},
    Known{.Acdb = Names::DataStore, .Name = Names::DataStore, .Id = 13, .Compressed = true, .PageSize = UsualPage},
    Known{.Acdb = Names::Security, .Name = Names::Security, .Id = 14, .Compressed = false, .PageSize = UsualPage},
    Known{.Acdb = Names::FileDepList, .Name = Names::FileDepList, .Id = 15, .Compressed = false, .PageSize = Small},
    Known{.Acdb = Names::VbaProject, .Name = Names::VbaProject, .Id = 16, .Compressed = false, .PageSize = Small},
    Known{.Acdb = Names::Signature, .Name = Names::Signature, .Id = 17, .Compressed = false, .PageSize = UsualPage},
};
} // namespace

auto Find(std::string_view acdb) -> const Known *
{
  const auto known = std::ranges::find(Table, acdb, &Known::Acdb);
  return known == Table.end() ? nullptr : &*known;
}

auto PageOf(const Known &known, Version release) -> std::uint32_t
{
  return known.Acdb == Names::SummaryInfo && release >= Version::R2010 ? LaterSummary : known.PageSize;
}

auto SectionNameOf(std::string_view acdb) -> std::string
{
  const Known *known = Find(acdb);
  return std::string{known == nullptr ? acdb : known->Name};
}

auto AcdbOf(std::string_view name) -> std::string
{
  const auto known = std::ranges::find(Table, name, &Known::Name);
  return std::string{known == Table.end() ? name : known->Acdb};
}
} // namespace DwgIO::File::Paged
