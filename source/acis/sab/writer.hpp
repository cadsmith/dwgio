/***********************************************************************************************************************
 *  @file      source/acis/sab/writer.hpp
 *  @brief     The writer of SAB bodies.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ACIS_SAB_WRITER_HPP
#define DWGIO_ACIS_SAB_WRITER_HPP

#include "acis/model.hpp"
#include "acis/sink.hpp"
#include "acis/spec.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace DwgIO::Acis::Sab
{
/**
 *  Writes a SAB body of a release: the magic of its release, the four longs, the tagged product, modeler, date and
 *  tolerances, the records, each value behind its tag, and the record that ends the body.
 */
class Writer final : public Acis::Sink
{
public:
  void WriteHead(const Head &head) override;
  void Name(const std::string &name) override;
  auto Write(const Spec &spec, const Value &value) -> bool override;
  void Brace(bool open) override;
  void Break() override;
  void End() override;
  auto Finish() -> std::string override;

private:
  /**
   *  A little-endian integer of that many bytes.
   */
  void Integer(std::int64_t value, std::size_t size);
  void Real(double value);
  /**
   *  A word behind its tag and length.
   */
  void Word(std::uint8_t tag, std::string_view word);

  std::string m_Bytes;
  int m_Release{};
};
} // namespace DwgIO::Acis::Sab

#endif /* DWGIO_ACIS_SAB_WRITER_HPP */
