/***********************************************************************************************************************
 *  @file      tests/dwgio/support/pass.hpp
 *  @brief     One pass of a save run alone on a drawing, as the save runs it.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TESTS_SUPPORT_PASS_HPP
#define DWGIO_TESTS_SUPPORT_PASS_HPP

#include "releases/remake.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/version.hpp"

#include <optional>
#include <utility>

namespace DwgIO::Releases
{
/**
 *  The drawing the pass makes, or nothing when it changes nothing.
 */
inline auto Pass(void (*pass)(Remake &, Version), const Drawing &drawing, Version to) -> std::optional<Drawing>
{
  Remake remade{drawing};
  pass(remade, to);
  return std::move(remade).Take();
}
} // namespace DwgIO::Releases

#endif /* DWGIO_TESTS_SUPPORT_PASS_HPP */
