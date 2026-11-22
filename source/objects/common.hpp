/***********************************************************************************************************************
 *  @file      source/objects/common.hpp
 *  @brief     The common entity data and handles written, chapters 20.4.1 and 20.4.2.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_COMMON_HPP
#define DWGIO_OBJECTS_COMMON_HPP

#include "fields/context.hpp"

#include "dwgio/base.hpp"
#include "dwgio/common.hpp"

namespace DwgIO::Objects
{
/**
 *  Writes chapter 20.4.1 from the space code on and chapter 20.4.2's handles.
 *
 *  The owner and the links are written as the relative references AutoCAD writes, the links left out when they are the
 *  handles next to the entity's own; an R14 lineweight of -1 is written as ByLayer. The caller writes the proxy
 *  graphics and, before R2000, the bit size first.
 */
void WriteCommon(const Common &common, const Base &base, const Fields::Output &to);
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_COMMON_HPP */
