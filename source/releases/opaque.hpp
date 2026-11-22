/***********************************************************************************************************************
 *  @file      source/releases/opaque.hpp
 *  @brief     An untyped object's own bits recast to another release by a layout.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_OPAQUE_HPP
#define DWGIO_RELEASES_OPAQUE_HPP

#include "dwgio/opaque.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/version.hpp"

#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace DwgIO::Encoding
{
struct Scheme;
} // namespace DwgIO::Encoding

namespace DwgIO::Releases
{
/**
 *  A layout's fields, split on blanks.
 */
[[nodiscard]] auto CollectTokens(std::string_view layout) -> std::vector<std::string_view>;

struct RecastContext
{
  Version To{};
  const Encoding::Scheme *Text{nullptr}; /**< The text's scheme before R2007. */
  /**
   *  The values of the fields only R2018 holds, in their order, written when upgrading to it: a text, a whole number.
   */
  std::span<const Content> Given{};
  /**
   *  Where the values of the fields only R2018 holds go when downgrading from it; dropped when null.
   */
  std::vector<Content> *Taken{nullptr};
};

/**
 *  An untyped object's own bits as another release holds them, walked by a layout: a list of the fields of chapter 2,
 *  separated by blanks, each copied as it is but for those that differ between releases.
 *
 *  A `T` is a text, in the data as a T up to R2004 and in the string stream as a TU from R2007; `E` the release a block
 *  element was saved in, a BL pair AutoCAD writes 25 and 104 up to R2004 and 33 and 377 from R2007; `V` the version of
 *  the object context data, a BS 3 up to R2007 and 4 from R2010; `CMC` a color, in each release's form; `X` a value of
 *  an evaluation expression, a BS type, -9999 for none, 40 for a BD, 70 for a BS or 90 for a BL, those the files hold;
 *  `N` a BL count, `MASK` the bits a BL sets among its low four, `LOOKUP` a BL row count and a BL column count, then a
 *  text per cell, its column count the count; `{ ... }` the fields between repeated the last count's times; `REST` the
 *  rest as it is; `T18` and `BL18` a text and a BL only R2018 holds, read into Taken when downgrading from it and
 *  written from Given when upgrading to it, an empty text or 0 where none is given; `B18` a bit AutoCAD writes 0 below
 *  R2018; `H` a handle of the handle stream, `H14` one only R14 and R2000 hold, a null hard owner, and `B04` a bit
 *  AutoCAD writes 1 from R2004 followed from R2013 by one it writes 0, each made where the release holds it and dropped
 *  where not; and a name one of a few groups of fields that several classes share, such as `ELEM` for a block element.
 *  A color crossing R2004 is written as AutoCAD writes it: R2000's 257 as no method when upgrading, and when
 *  downgrading no method as 257 and a true color as its nearest index. Nothing when the bits do not read to their end,
 *  or a text does not fit the release.
 */
[[nodiscard]] auto Recast(const Opaque &own, std::string_view layout, const RecastContext &with)
    -> std::optional<Opaque>;
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_OPAQUE_HPP */
