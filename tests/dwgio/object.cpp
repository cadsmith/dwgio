/***********************************************************************************************************************
 *  @file      tests/dwgio/object.cpp
 *  @brief     The accessors of an object, through the box that holds its kind.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/object.hpp"

#include "dwgio/boxed.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/unknown.hpp"

#include <doctest/doctest.h>

#include <type_traits>
#include <variant>

namespace DwgIO
{
TEST_CASE("object: the accessors reach the kind through its box")
{
  Object object{Entities::Line{.Thickness = 2}};
  const Object &read = object;
  CHECK(Holds<Entities::Line>(object));
  CHECK_FALSE(Holds<Tables::Layer>(object));
  CHECK(Get<Entities::Line>(object).Thickness == 2);
  CHECK(Get<Entities::Line>(read).Thickness == 2);
  CHECK_THROWS_AS((void)Get<Tables::Layer>(object), std::bad_variant_access);
  CHECK_THROWS_AS((void)Get<Tables::Layer>(read), std::bad_variant_access);

  CHECK(GetIf<Entities::Line>(&object) == &Get<Entities::Line>(object));
  CHECK(GetIf<Entities::Line>(&read) == &Get<Entities::Line>(read));
  CHECK(GetIf<Tables::Layer>(&object) == nullptr);
  CHECK(GetIf<Tables::Layer>(&read) == nullptr);
  CHECK(GetIf<Entities::Line>(static_cast<Object *>(nullptr)) == nullptr);
  CHECK(GetIf<Entities::Line>(static_cast<const Object *>(nullptr)) == nullptr);

  Visit(
      [](auto &kind) -> void
      {
        if constexpr (std::is_same_v<decltype(kind), Entities::Line &>)
        {
          kind.Thickness = 7;
        }
      },
      object);
  CHECK(Get<Entities::Line>(read).Thickness == 7);
  CHECK(Visit([](const auto &kind) -> bool { return std::is_same_v<decltype(kind), const Entities::Line &>; }, read));

  const Object copy = object;
  CHECK(copy == object);
  CHECK(Holds<Unknown>(Object{}));
}
} // namespace DwgIO
