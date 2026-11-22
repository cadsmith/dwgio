/***********************************************************************************************************************
 *  @file      source/acis/fields.cpp
 *  @brief     The field reads and writes a record's grammar is written in.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "acis/fields.hpp"

#include "acis/model.hpp"
#include "acis/spec.hpp"
#include "acis/visitor.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace DwgIO::Acis
{
Fields::Fields(Visitor &visitor) : m_Visitor(&visitor) {}

auto Fields::Pointer(Gate gate) const -> std::int64_t
{
  return m_Visitor->Field(Spec{.Kind = Kind::Pointer, .Gate = gate, .Fallback = Value{.Integer = -1}}).Integer;
}

auto Fields::Long(Gate gate, std::int64_t fallback) const -> std::int64_t
{
  return m_Visitor->Field(Spec{.Kind = Kind::Long, .Gate = gate, .Fallback = Value{.Integer = fallback}}).Integer;
}

auto Fields::Real(Gate gate, double fallback) const -> double
{
  return m_Visitor->Field(Spec{.Kind = Kind::Real, .Gate = gate, .Fallback = Value{.Reals = {fallback, 0, 0}}})
      .Reals[0];
}

auto Fields::Triple(Kind kind) const -> std::array<double, 3>
{
  return m_Visitor->Field(Spec{.Kind = kind}).Reals;
}

auto Fields::Text(Gate gate, std::string_view fallback) const -> std::string
{
  return m_Visitor->Field(Spec{.Kind = Kind::Text, .Gate = gate, .Fallback = Value{.Text = std::string{fallback}}})
      .Text;
}

auto Fields::Logical(const Words &words, Gate gate, bool fallback) const -> bool
{
  return m_Visitor
             ->Field(Spec{.Kind = Kind::Logical,
                          .Gate = gate,
                          .Fallback = Value{.Integer = static_cast<std::int64_t>(fallback)},
                          .Words = &words})
             .Integer != 0;
}

auto Fields::Choice(const Table &table, Gate gate, std::int64_t fallback) const -> std::int64_t
{
  return m_Visitor
      ->Field(Spec{.Kind = Kind::Choice, .Gate = gate, .Fallback = Value{.Integer = fallback}, .Table = &table})
      .Integer;
}

void Fields::Bound(Gate gate, std::optional<double> fallback) const
{
  static_cast<void>(
      m_Visitor->Field(Spec{.Kind = Kind::Bound,
                            .Gate = gate,
                            .Fallback = Value{.Integer = fallback ? 1 : 0, .Reals = {fallback.value_or(0), 0, 0}}}));
}

auto Fields::Word() const -> std::string
{
  return m_Visitor->Field(Spec{.Kind = Kind::Word}).Text;
}

auto Fields::Huge() const -> std::int64_t
{
  return m_Visitor->Field(Spec{.Kind = Kind::Huge}).Integer;
}

void Fields::Open() const
{
  m_Visitor->Open();
}

void Fields::Close() const
{
  m_Visitor->Close();
}

void Fields::Break(Gate gate) const
{
  m_Visitor->Break(gate);
}

auto Fields::AtEnd() const -> bool
{
  return m_Visitor->AtEnd();
}

void Fields::Fail(std::string why) const
{
  m_Visitor->Fail(std::move(why));
}

auto Fields::HasFailed() const -> bool
{
  return !m_Visitor->Failure().empty();
}
} // namespace DwgIO::Acis
