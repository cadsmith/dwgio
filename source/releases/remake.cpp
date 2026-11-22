/***********************************************************************************************************************
 *  @file      source/releases/remake.cpp
 *  @brief     The drawing a save makes over pass after pass, copied once.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/remake.hpp"

#include <cstddef>
#include <utility>

namespace DwgIO::Releases
{
namespace
{
/**
 *  The room a copy leaves for objects the passes add: a sixteenth more, and some for a small drawing.
 */
constexpr std::size_t Share = 16;
constexpr std::size_t Least = 64;
} // namespace

auto Remake::Change() -> Drawing &
{
  if (!m_Made)
  {
    // The room is made first, and the copy fills it: a vector assigned what fits keeps its room, where reserving
    // after the copy would hold the objects twice for a moment.
    const std::size_t size = m_Given->Objects.size();
    m_Made.emplace();
    m_Made->Objects.reserve(size + (size / Share) + Least);
    *m_Made = *m_Given;
  }
  return *m_Made;
}

auto Remake::Take() && -> std::optional<Drawing>
{
  return std::move(m_Made);
}
} // namespace DwgIO::Releases
