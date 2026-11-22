/***********************************************************************************************************************
 *  @file      source/bits/writer.hpp
 *  @brief     The writer of the bit codes of chapter 2.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_BITS_WRITER_HPP
#define DWGIO_BITS_WRITER_HPP

#include "bits/reference.hpp"

#include "dwgio/color.hpp"
#include "dwgio/vector.hpp"
#include "dwgio/version.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace DwgIO::Encoding
{
struct Scheme;
} // namespace DwgIO::Encoding

namespace DwgIO::Bits
{
/**
 *  Writes the bit codes of chapter 2, each in the fewest bits that read back to the value, so that what the reader read
 *  from a file this writes back as it was.
 */
class Writer
{
public:
  /**
   *  In bits.
   */
  [[nodiscard]] auto Position() const -> std::size_t;
  /**
   *  What was written so far, the last byte padded with zero bits.
   */
  [[nodiscard]] auto Bytes() const -> std::span<const std::byte>;
  /**
   *  Takes what was written, leaving the writer empty.
   */
  auto Take() -> std::vector<std::byte>;

  auto B(bool bit) -> void;
  /**
   *  BB: two bits, the first the more significant.
   */
  auto BB(int value) -> void;
  /**
   *  3B: 0, 2, 6 or 7, what the triplet can say; anything else is written as the next of those above it.
   */
  auto B3(int value) -> void;
  /**
   *  That many low bits of the value, the most significant first.
   */
  auto Bits(std::uint64_t value, int count) -> void;
  auto RC(std::uint8_t value) -> void;
  auto RS(std::uint16_t value) -> void;
  auto RL(std::uint32_t value) -> void;
  auto RLL(std::uint64_t value) -> void;
  auto RD(double value) -> void;
  /**
   *  Moves to the next byte boundary, or stays on one.
   */
  auto Pad() -> void;
  /**
   *  Fills up to the next byte boundary with one bits, as AutoCAD pads an object and the classes.
   */
  auto Fill() -> void;
  /**
   *  Those bytes from the position, whatever its alignment.
   */
  auto Raw(std::span<const std::byte> bytes) -> void;
  /**
   *  Another writer's bits from the position, whatever the alignment of either.
   */
  auto Append(const Writer &tail) -> void;

  /**
   *  BS: 0 as 10, 256 as 11, 1 to 255 as 01 and a byte, anything else as 00 and a short.
   */
  auto BS(int value) -> void;
  /**
   *  BL: 0 as 10, 1 to 255 as 01 and a byte, anything else as 00 and a long.
   */
  auto BL(std::int32_t value) -> void;
  /**
   *  BLL: the fewest bytes that hold the value, at most seven, counted in three bits, then those bytes.
   */
  auto BLL(std::int64_t value) -> void;
  /**
   *  BD: 0.0 as 10, 1.0 as 01, anything else as 00 and a double; a negative zero is anything else.
   */
  auto BD(double value) -> void;
  /**
   *  MC: the fewest bytes of seven bits, 0x40 of the last the sign.
   */
  auto MC(std::int64_t value) -> void;
  /**
   *  UMC: the fewest bytes of seven bits, no sign.
   */
  auto UMC(std::uint64_t value) -> void;
  /**
   *  MS: the fewest shorts of fifteen bits.
   */
  auto MS(std::uint64_t value) -> void;
  /**
   *  H: the code, the fewest bytes the handle needs, then those bytes most significant first.
   */
  auto H(Reference reference) -> void;
  /**
   *  DD: the shortest of nothing, four bytes, six bytes or the whole double that turns the other into the value.
   */
  auto DD(double value, double other) -> void;
  /**
   *  BT: from R2000 one bit for 0.0, else a zero bit and a BD; a BD before.
   */
  auto BT(double value, Version release) -> void;
  /**
   *  BE: from R2000 one bit for (0, 0, 1), else a zero bit and three BD; three BD before.
   */
  auto BE(const Vector &value, Version release) -> void;
  /**
   *  2BD.
   */
  auto Point2(const Vector &value) -> void;
  /**
   *  3BD.
   */
  auto Point3(const Vector &value) -> void;
  /**
   *  CMC: the index as BS; from R2004 the true color as BL, a byte flagging the names, and each name as a T, a TU from
   *  R2007.
   */
  auto CMC(const Color &value, Version release) -> void;
  /**
   *  CMC with its names written to another writer: the string stream of R2007 and later.
   */
  auto CMC(const Color &value, Version release, Writer &names) -> void;
  /**
   *  ENC: the index as BS; from R2004 with 0x8000 and a BL for a true color, 0x2000 and a BL for a transparency.
   */
  auto ENC(const Color &value, Version release) -> void;
  /**
   *  OT: a BS before R2010; from R2010 a byte, a byte above 0x1F0, or a short, each after its two bits.
   */
  auto OT(int value, Version release) -> void;
  /**
   *  T: the text through the scheme and, unless empty, the NUL AutoCAD ends a text with, the byte count as BS first; a
   *  text the BS cannot count is cut to 32766 bytes, which is then returned.
   */
  auto T(const std::string &text, const Encoding::Scheme &with) -> std::optional<std::size_t>;
  /**
   *  TU: the text as UTF-16 code units, little-endian, their count as BS first; a text the BS cannot count is cut to
   *  32767 units, which is then returned.
   */
  auto TU(const std::string &text) -> std::optional<std::size_t>;
  /**
   *  TV: a TU from R2007, a T before.
   */
  auto TV(const std::string &text, Version release, const Encoding::Scheme &with) -> std::optional<std::size_t>;

private:
  auto Put(bool bit) -> void;
  auto Modular(std::uint64_t magnitude, std::uint64_t last, std::uint8_t sign) -> void;
  auto Text(const std::string &text) -> void;

  std::vector<std::byte> m_Bytes;
  std::size_t m_Position{};
};
} // namespace DwgIO::Bits

#endif /* DWGIO_BITS_WRITER_HPP */
