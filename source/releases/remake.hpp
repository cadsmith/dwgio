/***********************************************************************************************************************
 *  @file      source/releases/remake.hpp
 *  @brief     The drawing a save makes over pass after pass, copied once.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_REMAKE_HPP
#define DWGIO_RELEASES_REMAKE_HPP

#include "dwgio/drawing.hpp"

#include <optional>

namespace DwgIO::Releases
{
/**
 *  A drawing a save makes over, pass after pass: the caller's as long as no pass changes it, then one copy, which every
 *  later pass changes in place. A large drawing is held at most twice, the caller's and the copy.
 */
class Remake
{
public:
  explicit Remake(const Drawing &drawing) : m_Given{&drawing} {}

  [[nodiscard]] auto Now() const -> const Drawing & { return m_Made ? *m_Made : *m_Given; }

  /**
   *  The drawing to change: the copy, made on the first call with room for the few objects the passes add, so that
   *  adding one does not double what the objects take.
   */
  [[nodiscard]] auto Change() -> Drawing &;
  /**
   *  The copy, or nothing when no pass changed the drawing.
   */
  [[nodiscard]] auto Take() && -> std::optional<Drawing>;

private:
  const Drawing *m_Given;
  std::optional<Drawing> m_Made;
};
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_REMAKE_HPP */
