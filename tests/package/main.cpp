/***********************************************************************************************************************
 *  @file      tests/package/main.cpp
 *  @brief     Loads and saves a drawing through the installed package, to prove it links.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/dwgio.hpp"

#include <cstdio>

auto main(int count, char **paths) -> int
{
  if (count != 2)
  {
    return 2;
  }
  const auto loaded = DwgIO::LoadFile(paths[1]);
  if (!loaded)
  {
    std::puts(loaded.error().What.c_str());
    return 1;
  }
  const auto saved = DwgIO::Save(loaded->Drawing);
  if (!saved)
  {
    return 1;
  }
  std::printf("dwgio %s: %zu objects, %zu bytes\n", DWGIO_VERSION_STRING, loaded->Drawing.Objects.size(),
              saved->Bytes.size());
  return 0;
}
