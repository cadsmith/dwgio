/***********************************************************************************************************************
 *  @file      source/objects/shape.hpp
 *  @brief     How a kind lies in the file, the template each kind's shape specializes.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_SHAPE_HPP
#define DWGIO_OBJECTS_SHAPE_HPP

#include "dwgio/base.hpp"

namespace DwgIO::Objects
{
/**
 *  How a kind lies in the file: `Types`, the type codes it is stored under; `Name`, its DXF name; `Layout()`, its
 *  fields after the common header, data and handles alike.
 */
template <typename KindT>
struct Shape;

/**
 *  The kind's Base: a table record's is in its Record.
 */
template <typename KindT>
auto BaseOfKind(KindT &kind) -> auto &
{
  if constexpr (requires { kind.Record.Base; })
  {
    return kind.Record.Base;
  }
  else
  {
    return kind.Base;
  }
}

template <typename KindT>
auto CommonOfKind(KindT &kind) -> auto &
{
  return kind.Common;
}
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_SHAPE_HPP */
