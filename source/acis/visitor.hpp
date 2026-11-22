/***********************************************************************************************************************
 *  @file      source/acis/visitor.hpp
 *  @brief     What walks a record's grammar, reading or writing its fields.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ACIS_VISITOR_HPP
#define DWGIO_ACIS_VISITOR_HPP

#include "acis/model.hpp"
#include "acis/spec.hpp"

#include <string>

namespace DwgIO::Acis
{
/**
 *  What walks a record's grammar: one reads its fields from a body into the model, another writes the model's values
 *  out as another release. Each field gives back its value, which the grammar steers by.
 */
class Visitor
{
public:
  Visitor() = default;
  Visitor(const Visitor &) = delete;
  Visitor(Visitor &&) = delete;
  auto operator=(const Visitor &) -> Visitor & = delete;
  auto operator=(Visitor &&) -> Visitor & = delete;
  virtual ~Visitor() = default;

  /**
   *  The field's value, read or written.
   */
  virtual auto Field(const Spec &spec) -> Value = 0;
  /**
   *  The brace that opens a subtype.
   */
  virtual void Open() = 0;
  /**
   *  The brace that closes it.
   */
  virtual void Close() = 0;
  /**
   *  Where SAT starts a new line, indented, in the releases the gate gives.
   */
  virtual void Break(Gate gate) = 0;
  [[nodiscard]] virtual auto AtEnd() -> bool = 0;
  /**
   *  Stops the walk: the body cannot be read or written, for that reason.
   */
  virtual void Fail(std::string why) = 0;
  /**
   *  Why the walk stopped, or empty.
   */
  [[nodiscard]] virtual auto Failure() const -> const std::string & = 0;
};

} // namespace DwgIO::Acis

#endif /* DWGIO_ACIS_VISITOR_HPP */
