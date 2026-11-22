/***********************************************************************************************************************
 *  @file      dwgio/boxed.hpp
 *  @brief     A value on the heap that copies and compares as a value.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_BOXED_HPP
#define DWGIO_BOXED_HPP

#include <memory>
#include <utility>

namespace DwgIO
{
/**
 *  A value kept on the heap and handled as a value: a copy copies the value, a comparison compares it, and a move hands
 *  the value over without copying it. A box moved from holds nothing: it may only be destroyed or assigned.
 */
template <class ValueT>
class Boxed
{
public:
  using Type = ValueT;

  Boxed() : m_Value(std::make_unique<ValueT>()) {}

  explicit(false) Boxed(const ValueT &value) : m_Value(std::make_unique<ValueT>(value)) {}

  explicit(false) Boxed(ValueT &&value) : m_Value(std::make_unique<ValueT>(std::move(value))) {}

  Boxed(const Boxed &other) : m_Value(std::make_unique<ValueT>(*other.m_Value)) {}

  /**
   *  The other holds nothing after.
   */
  Boxed(Boxed &&other) noexcept = default;

  ~Boxed() = default;

  auto operator=(const Boxed &other) -> Boxed &
  {
    if (this != &other)
    {
      m_Value = std::make_unique<ValueT>(*other.m_Value);
    }
    return *this;
  }

  /**
   *  The other holds nothing after.
   */
  auto operator=(Boxed &&other) noexcept -> Boxed & = default;

  [[nodiscard]] auto operator*() -> ValueT & { return *m_Value; }

  [[nodiscard]] auto operator*() const -> const ValueT & { return *m_Value; }

  [[nodiscard]] auto operator->() -> ValueT * { return m_Value.get(); }

  [[nodiscard]] auto operator->() const -> const ValueT * { return m_Value.get(); }

  [[nodiscard]] auto operator==(const Boxed &other) const -> bool { return *m_Value == *other.m_Value; }

private:
  std::unique_ptr<ValueT> m_Value;
};
} // namespace DwgIO

#endif /* DWGIO_BOXED_HPP */
