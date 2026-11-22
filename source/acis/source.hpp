/***********************************************************************************************************************
 *  @file      source/acis/source.hpp
 *  @brief     Where an ACIS body is read from, SAT text or SAB bytes.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ACIS_SOURCE_HPP
#define DWGIO_ACIS_SOURCE_HPP

#include "acis/model.hpp"
#include "acis/spec.hpp"

#include <string>

namespace DwgIO::Acis
{
/**
 *  Where a body is read from: SAT text or SAB bytes.
 */
class Source
{
public:
  Source() = default;
  Source(const Source &) = delete;
  Source(Source &&) = delete;
  auto operator=(const Source &) -> Source & = delete;
  auto operator=(Source &&) -> Source & = delete;
  virtual ~Source() = default;

  /**
   *  The head; false when the body opens otherwise.
   */
  virtual auto ReadHead(Head &head) -> bool = 0;
  /**
   *  The next record's name, or empty after the last.
   */
  virtual auto Name() -> std::string = 0;
  /**
   *  A field's value, of its kind; false when the body holds no such thing there.
   */
  virtual auto Read(const Spec &spec, Value &value) -> bool = 0;
  /**
   *  A brace, the opening one when open; false when the body holds none.
   */
  virtual auto Brace(bool open) -> bool = 0;
  [[nodiscard]] virtual auto ReadEnd() -> bool = 0;
  /**
   *  The end of the record; false when the body holds more first.
   */
  virtual auto End() -> bool = 0;
};
} // namespace DwgIO::Acis

#endif /* DWGIO_ACIS_SOURCE_HPP */
