/***********************************************************************************************************************
 *  @file      source/releases/untyped.hpp
 *  @brief     The untyped objects a save carries to another release, and those it leaves out.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_UNTYPED_HPP
#define DWGIO_RELEASES_UNTYPED_HPP

#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/opaque.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/unknown.hpp"
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
struct CarryContext
{
  Version From{};
  Version To{};
  const Encoding::Scheme *Text{nullptr}; /**< The text's scheme before R2007. */
  /**
   *  The values of the fields only R2018 holds, in their order, written when upgrading to it.
   */
  std::span<const Content> Given{};
  /**
   *  Where the values of the fields only R2018 holds go when downgrading from it; dropped when null.
   */
  std::vector<Content> *Taken{nullptr};
};

/**
 *  An untyped object's own bits as the release written holds them, where AutoCAD writes its class the same way in both
 *  releases, or differently in a way the class's layout recasts; as they are when they are of the release written
 *  already, as a save makes them where it moves fields into a record.
 *
 *  Nothing for a class not known, one whose bits change between the releases in a way no layout recasts, or bits that
 *  do not read by the layout.
 */
[[nodiscard]] auto Carry(const Unknown &unknown, const CarryContext &with) -> std::optional<Opaque>;

/**
 *  The untyped objects a save leaves out with those it cannot carry, which AutoCAD refuses or mends in part: the whole
 *  associative network (its actions, dependencies, parameters, bodies and variables) when one of its objects cannot
 *  cross, and an evaluation graph with all it owns when the graph or one of those cannot, a solid's history that owns
 *  the graph with it.
 */
[[nodiscard]] auto CollectTorn(const Drawing &drawing, std::span<const Handle> leftOut) -> std::vector<Handle>;

/**
 *  What CollectTorn leaves an object out with: "associative network", "evaluation graph", or empty for neither.
 */
[[nodiscard]] auto KinOf(const Object &object, const Drawing &drawing) -> std::string_view;
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_UNTYPED_HPP */
