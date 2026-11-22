/***********************************************************************************************************************
 *  @file      source/fields/custom.hpp
 *  @brief     A field read and written by hand.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FIELDS_CUSTOM_HPP
#define DWGIO_FIELDS_CUSTOM_HPP

#include "fields/context.hpp"

namespace DwgIO::Fields
{
/**
 *  A field read and written by hand, for what no code says.
 */
template <typename KindT>
struct CustomField
{
  /**
   *  Reads into the kind; false to stop the layout.
   */
  bool (*In)(KindT &kind, const Input &in){nullptr};
  /**
   *  Writes from the kind; false to stop the layout.
   */
  bool (*Out)(const KindT &kind, const Output &to){nullptr};

  auto Read(KindT &kind, Input &in) const -> bool
  {
    if (in.In->HasFailed())
    {
      return false;
    }
    return In(kind, in);
  }

  auto Write(const KindT &kind, Output &to) const -> bool { return Out(kind, to); }
};

template <typename KindT>
auto Custom(bool (*in)(KindT &kind, const Input &in), bool (*out)(const KindT &kind, const Output &to))
    -> CustomField<KindT>
{
  return {.In = in, .Out = out};
}
} // namespace DwgIO::Fields

#endif /* DWGIO_FIELDS_CUSTOM_HPP */
