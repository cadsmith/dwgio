/***********************************************************************************************************************
 *  @file      source/fields/codes.hpp
 *  @brief     The codecs from chapter 2's bit codes to a kind's members.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FIELDS_CODES_HPP
#define DWGIO_FIELDS_CODES_HPP

#include "bits/reference.hpp"
#include "encoding/encoding.hpp"
#include "fields/context.hpp"
#include "fields/field.hpp"

#include "dwgio/color.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/vector.hpp"
#include "dwgio/version.hpp"

#include <algorithm>
#include <cstdint>
#include <format>
#include <limits>
#include <optional>
#include <string>

namespace DwgIO::Fields
{
struct BitCode
{
  static auto Read(Input &in) -> bool { return in.In->B(); }

  static void Write(bool value, Output &to) { to.To->B(value); }
};

/**
 *  BS into an int; a value outside a short is written as the nearest one and noted.
 */
struct ShortCode
{
  static auto Read(Input &in) -> int { return in.In->BS(); }

  static void Write(int value, Output &to)
  {
    const int fitted =
        std::clamp(value, int{std::numeric_limits<std::int16_t>::min()}, int{std::numeric_limits<std::int16_t>::max()});
    if (fitted != value)
    {
      Note(to, IssueCode::OutOfRange, std::format("short {} out of range, {} written", value, fitted));
    }
    to.To->BS(fitted);
  }
};

struct LongCode
{
  static auto Read(Input &in) -> std::int32_t { return in.In->BL(); }

  static void Write(std::int32_t value, Output &to) { to.To->BL(value); }
};

struct LongLongCode
{
  static auto Read(Input &in) -> std::int64_t { return in.In->BLL(); }

  static void Write(std::int64_t value, Output &to) { to.To->BLL(value); }
};

struct DoubleCode
{
  static auto Read(Input &in) -> double { return in.In->BD(); }

  static void Write(double value, Output &to) { to.To->BD(value); }
};

struct CharCode
{
  static auto Read(Input &in) -> std::uint8_t { return in.In->RC(); }

  static void Write(std::uint8_t value, Output &to) { to.To->RC(value); }
};

struct RawShortCode
{
  static auto Read(Input &in) -> std::uint16_t { return in.In->RS(); }

  static void Write(std::uint16_t value, Output &to) { to.To->RS(value); }
};

struct RawLongCode
{
  static auto Read(Input &in) -> std::uint32_t { return in.In->RL(); }

  static void Write(std::uint32_t value, Output &to) { to.To->RL(value); }
};

struct RawDoubleCode
{
  static auto Read(Input &in) -> double { return in.In->RD(); }

  static void Write(double value, Output &to) { to.To->RD(value); }
};

/**
 *  RC into an int, for an item a release stores as a byte and another as a short; a value outside a byte is written as
 *  the nearest one and noted.
 */
struct ByteCode
{
  static auto Read(Input &in) -> int { return in.In->RC(); }

  static void Write(int value, Output &to)
  {
    const int fitted = std::clamp(value, 0, int{std::numeric_limits<std::uint8_t>::max()});
    if (fitted != value)
    {
      Note(to, IssueCode::OutOfRange, std::format("byte {} out of range, {} written", value, fitted));
    }
    to.To->RC(static_cast<std::uint8_t>(fitted));
  }
};

struct RawPoint2Code
{
  static auto Read(Input &in) -> Vector
  {
    Vector point;
    point.X = in.In->RD();
    point.Y = in.In->RD();
    return point;
  }

  static void Write(const Vector &value, Output &to)
  {
    to.To->RD(value.X);
    to.To->RD(value.Y);
  }
};

/**
 *  TV into a string, through the context's scheme, or UTF-8 when it has none, from the string stream from R2007 when
 *  there is one; a text longer than a BS counts is cut and noted.
 */
struct TextCode
{
  static auto Scheme(const Context &with) -> const Encoding::Scheme &
  {
    static const Encoding::Scheme utf8 = Encoding::Utf8();
    return with.Text == nullptr ? utf8 : *with.Text;
  }

  static auto Read(const Input &in) -> std::string { return TextIn(in).TV(in.With.Release, Scheme(in.With)); }

