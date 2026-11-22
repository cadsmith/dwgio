/***********************************************************************************************************************
 *  @file      source/acis/reader.hpp
 *  @brief     The visitor that reads a record's fields from a body.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ACIS_READER_HPP
#define DWGIO_ACIS_READER_HPP

#include "acis/model.hpp"
#include "acis/source.hpp"
#include "acis/spec.hpp"
#include "acis/visitor.hpp"

#include <string>

namespace DwgIO::Acis
{
/**
 *  Reads a record's fields from a body into the model: those the body's release has from the body, the others as their
 *  defaults, marked not given.
 */
class FieldReader final : public Visitor
{
public:
  /**
   *  Both must outlive the reading.
   */
  FieldReader(Source &source, int release, Record &record);

  auto Field(const Spec &spec) -> Value override;
  void Open() override;
  void Close() override;
  void Break(Gate gate) override;
  [[nodiscard]] auto AtEnd() -> bool override;
  void Fail(std::string why) override;
  [[nodiscard]] auto Failure() const -> const std::string & override;

private:
  Source *m_Source;
  int m_Release;
  Record *m_Record;
  std::string m_Failure{};
};
} // namespace DwgIO::Acis

#endif /* DWGIO_ACIS_READER_HPP */
