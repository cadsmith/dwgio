/***********************************************************************************************************************
 *  @file      source/acis/writer.hpp
 *  @brief     The visitor that writes a record's fields as a release holds them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ACIS_WRITER_HPP
#define DWGIO_ACIS_WRITER_HPP

#include "acis/model.hpp"
#include "acis/sink.hpp"
#include "acis/spec.hpp"
#include "acis/visitor.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace DwgIO::Acis
{
/**
 *  The release a body is written as, and how far its pointers move: by one where the header record of ACIS 20800 and
 *  later is added or removed.
 */
struct Plan
{
  int Release{};
  std::int64_t Shift{}; /**< What every pointer to a record gains. */
};

/**
 *  Writes a record of the model as a release holds it: the values of the fields it has, in its words.
 */
class FieldWriter final : public Visitor
{
public:
  /**
   *  Both must outlive the writing.
   */
  FieldWriter(Sink &sink, Plan plan, const Record &record);

  auto Field(const Spec &spec) -> Value override;
  void Open() override;
  void Close() override;
  void Break(Gate gate) override;
  [[nodiscard]] auto AtEnd() -> bool override;
  void Fail(std::string why) override;
  [[nodiscard]] auto Failure() const -> const std::string & override;

private:
  Sink *m_Sink;
  Plan m_Plan;
  const Record *m_Record;
  std::size_t m_At{};
  std::string m_Failure{};
};
} // namespace DwgIO::Acis

#endif /* DWGIO_ACIS_WRITER_HPP */
