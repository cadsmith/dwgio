/***********************************************************************************************************************
 *  @file      dwgio/objects/placeholder.hpp
 *  @brief     The ACDBPLACEHOLDER object: a dictionary entry with no object of its own.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_PLACEHOLDER_HPP
#define DWGIO_OBJECTS_PLACEHOLDER_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"

namespace DwgIO::Objects
{
/**
 *  An ACDBPLACEHOLDER: stands for a dictionary entry that has no object of its own, such as the Normal plot style.
 */
struct DWGIO_API Placeholder
{
  DwgIO::Base Base{};

  auto operator==(const Placeholder &) const -> bool = default;
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_PLACEHOLDER_HPP */
