/***********************************************************************************************************************
 *  @file      dwgio/audit.hpp
 *  @brief     What AutoCAD's AUDIT would find in a drawing, and its repair in place.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_AUDIT_HPP
#define DWGIO_AUDIT_HPP

#include "dwgio/api.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace DwgIO::Audit
{
enum class Severity : std::uint8_t
{
  Warning, /**< AutoCAD opens the drawing as it is.          */
  Error,   /**< AutoCAD recovers the drawing, or refuses it. */
};

struct DWGIO_API Finding
{
  Severity Level{};
  std::string What{}; /**< Such as "layer 1001 missing" or "handle 2B of a CIRCLE used twice". */
  /**
   *  The object at fault, or the one that needed what is missing; null when there is none, as for the header.
   */
  Handle Subject{};
  bool Fixed{}; /**< Set by Repair; Check leaves it false. */

  auto operator==(const Finding &) const -> bool = default;
};

struct DWGIO_API Report
{
  std::vector<Finding> Findings{}; /**< In the order the checks ran: handles, the seed, then the rest. */

  /**
   *  No error is left unfixed: AutoCAD would open the drawing without recovering it.
   */
  [[nodiscard]] auto IsClean() const noexcept -> bool;
  /**
   *  The findings that are errors, fixed or not.
   */
  [[nodiscard]] auto Errors() const noexcept -> std::size_t;
  /**
   *  The findings that are warnings, fixed or not.
   */
  [[nodiscard]] auto Warnings() const noexcept -> std::size_t;

  auto operator==(const Report &) const -> bool = default;
};

/**
 *  Lists what AutoCAD would complain about, leaving the drawing as it is.
 */
[[nodiscard]] DWGIO_API auto Check(const Drawing &drawing) -> Report;

/**
 *  Runs Check's checks and fixes what they find, in place: an object with no handle, or with another's, takes a new one
 *  past every handle in use; the handle seed moves past the highest handle.
 *
 *  An object it leaves out is erased from the drawing's objects, so a pointer the view gave before is not to be trusted
 *  after.
 */
DWGIO_API auto Repair(Drawing &drawing) -> Report;
} // namespace DwgIO::Audit

#endif /* DWGIO_AUDIT_HPP */
