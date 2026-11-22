/***********************************************************************************************************************
 *  @file      source/acis/sab/reader.hpp
 *  @brief     The reader of SAB bodies.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ACIS_SAB_READER_HPP
#define DWGIO_ACIS_SAB_READER_HPP

#include "acis/model.hpp"
#include "acis/source.hpp"
#include "acis/spec.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace DwgIO::Acis::Sab
{
/**
 *  Reads a SAB body: the magic, four longs and the tagged product, modeler, date and tolerances, then the records, each
 *  value behind its tag, to the record that ends the body.
 *
 *  A read past the end or of another tag than the one asked fails the reader, which then gives zeros and empty text.
 */
class Reader final : public Acis::Source
{
public:
  /**
   *  The bytes must outlive the reader.
   */
  explicit Reader(std::string_view bytes);

  auto ReadHead(Head &head) -> bool override;
  auto Name() -> std::string override;
  auto Read(const Spec &spec, Value &value) -> bool override;
  auto Brace(bool open) -> bool override;
  [[nodiscard]] auto ReadEnd() -> bool override;
  auto End() -> bool override;

private:
  /**
   *  Whether the next byte is that tag, which it then takes; a reader that has failed has no tag.
   */
  auto TakeTag(std::uint8_t tag) -> bool;
  /**
   *  The next byte must be that tag; fails the reader when not.
   */
  void Expect(std::uint8_t tag);
  /**
   *  A little-endian integer of that many bytes, sign-extended.
   */
  auto Integer(std::size_t size) -> std::int64_t;
  auto Real() -> double;
  /**
   *  Text of that many bytes.
   */
  auto Chars(std::size_t size) -> std::string;
  /**
   *  Text behind its tag and length.
   */
  auto Text() -> std::string;
  /**
   *  A word behind that tag and its length.
   */
  auto Word(std::uint8_t tag) -> std::string;

  std::string_view m_Bytes;
  std::size_t m_At{};
  bool m_Failed{};
};
} // namespace DwgIO::Acis::Sab

#endif /* DWGIO_ACIS_SAB_READER_HPP */
