/***********************************************************************************************************************
 *  @file      source/acis/sab/writer.cpp
 *  @brief     The writer of SAB bodies.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "acis/sab/writer.hpp"

#include "acis/model.hpp"
#include "acis/sab/tags.hpp"
#include "acis/spec.hpp"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

namespace DwgIO::Acis::Sab
{
namespace
{
constexpr std::size_t Short = 2;
constexpr std::size_t Long = 4;
constexpr std::size_t Wide = 8;
constexpr std::uint8_t ByteBits = 8;

/**
 *  The tag of a pointer, a long or an enumeration.
 */
auto Integral(Kind kind) -> std::uint8_t
{
  if (kind == Kind::Pointer)
  {
    return Tags::Pointer;
  }
  return kind == Kind::Long ? Tags::Long : Tags::Choice;
}
} // namespace

void Writer::Integer(std::int64_t value, std::size_t size)
{
  const auto bits = static_cast<std::uint64_t>(value);
  for (std::size_t at = 0; at < size; ++at)
  {
    m_Bytes.push_back(static_cast<char>(static_cast<std::uint8_t>(bits >> (ByteBits * at))));
  }
}

void Writer::Real(double value)
{
  Integer(std::bit_cast<std::int64_t>(value), Wide);
}

void Writer::Word(std::uint8_t tag, std::string_view word)
{
  m_Bytes.push_back(static_cast<char>(tag));
  m_Bytes.push_back(static_cast<char>(static_cast<std::uint8_t>(word.size())));
  m_Bytes += word;
}

void Writer::WriteHead(const Head &head)
{
  m_Release = head.Release;
  m_Bytes = head.Release >= NewMagicSince ? NewMagic : Magic;
  Integer(head.Release, Long);
  Integer(head.Records, Long);
  Integer(head.Bodies, Long);
  Integer(head.Flags, Long);
  for (const std::string *text : {&head.Product, &head.Modeler, &head.Date})
  {
    Write(Spec{.Kind = Kind::Text}, Value{.Kind = Kind::Text, .Text = *text});
  }
  for (const double tolerance : head.Tolerances)
  {
    m_Bytes.push_back(static_cast<char>(Tags::Real));
    Real(tolerance);
  }
}

void Writer::Name(const std::string &name)
{
  std::string_view rest = name;
  for (std::size_t hyphen = rest.find('-'); hyphen != std::string_view::npos; hyphen = rest.find('-'))
  {
    Word(Tags::Part, rest.substr(0, hyphen));
    rest.remove_prefix(hyphen + 1);
  }
  Word(Tags::Word, rest);
}

auto Writer::Write(const Spec &spec, const Value &value) -> bool
{
  switch (spec.Kind)
  {
  case Kind::Pointer:
  case Kind::Long:
  case Kind::Choice:
    m_Bytes.push_back(static_cast<char>(Integral(spec.Kind)));
    Integer(value.Integer, Long);
    break;
  case Kind::Huge:
    m_Bytes.push_back(static_cast<char>(Tags::Huge));
    Integer(value.Integer, Wide);
    break;
  case Kind::Logical:
    m_Bytes.push_back(static_cast<char>(value.Integer != 0 ? Tags::True : Tags::False));
    break;
  case Kind::Real:
    m_Bytes.push_back(static_cast<char>(Tags::Real));
    Real(value.Reals[0]);
    break;
  case Kind::Position:
  case Kind::Direction:
    m_Bytes.push_back(static_cast<char>(spec.Kind == Kind::Position ? Tags::Position : Tags::Direction));
    for (const double real : value.Reals)
    {
      Real(real);
    }
    break;
  case Kind::Bound:
    m_Bytes.push_back(static_cast<char>(value.Integer != 0 ? Tags::True : Tags::False));
    if (value.Integer != 0)
    {
      m_Bytes.push_back(static_cast<char>(Tags::Real));
      Real(value.Reals[0]);
    }
    break;
  case Kind::Text:
    if (value.Text.size() <= std::numeric_limits<std::uint8_t>::max())
    {
      Word(Tags::Text, value.Text);
    }
    else
    {
      m_Bytes.push_back(static_cast<char>(Tags::LongText));
      Integer(static_cast<std::int64_t>(value.Text.size()), Short);
      m_Bytes += value.Text;
    }
    break;
  case Kind::Word:
    Word(Tags::Word, value.Text);
    break;
  }
  return true;
}

void Writer::Brace(bool open)
{
  m_Bytes.push_back(static_cast<char>(open ? Tags::Open : Tags::Close));
}

void Writer::Break() {}

void Writer::End()
{
  m_Bytes.push_back(static_cast<char>(Tags::End));
}

auto Writer::Finish() -> std::string
{
  const auto &parts = m_Release >= NewEndingSince ? NewEnding : Ending;
  for (std::size_t at = 0; at + 1 < parts.size(); ++at)
  {
    Word(Tags::Part, parts.at(at));
  }
  Word(Tags::Word, parts.back());
  return std::move(m_Bytes);
}
} // namespace DwgIO::Acis::Sab
