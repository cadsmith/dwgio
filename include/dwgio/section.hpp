/***********************************************************************************************************************
 *  @file      dwgio/section.hpp
 *  @brief     A section of the file kept as bytes because nothing types it.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_SECTION_HPP
#define DWGIO_SECTION_HPP

#include "dwgio/api.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace DwgIO
{
/**
 *  A section of the file kept as it was read, because nothing types it: the auxiliary header, the pad before the
 *  objects, the second header, the measurement, or bytes no section claimed.
 */
struct DWGIO_API Section
{
  std::string Name{}; /**< auxiliary, pad, second, measurement or unknown. */
  std::vector<std::byte> Bytes{};

  auto operator==(const Section &) const -> bool = default;
};
} // namespace DwgIO

#endif /* DWGIO_SECTION_HPP */
