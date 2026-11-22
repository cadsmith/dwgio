/***********************************************************************************************************************
 *  @file      source/classes/layout.hpp
 *  @brief     The classes section of chapter 10.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_CLASSES_LAYOUT_HPP
#define DWGIO_CLASSES_LAYOUT_HPP

#include "fields/codes.hpp"
#include "fields/context.hpp"
#include "fields/layout.hpp"
#include "fields/since.hpp"

#include "dwgio/class.hpp"
#include "dwgio/version.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace DwgIO::Classes
{
/**
 *  The bytes a paged file keeps after the classes' end sentinel, from R2004: eight, zero.
 */
constexpr std::size_t Trailer = 8;

/**
 *  One class as chapter 10 lays it out: R13 to R2000 items, then those R2004 added. The maintenance version is a BL,
 *  not the BS the text says.
 */
inline auto Layout()
{
  using namespace Fields;
  return Fields::Layout(Short(&Class::Number), Short(&Class::ProxyFlags), Text(&Class::Application),
                        Text(&Class::CppName), Text(&Class::DxfName), Bit(&Class::WasZombie), Short(&Class::ItemClass),
                        Since(Version::R2004, Long(&Class::Instances), Short(&Class::DwgVersion),
                              Long(&Class::Maintenance), Long(&Class::Unknown1), Long(&Class::Unknown2)));
}

/**
 *  Reads the classes of a section body, one after another while a byte or more remains; a class cut short is noted and
 *  left out.
 */
[[nodiscard]] auto Read(std::span<const std::byte> body, const Fields::Context &with) -> std::vector<Class>;

/**
 *  Writes the classes one after another, the last byte filled with one bits as AutoCAD fills it.
 */
[[nodiscard]] auto Write(std::span<const Class> classes, const Fields::Context &with) -> std::vector<std::byte>;
} // namespace DwgIO::Classes

#endif /* DWGIO_CLASSES_LAYOUT_HPP */
