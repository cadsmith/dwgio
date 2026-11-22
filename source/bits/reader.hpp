/***********************************************************************************************************************
 *  @file      source/bits/reader.hpp
 *  @brief     The reader of the bit codes of chapter 2.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_BITS_READER_HPP
#define DWGIO_BITS_READER_HPP

#include "bits/reference.hpp"

#include "dwgio/color.hpp"
#include "dwgio/vector.hpp"
#include "dwgio/version.hpp"

#include <cstddef>
#include <cstdint>
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
 *  Reads the bit codes of chapter 2 from a run of bytes, most significant bit first, numbers little-endian.
 *
 *  A read past the end fails the reader: it and every later read give zero, and the position stays at the end.
 */
class Reader
{
public:
  /**
   *  The bytes must outlive the reader.
   */
  explicit Reader(std::span<const std::byte> bytes);

  /**
   *  In bits.
   */
  [[nodiscard]] auto Position() const -> std::size_t;
  /**
   *  In bits.
   */
  [[nodiscard]] auto Size() const -> std::size_t;
  /**
   *  Moves to that bit; past the end fails and stays at the end.
   */
  auto Seek(std::size_t bit) -> void;
  [[nodiscard]] auto HasFailed() const -> bool;

  auto B() -> bool;
  /**
   *  BB: two bits, the first the more significant.
   */
  auto BB() -> int;
  /**
   *  3B: one to three bits, up to the first zero or the third, shifted in from the right: 0 to 7.
   */
  auto B3() -> int;
  /**
   *  Up to 64 bits, the first the most significant; none gives zero.
   */
  auto Bits(int count) -> std::uint64_t;
  auto RC() -> std::uint8_t;
  auto RS() -> std::uint16_t;
  auto RL() -> std::uint32_t;
  auto RLL() -> std::uint64_t;
  auto RD() -> double;
  /**
   *  Moves to the next byte boundary, or stays on one.
   */
  auto Pad() -> void;
  /**
   *  That many bytes from the position, whatever its alignment; none when fewer are left, and the reader fails.
   */
  auto Bytes(std::size_t count) -> std::vector<std::byte>;

  /**
   *  BS: two bits then a short, a byte, 0 or 256.
   */
  auto BS() -> int;
  /**
   *  BL: two bits then a long, a byte or 0.
   */
  auto BL() -> std::int32_t;
  /**
   *  BLL: three bits giving how many bytes follow, little-endian; the files hold three plain bits where chapter 2.4
   *  says a triplet.
   */
  auto BLL() -> std::int64_t;
  /**
   *  BD: two bits then a double, 1.0 or 0.0.
   */
  auto BD() -> double;
  /**
   *  MC: bytes of seven bits each, the high bit asking for another, 0x40 of the last negating.
   */
  auto MC() -> std::int64_t;
  /**
   *  UMC: a modular char with no sign bit, as the object map stores handle offsets.
   */
  auto UMC() -> std::uint64_t;
  /**
   *  MS: shorts of fifteen bits each, 0x8000 asking for another.
   */
  auto MS() -> std::uint64_t;
  /**
   *  H: a code, a count, then that many bytes of handle, most significant first; a count past eight fails.
   */
  auto H() -> Reference;
  /**
   *  DD: two bits then nothing, four bytes, six bytes or a whole double patched into the other.
   */
  auto DD(double other) -> double;
  /**
   *  BT: from R2000 one bit for a thickness of 0.0, else a BD; a BD before.
   */
  auto BT(Version release) -> double;
  /**
   *  BE: from R2000 one bit for the extrusion (0, 0, 1), else three BD; three BD before.
   */
  auto BE(Version release) -> Vector;
  /**
   *  2BD, z zero.
   */
  auto Point2() -> Vector;
  /**
   *  3BD.
   */
  auto Point3() -> Vector;
  /**
   *  CMC: a BS index; from R2004 a BL true color, a byte and the names it flags, each a T, a TU from R2007.
   */
  auto CMC(Version release) -> Color;
  /**
   *  CMC with its names read from another reader: the string stream of R2007 and later.
   */
  auto CMC(Version release, Reader &names) -> Color;
  /**
   *  ENC: a BS index; from R2004 its top bits flag a BL true color (0x8000) and a BL transparency (0x2000).
   */
  auto ENC(Version release) -> Color;
  /**
   *  OT: a BS before R2010; from R2010 two bits then a byte, a byte plus 0x1F0, or a short.
   */
  auto OT(Version release) -> int;
  /**
   *  T: a BS length then that many bytes, read through the scheme, less the NUL AutoCAD ends a text with; a negative
   *  length fails.
   */
  auto T(const Encoding::Scheme &with) -> std::string;
  /**
   *  TU: a BS count then that many UTF-16 code units, little-endian; a negative count fails.
   */
  auto TU() -> std::string;
  /**
   *  TV: a TU from R2007, a T before.
   */
  auto TV(Version release, const Encoding::Scheme &with) -> std::string;

private:
  auto Fail() -> void;
  auto Take(int count) -> std::uint64_t;
  auto Modular(bool withSign) -> std::uint64_t;
  auto PatchBytes(double other, std::span<const unsigned> order) -> double;
  auto Raw() -> std::string;

  std::span<const std::byte> m_Bytes;
  std::size_t m_Position{};
  bool m_Failed{};
};
} // namespace DwgIO::Bits

#endif /* DWGIO_BITS_READER_HPP */
