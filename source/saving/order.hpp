/***********************************************************************************************************************
 *  @file      source/saving/order.hpp
 *  @brief     The order a save writes a file's sections in.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_SAVING_ORDER_HPP
#define DWGIO_SAVING_ORDER_HPP

#include "dwgio/drawing.hpp"
#include "dwgio/issue.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace DwgIO::Saving
{
/**
 *  Where a name stands in an order, or its end.
 */
[[nodiscard]] auto FindName(std::vector<std::string> &order, std::string_view name)
    -> std::vector<std::string>::iterator;

/**
 *  The order a paged file's sections are written in: the drawing's, with what the locator container alone has left out,
 *  or AutoCAD's usual order; the template is made when the drawing has none.
 *
 *  An order that misses one of the blocks every file has gives way to AutoCAD's usual order.
 */
[[nodiscard]] auto PagedOrder(const Drawing &drawing, std::vector<Issue> &issues) -> std::vector<std::string>;

/**
 *  The order a locator file's blocks are written in: the drawing's, or AutoCAD's usual order for a new drawing or one
 *  from a paged file, whose sections the text has no place for.
 */
[[nodiscard]] auto LocatorOrder(const Drawing &drawing, std::vector<Issue> &issues) -> std::vector<std::string>;
} // namespace DwgIO::Saving

#endif /* DWGIO_SAVING_ORDER_HPP */
