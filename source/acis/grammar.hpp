/***********************************************************************************************************************
 *  @file      source/acis/grammar.hpp
 *  @brief     The grammar of each ACIS record AutoCAD's files hold.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ACIS_GRAMMAR_HPP
#define DWGIO_ACIS_GRAMMAR_HPP

#include "acis/visitor.hpp"

#include <string_view>

namespace DwgIO::Acis
{
/**
 *  The values every record has before its own: the pointer to its attributes, from ACIS 20800 a history long and, for
 *  an entity but a transform and the header record, a pointer after it.
 */
namespace Prefix
{
/**
 *  How many values an entity's prefix has.
 */
constexpr int Entity = 3;
} // namespace Prefix

/**
 *  Walks the fields of a record of that name; false for a record AutoCAD's files never hold, whose fields in each
 *  release are unknown.
 */
[[nodiscard]] auto Walk(Visitor &visitor, std::string_view name) -> bool;
} // namespace DwgIO::Acis

#endif /* DWGIO_ACIS_GRAMMAR_HPP */
