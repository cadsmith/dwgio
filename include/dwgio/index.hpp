/***********************************************************************************************************************
 *  @file      dwgio/index.hpp
 *  @brief     A drawing's objects found by handle in constant time.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_INDEX_HPP
#define DWGIO_INDEX_HPP

#include "dwgio/api.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tables/control.hpp"
#include "dwgio/tables/table.hpp"

#include <array>
#include <cstdint>
#include <unordered_map>

namespace DwgIO
{
/**
 *  A drawing's objects by handle, and its table controls by table, read once so that each lookup takes constant time:
 *  what walking a large drawing through `View` needs, where `Find` on the drawing scans it.
 *
 *  It holds pointers into `Drawing::Objects`, good until the vector changes; the drawing must outlive it.
 */
class DWGIO_API Index
{
public:
  explicit Index(const Drawing &drawing);

  [[nodiscard]] auto Source() const -> const Drawing &;

  /**
   *  @return Null when no object has that handle, and for the null handle; the first in file order when several do.
   */
  [[nodiscard]] auto Find(Handle self) const -> const Object *;

  /**
   *  @return The first control of that table in file order; null when there is none.
   */
  [[nodiscard]] auto Control(Tables::Table table) const -> const Tables::Control *;

private:
  const Drawing *m_Drawing;
  std::unordered_map<std::uint64_t, const Object *> m_Objects;
  std::array<const Tables::Control *, Tables::Count> m_Controls{};
};

/**
 *  @return Null when no object has that handle, and for the null handle.
 */
[[nodiscard]] DWGIO_API auto Find(const Index &index, Handle self) -> const Object *;
} // namespace DwgIO

#endif /* DWGIO_INDEX_HPP */
