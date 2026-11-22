/***********************************************************************************************************************
 *  @file      source/acis/sat/writer.cpp
 *  @brief     A SAT body written as AutoCAD lays it out.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "acis/sat/writer.hpp"

#include "acis/model.hpp"
#include "acis/spec.hpp"

#include <cstddef>
#include <format>
#include <string>
#include <string_view>

namespace DwgIO::Acis::Sat
{
namespace
{
/**
 *  The first ACIS release whose SAT names its product and gives its tolerances, and writes every logical and
 *  enumeration as a word.
 */
constexpr int NamedSince = 200;
/**
 *  The width AutoCAD pads the first line to.
 */
constexpr std::size_t FirstLine = 20;
} // namespace

void Writer::Real(double value)
{
  m_Body += std::format("{:.19g} ", value);
}

void Writer::WriteHead(const Head &head)
{
  m_Head = head;
  m_Head.Records = 0;
}

void Writer::Name(const std::string &name)
{
  ++m_Head.Records;
  m_Body += name + " ";
}

auto Writer::Write(const Spec &spec, const Value &value) -> bool
{
  const bool numeric = m_Head.Release < NamedSince;
  switch (spec.Kind)
  {
  case Kind::Pointer:
    m_Body += std::format("${} ", value.Integer);
    break;
  case Kind::Long:
  case Kind::Huge:
    m_Body += std::format("{} ", value.Integer);
    break;
  case Kind::Real:
    Real(value.Reals[0]);
    break;
  case Kind::Position:
  case Kind::Direction:
    for (const double real : value.Reals)
    {
      Real(real);
    }
    break;
  case Kind::Text:
    m_Body += std::format("{} {} ", value.Text.size(), value.Text);
    break;
  case Kind::Word:
    m_Body += value.Text + " ";
    break;
  case Kind::Bound:
    m_Body += value.Integer != 0 ? "F " : "I ";
    if (value.Integer != 0)
    {
      Real(value.Reals[0]);
    }
    break;
  case Kind::Logical:
    if (numeric && spec.Words->Numeric)
    {
      m_Body += std::format("{} ", value.Integer);
      break;
    }
    m_Body += std::string{value.Integer != 0 ? spec.Words->True : spec.Words->False} + " ";
    break;
  case Kind::Choice:
  {
    if (numeric && spec.Table->Numeric)
    {
      m_Body += std::format("{} ", value.Integer);
      break;
    }
    const auto at = static_cast<std::size_t>(value.Integer);
    const std::string_view word = value.Integer >= 0 && at < spec.Table->Names.size() ? spec.Table->Names[at] : "";
    if (word.empty())
    {
      return false;
    }
    m_Body += std::string{word} + " ";
    break;
  }
  }
  return true;
}

void Writer::Brace(bool open)
{
  m_Body += open ? "{ " : "} ";
}

void Writer::Break()
{
  m_Body += "\n\t";
}

void Writer::End()
{
  m_Body += "#\n";
}

auto Writer::Finish() -> std::string
{
  std::string text = std::format(
      "{:<{}}\n", std::format("{} {} {} {}", m_Head.Release, m_Head.Records, m_Head.Bodies, m_Head.Flags), FirstLine);
  if (m_Head.Release >= NamedSince)
  {
    for (const std::string *name : {&m_Head.Product, &m_Head.Modeler, &m_Head.Date})
    {
      text += std::format("{} {} ", name->size(), *name);
    }
    text += '\n';
    for (const double tolerance : m_Head.Tolerances)
    {
      text += std::format("{:.19g} ", tolerance);
    }
    text += '\n';
  }
  return text + m_Body;
}
} // namespace DwgIO::Acis::Sat
