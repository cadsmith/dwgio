/***********************************************************************************************************************
 *  @file      source/objects/shapes/table/value.hpp
 *  @brief     How a table's values lie in the file, chapter 20.4.99.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_SHAPES_TABLE_VALUE_HPP
#define DWGIO_OBJECTS_SHAPES_TABLE_VALUE_HPP

#include "fields/codes.hpp"
#include "fields/context.hpp"
#include "fields/list.hpp"

#include "dwgio/objects/tablestyles/value.hpp"
#include "dwgio/version.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace DwgIO::Objects::Shapes::Table
{
using Objects::TableStyles::Value;

/**
 *  Chapter 20.4.99's value types the library reads, and the flag for a value with no data.
 */
namespace Values
{
constexpr unsigned Bare = 1U;
constexpr int Unknown = 0;
constexpr int Long = 1;
constexpr int Real = 2;
constexpr int Text = 4;
constexpr int Date = 8;
constexpr int General = 512;
} // namespace Values

/**
 *  None, a long, a double, or bytes.
 */
inline auto IsKnownValue(const Value &value) -> bool
{
  if ((static_cast<unsigned>(value.Flags) & Values::Bare) != 0)
  {
    return true;
  }
  switch (value.Type)
  {
  case Values::Unknown:
  case Values::Long:
  case Values::Real:
  case Values::Text:
  case Values::Date:
  case Values::General:
    return true;
  default:
    return false;
  }
}

/**
 *  A BL count, then the bytes; false when the count outruns the data.
 */
inline auto ReadSized(Value &value, const Fields::Input &in) -> bool
{
  const std::int32_t count = in.In->BL();
  if (in.In->HasFailed() || count < 0 || static_cast<std::size_t>(count) > Fields::Room(in))
  {
    in.In->Seek(in.In->Size() + 1);
    return false;
  }
  value.Bytes = in.In->Bytes(static_cast<std::size_t>(count));
  return true;
}

/**
 *  From R2007 the flags may say there is no data; before R2007 a string's bytes are in the code page.
 *
 *  False when the reader failed or the type is one the library does not read.
 */
inline auto ReadValue(Value &value, const Fields::Input &in) -> bool
{
  const bool later = in.With.Release >= Version::R2007;
  value.Flags = later ? in.In->BL() : 0;
  value.Type = in.In->BL();
  if (!IsKnownValue(value))
  {
    return false;
  }
  if ((static_cast<unsigned>(value.Flags) & Values::Bare) == 0)
  {
    if (value.Type == Values::Real)
    {
      value.Real = in.In->BD();
    }
    else if (value.Type == Values::Unknown || value.Type == Values::Long)
    {
      value.Long = in.In->BL();
    }
    else if (!ReadSized(value, in))
    {
      return false;
    }
  }
  if (later)
  {
    value.Unit = in.In->BL();
    value.Format = Fields::TextCode::Read(in);
    value.Text = Fields::TextCode::Read(in);
  }
  return !in.In->HasFailed();
}

inline void WriteValue(const Value &value, const Fields::Output &to)
{
  const bool later = to.With.Release >= Version::R2007;
  if (later)
  {
    to.To->BL(value.Flags);
  }
  to.To->BL(value.Type);
  if ((static_cast<unsigned>(value.Flags) & Values::Bare) == 0)
  {
    if (value.Type == Values::Real)
    {
      to.To->BD(value.Real);
    }
    else if (value.Type == Values::Unknown || value.Type == Values::Long)
    {
      to.To->BL(value.Long);
    }
    else
    {
      to.To->BL(static_cast<std::int32_t>(value.Bytes.size()));
      to.To->Raw(std::span{value.Bytes});
    }
  }
  if (later)
  {
    to.To->BL(value.Unit);
    Fields::TextCode::Write(value.Format, to);
    Fields::TextCode::Write(value.Text, to);
  }
}
} // namespace DwgIO::Objects::Shapes::Table

#endif /* DWGIO_OBJECTS_SHAPES_TABLE_VALUE_HPP */
