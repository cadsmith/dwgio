/***********************************************************************************************************************
 *  @file      source/object.cpp
 *  @brief     The type name, base and common entity data of any kind of object.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/object.hpp"

#include "objects/kinds.hpp"
#include "objects/types.hpp"

#include <type_traits>
#include <variant>

namespace DwgIO
{
auto TypeOf(const Object &object) -> std::string_view
{
  return Visit(
      [](const auto &kind) -> std::string_view
      {
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(kind)>, Unknown>)
        {
          return kind.Class.IsEmpty() ? Objects::TypeNameOf(kind.Type) : kind.Class.View();
        }
        else
        {
          return Objects::NameOf(kind);
        }
      },
      object);
}

auto BaseOf(const Object &object) -> const Base &
{
  return Visit([](const auto &kind) -> const Base & { return Objects::BaseOfKind(kind); }, object);
}

auto BaseOf(Object &object) -> Base &
{
  return Visit([](auto &kind) -> Base & { return Objects::BaseOfKind(kind); }, object);
}

namespace
{
/**
 *  Null for a kind that is no entity.
 */
auto CommonOfKind(auto &kind) -> decltype(&kind.Common)
{
  if constexpr (std::is_same_v<std::remove_cvref_t<decltype(kind)>, Unknown>)
  {
    return kind.Entity ? &kind.Common : nullptr;
  }
  else
  {
    return &kind.Common;
  }
}
} // namespace

auto CommonOf(const Object &object) -> const Common *
{
  return Visit(
      [](const auto &kind) -> const Common *
      {
        if constexpr (requires { kind.Common; })
        {
          return CommonOfKind(kind);
        }
        else
        {
          return nullptr;
        }
      },
      object);
}

auto CommonOf(Object &object) -> Common *
{
  return Visit(
      [](auto &kind) -> Common *
      {
        if constexpr (requires { kind.Common; })
        {
          return CommonOfKind(kind);
        }
        else
        {
          return nullptr;
        }
      },
      object);
}
} // namespace DwgIO