  static void Write(const std::string &value, const Output &to)
  {
    if (const std::optional<std::size_t> cut = TextOut(to).TV(value, to.With.Release, Scheme(to.With)))
    {
      Note(to, IssueCode::OutOfRange, std::format("text cut to {} code units", *cut));
    }
  }
};

struct Point2Code
{
  static auto Read(Input &in) -> Vector { return in.In->Point2(); }

  static void Write(const Vector &value, Output &to) { to.To->Point2(value); }
};

struct Point3Code
{
  static auto Read(Input &in) -> Vector { return in.In->Point3(); }

  static void Write(const Vector &value, Output &to) { to.To->Point3(value); }
};

struct ThicknessCode
{
  static auto Read(Input &in) -> double { return in.In->BT(in.With.Release); }

  static void Write(double value, Output &to) { to.To->BT(value, to.With.Release); }
};

struct ExtrusionCode
{
  static auto Read(Input &in) -> Vector { return in.In->BE(in.With.Release); }

  static void Write(const Vector &value, Output &to) { to.To->BE(value, to.With.Release); }
};

/**
 *  CMC into a color, its names in the string stream from R2007 when there is one.
 */
struct ColorCode
{
  static auto Read(Input &in) -> Color { return in.In->CMC(in.With.Release, TextIn(in)); }

  static void Write(const Color &value, Output &to) { to.To->CMC(value, to.With.Release, TextOut(to)); }
};

/**
 *  H into a handle, with the code the layout expects: a file's other code is noted and the handle kept.
 *
 *  From R2007 the handle is in the handle stream when there is one, unless the field stays with the data, as $HANDSEED
 *  does.
 */
template <typename KindT>
struct HandleField
{
  Handle KindT::*At{nullptr};
  int Code{};  /**< The reference code the specification gives the field. */
  bool Kept{}; /**< Whether the handle stays in the data from R2007.      */

  [[nodiscard]] auto From(Input &in) const -> Bits::Reader &
  {
    return !Kept && in.With.Release >= Version::R2007 && in.Handles != nullptr ? *in.Handles : *in.In;
  }

  [[nodiscard]] auto To(Output &to) const -> Bits::Writer &
  {
    return !Kept && to.With.Release >= Version::R2007 && to.Handles != nullptr ? *to.Handles : *to.To;
  }

  /**
   *  Reads the handle; false when the reader failed, before or now.
   */
  auto Read(KindT &kind, Input &in) const -> bool
  {
    if (in.In->HasFailed())
    {
      return false;
    }
    Bits::Reader &from = From(in);
    const Bits::Reference found = from.H();
    if (from.HasFailed())
    {
      return false;
    }
    kind.*At = found.Value;
    if (found.Code != Code)
    {
      Note(in, IssueCode::Damaged,
           std::format("handle {} with code {}, {} expected", HexOf(found.Value), found.Code, Code));
    }
    return true;
  }

  auto Write(const KindT &kind, Output &to) const -> bool
  {
    To(to).H(Bits::Reference{.Code = Code, .Value = kind.*At});
    return true;
  }
};

/**
 *  Reads a handle from the handle stream; false with no stream, a failed data reader or a stream cut short.
 *
 *  An offset code counts from the record's own handle and passes as a soft pointer; any other code than the layout's is
 *  noted and the handle kept.
 */
inline auto ReadLink(Handle &value, int code, const Input &in) -> bool
{
  if (in.In->HasFailed() || in.Handles == nullptr)
  {
    return false;
  }
  const Bits::Reference found = in.Handles->H();
  value = Bits::Resolve(found, in.With.Record);
  if (in.Handles->HasFailed())
  {
    return false;
  }
  if (found.Code != code && !Bits::IsOffset(found.Code))
  {
    Note(in, IssueCode::Damaged,
         std::format("handle {} with code {}, {} expected", HexOf(found.Value), found.Code, code));
  }
  return true;
}

/**
 *  Writes a handle to the handle stream with the layout's code; false with no stream.
 */
inline auto WriteLink(const Handle &value, int code, const Output &to) -> bool
{
  if (to.Handles == nullptr)
  {
    return false;
  }
  to.Handles->H(Bits::Reference{.Code = code, .Value = value});
  return true;
}

/**
 *  A handle member read from and written to the handle stream.
 */
template <typename KindT>
struct LinkField
{
  Handle KindT::*At{nullptr};
  int Code{}; /**< The reference code the specification gives the handle. */

