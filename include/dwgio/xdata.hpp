/***********************************************************************************************************************
 *  @file      dwgio/xdata.hpp
 *  @brief     The extended data one application attaches to a record.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_XDATA_HPP
#define DWGIO_XDATA_HPP

#include "dwgio/api.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/tag.hpp"

#include <string>
#include <vector>

namespace DwgIO
{
/**
 *  The extended data one application attaches to a record: its groups 1000 to 1071.
 */
struct DWGIO_API Xdata
{
  /**
   *  As the APPID table registers it, such as "ACAD"; empty until that record is read.
   */
  std::string Application{};
  Handle AppId{}; /**< The APPID record, which is what a DWG stores. */
  std::vector<Tag> Tags{};

  auto operator==(const Xdata &) const -> bool = default;
};
} // namespace DwgIO

#endif /* DWGIO_XDATA_HPP */
