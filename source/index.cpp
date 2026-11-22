/***********************************************************************************************************************
 *  @file      source/index.cpp
 *  @brief     A drawing's objects found by handle in constant time.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/index.hpp"

#include <cstddef>
#include <utility>

namespace DwgIO
{
Index::Index(const Drawing &drawing) : m_Drawing{&drawing}
{
  m_Objects.reserve(drawing.Objects.size());
  for (const Object &object : drawing.Objects)
  {
    m_Objects.try_emplace(BaseOf(object).Self.Value, &object);
    if (const auto *control = GetIf<Tables::Control>(&object))
    {
      const auto at = static_cast<std::size_t>(std::to_underlying(control->Table));
      if (at < m_Controls.size() && m_Controls.at(at) == nullptr)
      {
        m_Controls.at(at) = control;
      }
    }
  }
}

auto Index::Source() const -> const Drawing &
{
  return *m_Drawing;
}

auto Index::Find(Handle self) const -> const Object *
{
  if (self == Handle{})
  {
    return nullptr;
  }
  const auto found = m_Objects.find(self.Value);
  return found == m_Objects.end() ? nullptr : found->second;
}

auto Index::Control(Tables::Table table) const -> const Tables::Control *
{
  const auto at = static_cast<std::size_t>(std::to_underlying(table));
  return at < m_Controls.size() ? m_Controls.at(at) : nullptr;
}

auto Find(const Index &index, Handle self) -> const Object *
{
  return index.Find(self);
}
} // namespace DwgIO