  auto Read(KindT &kind, Input &in) const -> bool { return ReadLink(kind.*At, Code, in); }

  auto Write(const KindT &kind, Output &to) const -> bool { return WriteLink(kind.*At, Code, to); }
};

/**
 *  A handle of a list, read from and written to the handle stream.
 */
struct LinkElement
{
  int Code{}; /**< The reference code the specification gives the handles. */

  auto Read(Handle &value, Input &in) const -> bool { return ReadLink(value, Code, in); }

  auto Write(const Handle &value, Output &to) const -> bool { return WriteLink(value, Code, to); }
};

template <typename KindT>
auto Bit(bool KindT::*at) -> Member<KindT, bool, BitCode>
{
  return {at};
}

template <typename KindT>
auto Short(int KindT::*at) -> Member<KindT, int, ShortCode>
{
  return {at};
}

/**
 *  BS, the value itself.
 */
inline auto Short() -> Self<int, ShortCode>
{
  return {};
}

/**
 *  3BD, the value itself.
 */
inline auto Point3() -> Self<Vector, Point3Code>
{
  return {};
}

template <typename KindT>
auto Long(std::int32_t KindT::*at) -> Member<KindT, std::int32_t, LongCode>
{
  return {at};
}

template <typename KindT>
auto LongLong(std::int64_t KindT::*at) -> Member<KindT, std::int64_t, LongLongCode>
{
  return {at};
}

template <typename KindT>
auto Double(double KindT::*at) -> Member<KindT, double, DoubleCode>
{
  return {at};
}

template <typename KindT>
auto Char(std::uint8_t KindT::*at) -> Member<KindT, std::uint8_t, CharCode>
{
  return {at};
}

/**
 *  RC into an int.
 */
template <typename KindT>
auto Byte(int KindT::*at) -> Member<KindT, int, ByteCode>
{
  return {at};
}

template <typename KindT>
auto RawPoint2(Vector KindT::*at) -> Member<KindT, Vector, RawPoint2Code>
{
  return {at};
}

template <typename KindT>
auto RawShort(std::uint16_t KindT::*at) -> Member<KindT, std::uint16_t, RawShortCode>
{
  return {at};
}

template <typename KindT>
auto RawLong(std::uint32_t KindT::*at) -> Member<KindT, std::uint32_t, RawLongCode>
{
  return {at};
}

template <typename KindT>
auto RawDouble(double KindT::*at) -> Member<KindT, double, RawDoubleCode>
{
  return {at};
}

template <typename KindT>
auto Text(std::string KindT::*at) -> Member<KindT, std::string, TextCode>
{
  return {at};
}

template <typename KindT>
auto Point2(Vector KindT::*at) -> Member<KindT, Vector, Point2Code>
{
  return {at};
}

template <typename KindT>
auto Point3(Vector KindT::*at) -> Member<KindT, Vector, Point3Code>
{
  return {at};
}

template <typename KindT>
auto Thickness(double KindT::*at) -> Member<KindT, double, ThicknessCode>
{
  return {at};
}

template <typename KindT>
auto Extrusion(Vector KindT::*at) -> Member<KindT, Vector, ExtrusionCode>
{
  return {at};
}

/**
 *  H with the code the specification gives the field, in the handle stream from R2007.
 */
template <typename KindT>
auto StreamHandle(Handle KindT::*at, int code) -> HandleField<KindT>
{
  return {.At = at, .Code = code, .Kept = false};
}

/**
 *  H with the code the specification gives the field, in the data in every release.
 */
template <typename KindT>
auto DataHandle(Handle KindT::*at, int code) -> HandleField<KindT>
{
  return {.At = at, .Code = code, .Kept = true};
}

/**
 *  H in the handle stream, with the specification's code.
 */
template <typename KindT>
auto Link(Handle KindT::*at, int code) -> LinkField<KindT>
{
  return {.At = at, .Code = code};
}

/**
 *  H in the handle stream, the element of a list.
 */
inline auto Link(int code) -> LinkElement
{
  return {.Code = code};
}

template <typename KindT>
auto CmColor(Color KindT::*at) -> Member<KindT, Color, ColorCode>
{
  return {at};
}
} // namespace DwgIO::Fields

#endif /* DWGIO_FIELDS_CODES_HPP */
