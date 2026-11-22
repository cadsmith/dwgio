/***********************************************************************************************************************
 *  @file      source/acis/sab/tags.hpp
 *  @brief     The tags and markers of SAB, the binary ACIS.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ACIS_SAB_TAGS_HPP
#define DWGIO_ACIS_SAB_TAGS_HPP

#include <array>
#include <cstdint>
#include <string_view>

/**
 *  SAB, the binary ACIS AutoCAD writes from R2004: a head, then each value behind a byte that says what it is.
 */
namespace DwgIO::Acis::Sab
{
/**
 *  The bytes that say what follows.
 */
namespace Tags
{
/**
 *  Four bytes.
 */
constexpr std::uint8_t Long = 0x04;
/**
 *  Eight bytes.
 */
constexpr std::uint8_t Real = 0x06;
/**
 *  Text behind a byte of its length.
 */
constexpr std::uint8_t Text = 0x07;
/**
 *  Text behind two bytes of its length.
 */
constexpr std::uint8_t LongText = 0x08;
/**
 *  A logical true: a reversed sense, a double-sided face, a finite bound.
 */
constexpr std::uint8_t True = 0x0A;
/**
 *  A logical false: a forward sense, a single-sided face, an infinite bound.
 */
constexpr std::uint8_t False = 0x0B;
/**
 *  A pointer to a record, four bytes.
 */
constexpr std::uint8_t Pointer = 0x0C;
/**
 *  A word, or the last part of a record's name.
 */
constexpr std::uint8_t Word = 0x0D;
/**
 *  A part of a record's name a hyphen follows.
 */
constexpr std::uint8_t Part = 0x0E;
/**
 *  The brace that opens a subtype.
 */
constexpr std::uint8_t Open = 0x0F;
/**
 *  The brace that closes it.
 */
constexpr std::uint8_t Close = 0x10;
constexpr std::uint8_t End = 0x11;
/**
 *  Three doubles.
 */
constexpr std::uint8_t Position = 0x13;
/**
 *  Three doubles.
 */
constexpr std::uint8_t Direction = 0x14;
/**
 *  Four bytes.
 */
constexpr std::uint8_t Choice = 0x15;
/**
 *  Eight bytes.
 */
constexpr std::uint8_t Huge = 0x17;
} // namespace Tags

/**
 *  What opens a SAB body before ACIS 22300, and from it.
 */
constexpr std::string_view Magic = "ACIS BinaryFile";
constexpr std::string_view NewMagic = "ASM BinaryFile4";
/**
 *  The first ACIS release whose SAB opens with the newer magic.
 */
constexpr int NewMagicSince = 22300;
/**
 *  The record that ends a body before ACIS 21800, and from it, as its name's parts.
 */
constexpr std::array<std::string_view, 4> Ending{"End", "of", "ACIS", "data"};
constexpr std::array<std::string_view, 4> NewEnding{"End", "of", "ASM", "data"};
/**
 *  The first ACIS release ending with the newer record.
 */
constexpr int NewEndingSince = 21800;
} // namespace DwgIO::Acis::Sab

#endif /* DWGIO_ACIS_SAB_TAGS_HPP */
