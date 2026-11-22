/***********************************************************************************************************************
 *  @file      tests/dwgio/objects/types.cpp
 *  @brief     The fixed type codes of chapter 20.3.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "objects/types.hpp"

#include <doctest/doctest.h>

namespace DwgIO::Objects
{
TEST_CASE("objects/types: the fixed type codes of chapter 20.3 map to their names")
{
  CHECK(TypeNameOf(0x13) == "LINE");
  CHECK(TypeNameOf(0x01) == "TEXT");
  CHECK(TypeNameOf(0x2A) == "DICTIONARY");
  CHECK(TypeNameOf(0x31) == "BLOCK_HEADER");
  CHECK(TypeNameOf(0x43) == "APPID");
  CHECK(TypeNameOf(0x4D) == "LWPOLYLINE");
  CHECK(TypeNameOf(0x52) == "LAYOUT");
  CHECK(TypeNameOf(0x1F2) == "ACAD_PROXY_ENTITY");
  CHECK(TypeNameOf(0x1F3) == "ACAD_PROXY_OBJECT");
  CHECK(TypeNameOf(0).empty());
  CHECK(TypeNameOf(9).empty());
  CHECK(TypeNameOf(0x36).empty());
  CHECK(TypeNameOf(0x53).empty());
  CHECK(TypeNameOf(500).empty());
}

TEST_CASE("objects/types: the fixed types that are entities are known")
{
  CHECK(IsEntity(0x13));
  CHECK(IsEntity(0x04));
  CHECK(IsEntity(0x05));
  CHECK(IsEntity(0x2C));
  CHECK(IsEntity(0x4A));
  CHECK(IsEntity(0x4E));
  CHECK(IsEntity(0x1F2));
  CHECK_FALSE(IsEntity(0x2A));
  CHECK_FALSE(IsEntity(0x30));
  CHECK_FALSE(IsEntity(0x4F));
  CHECK_FALSE(IsEntity(0x1F3));
  CHECK_FALSE(IsEntity(0));
  CHECK_FALSE(IsEntity(500));
}
} // namespace DwgIO::Objects
