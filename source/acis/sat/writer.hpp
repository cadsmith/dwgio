/***********************************************************************************************************************
 *  @file      source/acis/sat/writer.hpp
 *  @brief     A SAT body written as AutoCAD lays it out.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ACIS_SAT_WRITER_HPP
#define DWGIO_ACIS_SAT_WRITER_HPP

#include "acis/model.hpp"
#include "acis/sink.hpp"
#include "acis/spec.hpp"

#include <string>

namespace DwgIO::Acis::Sat
{
/**
 *  Writes a SAT body of ACIS 1.06 or 4.00 as AutoCAD lays it out: the first line padded to twenty characters, from 4.00
 *  the product and the tolerances on lines of their own, a record a line, each word followed by a space, the subtypes'
 *  lists on lines of their own behind a tab.
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
   *  A double as AutoCAD's SAT writes it, nineteen significant digits.
   */
  void Real(double value);

  Head m_Head{};
  std::string m_Body{};
};
} // namespace DwgIO::Acis::Sat

#endif /* DWGIO_ACIS_SAT_WRITER_HPP */
