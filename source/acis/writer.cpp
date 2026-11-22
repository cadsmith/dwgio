/***********************************************************************************************************************
 *  @file      source/acis/writer.cpp
 *  @brief     The visitor that writes a record's fields as a release holds them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "acis/writer.hpp"

#include "acis/model.hpp"
#include "acis/sink.hpp"
#include "acis/spec.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <string>
#include <string_view>
#include <utility>

namespace DwgIO::Acis
{
namespace
{
/**
 *  The subtypes ACIS 21200 renamed, the old name first.
 */
constexpr std::array<std::pair<std::string_view, std::string_view>, 3> Renamed{
    {{"exactcur", "exact_int_cur"}, {"exppc", "exp_par_cur"}, {"exactsur", "exact_spl_sur"}}};

/**
 *  A word as the release spells it.
 */
auto SpellingOf(const std::string &word, int release) -> std::string
{
  for (const auto &[old, renamed] : Renamed)
  {
    if (word == old || word == renamed)
    {
      return std::string{release >= Releases::R2007 ? renamed : old};
    }
  }
  return word;
}
} // namespace

FieldWriter::FieldWriter(Sink &sink, Plan plan, const Record &record) : m_Sink(&sink), m_Plan(plan), m_Record(&record)
{
}

auto FieldWriter::Field(const Spec &spec) -> Value
{
  if (!m_Failure.empty())
  {
    return spec.Fallback;
  }
  if (m_At >= m_Record->Values.size() || m_Record->Values[m_At].Kind != spec.Kind)
  {
    Fail(std::format("{} holds other values", m_Record->Name));
    return spec.Fallback;
  }
  const Value &value = m_Record->Values[m_At++];
  if (spec.Gate.Has(m_Plan.Release))
  {
    Value written = value;
    if (spec.Kind == Kind::Pointer && written.Integer >= 0)
    {
      written.Integer += m_Plan.Shift;
    }
    if (spec.Kind == Kind::Word)
    {
      written.Text = SpellingOf(written.Text, m_Plan.Release);
    }
    if (!m_Sink->Write(spec, written))
    {
      Fail(std::format("{} of {} has no word in SAT", written.Integer, m_Record->Name));
    }
  }
  return value;
}

void FieldWriter::Open()
{
  m_Sink->Brace(true);
}

void FieldWriter::Close()
{
  m_Sink->Brace(false);
}

void FieldWriter::Break(Gate gate)
{
  if (gate.Has(m_Plan.Release))
  {
    m_Sink->Break();
  }
}

auto FieldWriter::AtEnd() -> bool
{
  return !m_Failure.empty() || m_At >= m_Record->Values.size();
}

void FieldWriter::Fail(std::string why)
{
  if (m_Failure.empty())
  {
    m_Failure = std::move(why);
  }
}

auto FieldWriter::Failure() const -> const std::string &
{
  return m_Failure;
}
} // namespace DwgIO::Acis
