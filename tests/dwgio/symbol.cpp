/***********************************************************************************************************************
 *  @file      tests/dwgio/symbol.cpp
 *  @brief     Interned names: shared, empty, from many threads.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/symbol.hpp"

#include <doctest/doctest.h>

#include <format>
#include <functional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace DwgIO
{
TEST_CASE("symbol: equal names share one interned text")
{
  const Symbol walls{"walls"};
  const Symbol again{std::string{"walls"}};
  CHECK(walls == again);
  CHECK(&walls.Str() == &again.Str());
  CHECK(std::hash<Symbol>{}(walls) == std::hash<Symbol>{}(again));
  CHECK_FALSE(walls == Symbol{"Walls"});
}

TEST_CASE("symbol: an empty name reads back as empty text")
{
  CHECK(Symbol{}.IsEmpty());
  CHECK(Symbol{""} == Symbol{});
  CHECK(Symbol{"0"}.View() == "0");
  CHECK(Symbol{"walls"} == std::string_view{"walls"});
  CHECK_FALSE(Symbol{"walls"} == std::string_view{"Walls"});
  CHECK(std::format("{}", Symbol{"ByLayer"}) == "ByLayer");
}

TEST_CASE("symbol: names interned from many threads are one")
{
  std::vector<const std::string *> seen(8);
  {
    std::vector<std::jthread> threads;
    threads.reserve(seen.size());
    for (const std::string *&slot : seen)
    {
      threads.emplace_back([&slot] -> void { slot = &Symbol{"threaded"}.Str(); });
    }
  }
  for (const std::string *one : seen)
  {
    CHECK(one == &Symbol{"threaded"}.Str());
  }
}

TEST_CASE("symbol: a name can be made from a string view")
{
  const std::string_view text = "walls";
  CHECK(Symbol{text} == Symbol{"walls"});
}
} // namespace DwgIO
