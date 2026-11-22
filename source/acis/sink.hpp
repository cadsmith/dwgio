/***********************************************************************************************************************
 *  @file      source/acis/sink.hpp
 *  @brief     Where an ACIS body is written to, SAT text or SAB bytes.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ACIS_SINK_HPP
#define DWGIO_ACIS_SINK_HPP

#include "acis/model.hpp"
#include "acis/spec.hpp"

#include <string>

namespace DwgIO::Acis
{
/**
 *  Where a body is written to: SAT text or SAB bytes.
 */
class Sink
{
public:
  Sink() = default;
  Sink(const Sink &) = delete;
  Sink(Sink &&) = delete;
  auto operator=(const Sink &) -> Sink & = delete;
  auto operator=(Sink &&) -> Sink & = delete;
  virtual ~Sink() = default;

  virtual void WriteHead(const Head &head) = 0;
  /**
   *  A record's name, which starts it.
   */
  virtual void Name(const std::string &name) = 0;
  /**
   *  A field's value; false when the format has no way to write it.
   */
  virtual auto Write(const Spec &spec, const Value &value) -> bool = 0;
  /**
   *  A brace, the opening one when open.
   */
  virtual void Brace(bool open) = 0;
  /**
   *  A new line, indented, in SAT.
   */
  virtual void Break() = 0;
  virtual void End() = 0;
  /**
   *  What follows the last record, and the whole body.
   */
  virtual auto Finish() -> std::string = 0;
};
} // namespace DwgIO::Acis

#endif /* DWGIO_ACIS_SINK_HPP */
