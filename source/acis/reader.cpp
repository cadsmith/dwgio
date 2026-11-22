/***********************************************************************************************************************
 *  @file      source/acis/reader.cpp
 *  @brief     The visitor that reads a record's fields from a body.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "acis/reader.hpp"

#include "acis/model.hpp"
#include "acis/source.hpp"
#include "acis/spec.hpp"

#include <format>
#include <string>
#include <utility>

namespace DwgIO::Acis
{
FieldReader::FieldReader(Source &source, int release, Record &record)
    : m_Source(&source), m_Release(release), m_Record(&record)
{
}

auto FieldReader::Field(const Spec &spec) -> Value
{
  Value value = spec.Fallback;
  value.Kind = spec.Kind;
  value.Given = false;
  if (m_Failure.empty() && spec.Gate.Has(m_Release))
  {
    value.Given = true;
    if (!m_Source->Read(spec, value))
    {
      Fail(std::format("{} of {} not read", value.Kind == Kind::Word ? "word" : "value", m_Record->Name));
    }
  }
  m_Record->Values.push_back(value);
  return value;
}

void FieldReader::Open()
{
  if (m_Failure.empty() && !m_Source->Brace(true))
  {
    Fail(std::format("subtype of {} not opened", m_Record->Name));
  }
}

void FieldReader::Close()
{
  if (m_Failure.empty() && !m_Source->Brace(false))
  {
    Fail(std::format("subtype of {} not closed", m_Record->Name));
  }
}

void FieldReader::Break(Gate /*gate*/) {}

auto FieldReader::AtEnd() -> bool
{
  return !m_Failure.empty() || m_Source->ReadEnd();
}

void FieldReader::Fail(std::string why)
{
  if (m_Failure.empty())
  {
    m_Failure = std::move(why);
  }
}

auto FieldReader::Failure() const -> const std::string &
{
  return m_Failure;
}
} // namespace DwgIO::Acis
