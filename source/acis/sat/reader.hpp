/***********************************************************************************************************************
 *  @file      source/acis/sat/reader.hpp
 *  @brief     A SAT body read word by word.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ACIS_SAT_READER_HPP
#define DWGIO_ACIS_SAT_READER_HPP

#include "acis/model.hpp"
#include "acis/source.hpp"
#include "acis/spec.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

/**
 *  SAT, the text ACIS AutoCAD writes in R14 and R2000: a head of one line in ACIS 1.06 and three from 4.00, then a
 *  record a line, its values words apart.
 */
namespace DwgIO::Acis::Sat
{
/**
 *  Reads a SAT body word by word, what each word is told by the grammar.
 */
class Reader final : public Acis::Source
{
public:
  /**
   *  The text must outlive the reader.
   */
  explicit Reader(std::string_view text);

  auto ReadHead(Head &head) -> bool override;
  auto Name() -> std::string override;
  auto Read(const Spec &spec, Value &value) -> bool override;
  auto Brace(bool open) -> bool override;
  [[nodiscard]] auto ReadEnd() -> bool override;
  auto End() -> bool override;

private:
  /**
   *  The next word, empty at the end.
   */
  auto Word() -> std::string_view;
  auto Peek() -> std::string_view;
  /**
   *  The next word as a pointer, `$` and its index, or nothing.
   */
  auto Pointer() -> std::optional<std::int64_t>;
  /**
   *  The next word as a long, or nothing.
   */
  auto Integer() -> std::optional<std::int64_t>;
  /**
   *  The next word as a double, or nothing.
   */
  auto Real() -> std::optional<double>;
  /**
   *  Text: its length, a space, then that many characters; or nothing.
   */
  auto Text() -> std::optional<std::string>;
  /**
   *  A logical or an enumeration's value from its word, or nothing.
   */
  auto ParseChoice(const Spec &spec) -> std::optional<std::int64_t>;

  std::string_view m_Text;
  std::size_t m_At{};
};
} // namespace DwgIO::Acis::Sat

#endif /* DWGIO_ACIS_SAT_READER_HPP */
