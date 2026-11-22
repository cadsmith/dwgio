/***********************************************************************************************************************
 *  @file      source/audit/pointers.hpp
 *  @brief     The handles each kind of object names, for the audit.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_AUDIT_POINTERS_HPP
#define DWGIO_AUDIT_POINTERS_HPP

#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/origin.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

/**
 *  The handles an object names that the names and references checks look at.
 */
namespace DwgIO::Audit
{
/**
 *  What a named object must be.
 */
enum class Wanted : std::uint8_t
{
  Layer,
  Linetype,
  TextStyle,
  DimStyle,
  MlineStyle,
  PlotStyle,
  Block,
  MleaderStyle,
  Arrowhead,
  TableStyle,
  Material,
  VisualStyle,
  History, /**< A solid's history, an ACSH_HISTORY_CLASS object. */
  ImageDef,
  ImageReactor,
  PdfDefinition,
  DwfDefinition,
  DgnDefinition,
};

/**
 *  One handle an object names: where it is, what it must name, and where the entity says it takes its linetype or plot
 *  style from, null when it says nothing.
 */
struct Referring
{
  Handle *Target;
  DwgIO::Origin *Origin;
  Wanted Kind;
};

/**
 *  The handles an object names, in a fixed order: the entity's layer, linetype and plot style when its own, then the
 *  kind's own: a text's style, a dimension's, a multiline's, an insert's block, a layer's linetype and plot style, a
 *  dimension style's text style, a layout's block; a multileader's style, linetype, text styles, block and arrowheads,
 *  and its style's linetype, arrowhead, text style and block, those that are not null; an image's definition and
 *  reactor, an underlay's definition, the null definition and reactor of a wipeout left out; a table's block and style,
 *  a table style's text styles; then from R2007 the entity's own material and its visual styles, a layer's material,
 *  and the visual style of a view, a viewport record, a viewport entity or a layout; a solid's history when it has one.
 */
[[nodiscard]] auto ReferringOf(Object &object) -> std::vector<Referring>;

/**
 *  The word a message names a wanted kind by.
 */
[[nodiscard]] auto WordOf(Wanted wanted) -> std::string_view;

[[nodiscard]] auto Fits(const Object &object, Wanted wanted) -> bool;
} // namespace DwgIO::Audit

#endif /* DWGIO_AUDIT_POINTERS_HPP */
