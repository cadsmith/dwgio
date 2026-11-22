/***********************************************************************************************************************
 *  @file      source/fields/field.hpp
 *  @brief     A field over one member, or over a whole value.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FIELDS_FIELD_HPP
#define DWGIO_FIELDS_FIELD_HPP

#include "fields/context.hpp"

namespace DwgIO::Fields
{
/**
 *  A field over one member of a kind: the codec reads the member's value and writes it back.
 */
template <typename KindT, typename ValueT, typename CodecT>
struct Member
{
  ValueT KindT::*At{nullptr};

  /**
   *  Reads the member; false when the reader failed, before or now, the member untouched when before.
   */
  auto Read(KindT &kind, Input &in) const -> bool
  {
    if (in.In->HasFailed())
    {
      return false;
    }
    kind.*At = CodecT::Read(in);
    return !in.In->HasFailed();
  }

  auto Write(const KindT &kind, Output &to) const -> bool
  {
    CodecT::Write(kind.*At, to);
    return true;
  }
};

/**
 *  A field over a whole value, as an element of a list is: the codec reads the value itself.
 */
template <typename ValueT, typename CodecT>
struct Self
{
  /**
   *  Reads the value; false when the reader failed.
   */
  auto Read(ValueT &value, Input &in) const -> bool
  {
    value = CodecT::Read(in);
    return !in.In->HasFailed();
  }

  auto Write(const ValueT &value, Output &to) const -> bool
  {
    CodecT::Write(value, to);
    return true;
  }
};
} // namespace DwgIO::Fields

#endif /* DWGIO_FIELDS_FIELD_HPP */
