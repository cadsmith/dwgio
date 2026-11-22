/***********************************************************************************************************************
 *  @file      source/fields/opaque.hpp
 *  @brief     The bits of an object dwgio does not type, kept as read.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FIELDS_OPAQUE_HPP
#define DWGIO_FIELDS_OPAQUE_HPP

#include "bits/byte.hpp"
#include "fields/context.hpp"

#include "dwgio/opaque.hpp"

#include <algorithm>
#include <cstddef>

namespace DwgIO::Fields
{
/**
 *  The bits from the reader's position to `end`, or to its end when `end` is zero or beyond it.
 */
inline auto ReadRest(Bits::Reader &in, std::size_t end) -> BitString
{
  const std::size_t last = end == 0 ? in.Size() : std::min(end, in.Size());
  BitString run{.Bytes = {}, .Size = last - std::min(in.Position(), last)};
  for (std::size_t done = 0; done < run.Size; done += Bits::Byte)
  {
    const int count = static_cast<int>(std::min<std::size_t>(Bits::Byte, run.Size - done));
    run.Bytes.push_back(static_cast<std::byte>(in.Bits(count) << (Bits::Byte - static_cast<unsigned>(count))));
  }
  return run;
}

inline void Put(const BitString &run, Bits::Writer &to)
{
  for (std::size_t done = 0; done < run.Size; done += Bits::Byte)
  {
    const int count = static_cast<int>(std::min<std::size_t>(Bits::Byte, run.Size - done));
    to.Bits(std::to_integer<std::uint64_t>(run.Bytes.at(done / Bits::Byte)) >>
                (Bits::Byte - static_cast<unsigned>(count)),
            count);
  }
}

/**
 *  Keeps what is left of the object's streams, in the release read.
 */
inline auto ReadOpaque(Opaque &opaque, const Input &in) -> bool
{
  opaque.Release = in.With.Release;
  opaque.Data = ReadRest(*in.In, in.DataEnd);
  if (in.Handles != nullptr)
  {
    opaque.Handles = ReadRest(*in.Handles, 0);
  }
  if (in.Strings != nullptr)
  {
    opaque.Strings = ReadRest(*in.Strings, in.StringsEnd);
  }
  return true;
}

/**
 *  Writes the kept bits back; false, noted, when they are of another release than the one written.
 */
inline auto WriteOpaque(const Opaque &opaque, const Output &to) -> bool
{
  if (opaque.Release != to.With.Release)
  {
    Note(to, IssueCode::NotKept, "bits kept from another release not written");
    return false;
  }
  Put(opaque.Data, *to.To);
  if (to.Handles != nullptr)
  {
    Put(opaque.Handles, *to.Handles);
  }
  if (to.Strings != nullptr)
  {
    Put(opaque.Strings, *to.Strings);
  }
  return true;
}
} // namespace DwgIO::Fields

#endif /* DWGIO_FIELDS_OPAQUE_HPP */
