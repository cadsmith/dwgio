/***********************************************************************************************************************
 *  @file      source/objects/storage.cpp
 *  @brief     The class a release stores an object under.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "objects/storage.hpp"

#include "objects/kinds.hpp"

#include "dwgio/object.hpp"
#include "dwgio/unknown.hpp"
#include "dwgio/version.hpp"

#include <string_view>
#include <type_traits>
#include <variant>

namespace DwgIO::Objects
{
auto ClassOf(const Object &object, Version release) -> std::string_view
{
  return Visit(
      [release](const auto &kind) -> std::string_view
      {
        using KindT = std::remove_cvref_t<decltype(kind)>;
        if constexpr (std::is_same_v<KindT, Unknown>)
        {
          return kind.Type >= FirstClass ? kind.Class.View() : std::string_view{};
        }
        else if constexpr (requires { Shape<KindT>::ClassAt(release); })
        {
          return Shape<KindT>::ClassAt(release);
        }
        else if constexpr (requires { Shape<KindT>::Fixed; })
        {
          return release < Shape<KindT>::Fixed ? NameOf(kind) : std::string_view{};
        }
        else if constexpr (Shape<KindT>::Types.empty())
        {
          return NameOf(kind);
        }
        else if constexpr (requires { Shape<KindT>::IsClassed(kind); })
        {
          return Shape<KindT>::IsClassed(kind) ? NameOf(kind) : std::string_view{};
        }
        else
        {
          return {};
        }
      },
      object);
}

auto ClassOf(const Object &object) -> std::string_view
{
  // The oldest release stores every classed kind under its class.
  return ClassOf(object, Version::R14);
}
} // namespace DwgIO::Objects
