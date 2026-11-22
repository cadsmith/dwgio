/***********************************************************************************************************************
 *  @file      source/fields/context.hpp
 *  @brief     What a layout reads and writes with besides the bits.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FIELDS_CONTEXT_HPP
#define DWGIO_FIELDS_CONTEXT_HPP

#include "bits/reader.hpp"
#include "bits/writer.hpp"

#include "dwgio/class.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/version.hpp"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace DwgIO::Encoding
{
struct Scheme;
} // namespace DwgIO::Encoding

namespace DwgIO::Fields
{
/**
 *  What a layout needs besides the bits: the release, the text scheme, and where to note what it finds.
 */
struct Context
{
  Version Release{};
  /**
   *  The scheme the text is in before R2007; null when the layout has no text.
   */
  const Encoding::Scheme *Text{nullptr};
  std::vector<Issue> *Issues{nullptr}; /**< Where issues go; null drops them.                                      */
  std::size_t Offset{};                /**< The file offset of the reader's first byte, so an issue can say where. */
  Handle Record{};                     /**< The record being read or written, for the issues.                      */
  /**
   *  The drawing's classes, which number a kind stored under a class on write; null when none are known.
   */
  const std::vector<Class> *Classes{nullptr};
};

/**
 *  A reader and the context it reads in.
 */
struct Input
{
  Bits::Reader *In{nullptr};
  /**
   *  An object's handle stream, where its handles are; null outside an object.
   */
  Bits::Reader *Handles{nullptr};
  /**
   *  From R2007, the string stream where the text is; null reads the text from the bits.
   */
  Bits::Reader *Strings{nullptr};
  Context With{};
  /**
   *  The bit where an object's data ends, before its string stream or its handles; zero for the reader's end.
   */
  std::size_t DataEnd{};
  std::size_t StringsEnd{}; /**< The bit where the string stream ends; zero for the reader's end. */
};

/**
 *  A writer and the context it writes in.
 */
struct Output
{
  Bits::Writer *To{nullptr};
  /**
   *  An object's handle stream, where its handles go; null outside an object.
   */
  Bits::Writer *Handles{nullptr};
  /**
   *  From R2007, the string stream where the text goes; null writes the text with the bits.
   */
  Bits::Writer *Strings{nullptr};
  Context With{};
};

/**
 *  Where the text is read: the string stream from R2007 when there is one, else the bits.
 */
inline auto TextIn(const Input &in) -> Bits::Reader &
{
  return in.With.Release >= Version::R2007 && in.Strings != nullptr ? *in.Strings : *in.In;
}

/**
 *  Where the text is written: the string stream from R2007 when there is one, else the bits.
 */
inline auto TextOut(const Output &to) -> Bits::Writer &
{
  return to.With.Release >= Version::R2007 && to.Strings != nullptr ? *to.Strings : *to.To;
}

/**
 *  Notes an issue at the reader's byte, when the context keeps them.
 */
inline void Note(const Input &in, IssueCode code, std::string what)
{
  if (in.With.Issues != nullptr)
  {
    const std::size_t bit = in.In->Position();
    in.With.Issues->push_back(
        Issue{.Code = code, .Offset = in.With.Offset + (bit / 8), .Subject = in.With.Record, .What = std::move(what)});
  }
}

/**
 *  Notes an issue of the object written, when the context keeps them.
 */
inline void Note(const Output &to, IssueCode code, std::string what)
{
  if (to.With.Issues != nullptr)
  {
    to.With.Issues->push_back(Issue{.Code = code, .Subject = to.With.Record, .What = std::move(what)});
  }
}
} // namespace DwgIO::Fields

#endif /* DWGIO_FIELDS_CONTEXT_HPP */
