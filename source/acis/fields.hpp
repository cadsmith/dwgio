/***********************************************************************************************************************
 *  @file      source/acis/fields.hpp
 *  @brief     The field reads and writes a record's grammar is written in.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ACIS_FIELDS_HPP
#define DWGIO_ACIS_FIELDS_HPP

#include "acis/model.hpp"
#include "acis/visitor.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace DwgIO::Acis
{
/**
 *  The releases from which a field is in a record.
 */
namespace Since
{
constexpr Gate R2000{.Since = Releases::R2000};
constexpr Gate R2004{.Since = Releases::R2004};
constexpr Gate R2007{.Since = Releases::R2007};
constexpr Gate R2010{.Since = Releases::R2010};
constexpr Gate R2013{.Since = Releases::R2013};
} // namespace Since

/**
 *  A record's fields one by one, each of its kind, walked by a visitor: what a grammar is written in.
 */
class Fields
{
public:
  /**
   *  The visitor must outlive the fields.
   */
  explicit Fields(Visitor &visitor);

  /**
   *  A pointer to another record, -1 where the release lacks it.
   */
  [[nodiscard]] auto Pointer(Gate gate = Always) const -> std::int64_t;
  [[nodiscard]] auto Long(Gate gate = Always, std::int64_t fallback = 0) const -> std::int64_t;
  [[nodiscard]] auto Real(Gate gate = Always, double fallback = 0) const -> double;
  /**
   *  A position or a direction.
   */
  [[nodiscard]] auto Triple(Kind kind) const -> std::array<double, 3>;
  [[nodiscard]] auto Text(Gate gate, std::string_view fallback) const -> std::string;
  [[nodiscard]] auto Logical(const Words &words, Gate gate = Always, bool fallback = false) const -> bool;
  [[nodiscard]] auto Choice(const Table &table, Gate gate = Always, std::int64_t fallback = 0) const -> std::int64_t;
  /**
   *  An end of an interval, infinite where the release lacks it unless finite at the fallback.
   */
  void Bound(Gate gate = Always, std::optional<double> fallback = std::nullopt) const;
  [[nodiscard]] auto Word() const -> std::string;
  [[nodiscard]] auto Huge() const -> std::int64_t;
  /**
   *  The brace that opens a subtype.
   */
  void Open() const;
  void Close() const;
  /**
   *  A new line, indented, in SAT of the releases the gate gives.
   */
  void Break(Gate gate = Always) const;
  [[nodiscard]] auto AtEnd() const -> bool;
  /**
   *  Stops the walk for that reason.
   */
  void Fail(std::string why) const;
  [[nodiscard]] auto HasFailed() const -> bool;

private:
  Visitor *m_Visitor;
};
} // namespace DwgIO::Acis

#endif /* DWGIO_ACIS_FIELDS_HPP */
