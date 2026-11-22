/***********************************************************************************************************************
 *  @file      source/tables/shapes/record.hpp
 *  @brief     What every table record opens with in the file.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TABLES_SHAPES_RECORD_HPP
#define DWGIO_TABLES_SHAPES_RECORD_HPP

#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/since.hpp"

#include "dwgio/tables/record.hpp"

namespace DwgIO::Tables::Shapes
{
// The code a handle reference carries for each kind of link.
constexpr int HardPointer = 5;
constexpr int HardOwner = 3;
constexpr int SoftPointer = 4;
constexpr int SoftOwner = 2;

/**
 *  The file stores the xref index plus one.
 */
inline auto ReadXref(Record &record, const Fields::Input &in) -> bool
{
  record.XrefIndex = in.In->BS() - 1;
  return !in.In->HasFailed();
}

inline auto WriteXref(const Record &record, const Fields::Output &to) -> bool
{
  to.To->BS(record.XrefIndex + 1);
  return true;
}

/**
 *  What every table record opens with, chapter 20.4.54 and those of the other records.
 *
 *  AutoCAD's files from R2007 hold no xref index, though the specification keeps it.
 */
inline auto RecordLayout()
{
  return Fields::Layout(Fields::Text(&Record::Name), Fields::Bit(&Record::Referenced),
                        Fields::Until(Version::R2004, Fields::Custom(ReadXref, WriteXref)),
                        Fields::Bit(&Record::Dependent), Fields::Link(&Record::Source, HardPointer));
}
} // namespace DwgIO::Tables::Shapes

#endif /* DWGIO_TABLES_SHAPES_RECORD_HPP */
