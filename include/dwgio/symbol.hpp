/***********************************************************************************************************************
 *  @file      dwgio/symbol.hpp
 *  @brief     Interned names, shared by every record that bears them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_SYMBOL_HPP
#define DWGIO_SYMBOL_HPP

#include "dwgio/api.hpp"

#include <cstddef>
#include <format>
#include <functional>
#include <string>
#include <string_view>

namespace DwgIO
{
/**
 *  A name records repeat, such as a layer, linetype, text style, block or class, stored once for the whole program.
 *
 *  Symbols of the same text share it, so a copy costs a pointer and comparing two compares addresses.
 */
class DWGIO_API Symbol
{
public:
  Symbol();
  /**
   *  Case kept.
   *
   *  A text not seen before joins a store every thread shares, for the life of the program.
   */
  explicit Symbol(std::string_view text);
  explicit Symbol(const std::string &text);
  /**
   *  @pre The text is not null.
   */
  explicit Symbol(const char *text);

  /**
   *  Valid for as long as the program runs.
   */
  [[nodiscard]] auto View() const noexcept -> std::string_view;
  /**
   *  Valid for as long as the program runs.
   */
  [[nodiscard]] auto Str() const noexcept -> const std::string &;
  [[nodiscard]] auto IsEmpty() const noexcept -> bool;

  auto operator==(const Symbol &) const -> bool = default;

  /**
   *  Case included.
   */
  auto operator==(std::string_view text) const -> bool;

private:
  const std::string *m_Text;
};
} // namespace DwgIO

template <>
struct std::hash<DwgIO::Symbol>
{
  auto operator()(const DwgIO::Symbol &one) const noexcept -> std::size_t
  {
    return std::hash<const void *>{}(&one.Str());
  }
};

template <>
struct std::formatter<DwgIO::Symbol> : std::formatter<std::string_view>
{
  template <class ContextT>
  auto format(const DwgIO::Symbol &one, ContextT &context) const
  {
    return std::formatter<std::string_view>::format(one.View(), context);
  }
};

#endif /* DWGIO_SYMBOL_HPP */
