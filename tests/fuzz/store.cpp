/***********************************************************************************************************************
 *  @file      tests/fuzz/store.cpp
 *  @brief     Fuzzes reading and writing the R2013 data store.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/store.hpp"

#include "store/read.hpp"
#include "store/write.hpp"

#include "dwgio/issue.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

extern "C" auto LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size) -> int
{
  const std::span<const std::byte> store{reinterpret_cast<const std::byte *>(data), size};
  const auto entries = DwgIO::Store::Read(store);
  if (entries)
  {
    const auto written = DwgIO::Store::Write(*entries);
    static_cast<void>(DwgIO::Store::Read(written));
  }
  std::vector<DwgIO::Issue> issues;
  const auto restored = DwgIO::Releases::LowerStore(store, issues);
  if (restored.size() != size)
  {
    __builtin_trap();
  }
  return 0;
}
