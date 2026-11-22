/***********************************************************************************************************************
 *  @file      tests/dwgio/tables/older.cpp
 *  @brief     Every typed object of the R14 fixtures written back through its R14 layout.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "fields/context.hpp"
#include "fields/layout.hpp"
#include "objects/kinds.hpp"
#include "support/fixtures.hpp"
#include "support/reader.hpp"

#include "dwgio/load.hpp"
#include "dwgio/object.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace DwgIO::Tables
{
TEST_CASE("tables/older: every typed object of the R14 fixtures writes back through its R14 layout unchanged")
{
  for (const auto &[name, expected] :
       {std::pair{std::string_view{"tables-r14.dwg"}, 357UZ}, std::pair{std::string_view{"entities-r14.dwg"}, 351UZ},
        std::pair{std::string_view{"curves-r14.dwg"}, 629UZ}, std::pair{std::string_view{"objects-r14.dwg"}, 350UZ},
        std::pair{std::string_view{"empty-r14.dwg"}, 299UZ}})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    std::vector<Issue> issues;
    const Fields::Context with{
        .Release = Version::R14, .Text = &Objects::Latin, .Issues = &issues, .Classes = &loaded->Drawing.Classes};
    std::size_t typed = 0;
    for (const Object &object : loaded->Drawing.Objects)
    {
      CAPTURE(TypeOf(object));
      Visit(
          [&](const auto &kind) -> void
          {
            using KindT = std::remove_cvref_t<decltype(kind)>;
            if constexpr (!std::is_same_v<KindT, Unknown>)
            {
              ++typed;
              Bits::Writer data;
              Bits::Writer handles;
              Fields::Output to{.To = &data, .Handles = &handles, .With = with};
              REQUIRE(Fields::Write(Objects::Shape<KindT>::Layout(), kind, to));
              Bits::Reader dataIn{data.Bytes()};
              Bits::Reader handlesIn{handles.Bytes()};
              Fields::Input in{.In = &dataIn, .Handles = &handlesIn, .With = with};
              KindT back{};
              Objects::BaseOfKind(back) = Objects::BaseOfKind(kind); // As the reader sets it, before the layout.
              if constexpr (requires { back.Table; })
              {
                back.Table = kind.Table;
              }
              if constexpr (requires { Objects::Shape<KindT>::Open(back, std::uint16_t{}); })
              {
                Objects::Shape<KindT>::Open(back, Objects::CodeOf(kind, with));
              }
              if constexpr (requires { Objects::Shape<KindT>::Open(back, std::string_view{}); })
              {
                Objects::Shape<KindT>::Open(back, Objects::NameOf(kind));
              }
              REQUIRE(Fields::Read(Objects::Shape<KindT>::Layout(), back, in));
              if constexpr (requires { back.Common; })
              {
                back.Common = kind.Common;
              }
              CHECK(back == kind);
            }
          },
          object);
    }
    CHECK(typed == expected);
    CHECK(issues.empty());
  }
}
} // namespace DwgIO::Tables
