/***********************************************************************************************************************
 *  @file      source/acis/sab/reader.cpp
 *  @brief     The reader of SAB bodies.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "acis/sab/reader.hpp"

#include "acis/model.hpp"
#include "acis/sab/tags.hpp"
#include "acis/spec.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace DwgIO::Acis::Sab
{
namespace
{
constexpr std::size_t Short = 2;
constexpr std::size_t Long = 4;
constexpr std::size_t Wide = 8;
constexpr std::size_t ByteBits = 8;

/**
 *  The parts of a name joined by hyphens.
 */
auto JoinName(const std::array<std::string_view, 4> &parts) -> std::string
{
  std::string name{parts[0]};
  for (std::size_t at = 1; at < parts.size(); ++at)
  {
    name += "-" + std::string{parts.at(at)};
  }
  return name;
}
} // namespace

Reader::Reader(std::string_view bytes) : m_Bytes(bytes) {}

auto Reader::TakeTag(std::uint8_t tag) -> bool
{
  if (!m_Failed && m_At < m_Bytes.size() && static_cast<std::uint8_t>(m_Bytes[m_At]) == tag)
  {
    ++m_At;
    return true;
  }
  return false;
}

void Reader::Expect(std::uint8_t tag)
{
  m_Failed = !TakeTag(tag);
}

auto Reader::Integer(std::size_t size) -> std::int64_t
{
  if (m_Failed || m_Bytes.size() - m_At < size)
  {
    m_Failed = true;
    return 0;
  }
  std::uint64_t value = 0;
  for (std::size_t at = 0; at < size; ++at)
  {
    value |= static_cast<std::uint64_t>(static_cast<std::uint8_t>(m_Bytes[m_At + at])) << (ByteBits * at);
  }
  m_At += size;
  // Sign-extended from its size: a long of four bytes is negative when its top bit is.
  const std::uint64_t sign = std::uint64_t{1} << ((ByteBits * size) - 1);
  return static_cast<std::int64_t>((value ^ sign) - sign);
}

auto Reader::Real() -> double
{
  return std::bit_cast<double>(Integer(Wide));
}

auto Reader::Chars(std::size_t size) -> std::string
{
  if (m_Failed || m_Bytes.size() - m_At < size)
  {
    m_Failed = true;
    return {};
  }
  std::string text{m_Bytes.substr(m_At, size)};
  m_At += size;
  return text;
}

auto Reader::Text() -> std::string
{
  if (TakeTag(Tags::LongText))
  {
    return Chars(static_cast<std::uint16_t>(Integer(Short)));
  }
  Expect(Tags::Text);
  return Chars(static_cast<std::uint8_t>(Integer(1)));
}

auto Reader::Word(std::uint8_t tag) -> std::string
{
  Expect(tag);
  return Chars(static_cast<std::uint8_t>(Integer(1)));
}

auto Reader::ReadHead(Head &head) -> bool
{
  const std::string_view magic = m_Bytes.substr(0, Magic.size());
  m_At = Magic.size();
  m_Failed = magic != Magic && magic != NewMagic;
  head.Release = static_cast<int>(Integer(Long));
  head.Records = static_cast<int>(Integer(Long));
  head.Bodies = static_cast<int>(Integer(Long));
  head.Flags = static_cast<int>(Integer(Long));
  head.Named = true;
  head.Product = Text();
  head.Modeler = Text();
  head.Date = Text();
  for (double &tolerance : head.Tolerances)
  {
    Expect(Tags::Real);
    tolerance = Real();
  }
  return !m_Failed;
}

auto Reader::Name() -> std::string
{
  std::string name;
  while (TakeTag(Tags::Part))
  {
    name += Chars(static_cast<std::uint8_t>(Integer(1))) + "-";
  }
  if (!TakeTag(Tags::Word))
  {
    return {};
  }
  name += Chars(static_cast<std::uint8_t>(Integer(1)));
  return m_Failed || name == JoinName(Sab::Ending) || name == JoinName(NewEnding) ? std::string{} : name;
}

auto Reader::Read(const Spec &spec, Value &value) -> bool
{
  value = Value{.Kind = spec.Kind};
  switch (spec.Kind)
  {
  case Kind::Pointer:
    Expect(Tags::Pointer);
    value.Integer = Integer(Long);
    break;
  case Kind::Long:
    Expect(Tags::Long);
    value.Integer = Integer(Long);
    break;
  case Kind::Choice:
    Expect(Tags::Choice);
    value.Integer = Integer(Long);
    break;
  case Kind::Huge:
    Expect(Tags::Huge);
    value.Integer = Integer(Wide);
    break;
  case Kind::Logical:
    value.Integer = TakeTag(Tags::True) ? 1 : 0;
    m_Failed = m_Failed || (value.Integer == 0 && !TakeTag(Tags::False));
    break;
  case Kind::Real:
    Expect(Tags::Real);
    value.Reals[0] = Real();
    break;
  case Kind::Position:
  case Kind::Direction:
    Expect(spec.Kind == Kind::Position ? Tags::Position : Tags::Direction);
    for (double &real : value.Reals)
    {
      real = Real();
    }
    break;
  case Kind::Bound:
    value.Integer = TakeTag(Tags::False) ? 0 : 1;
    if (value.Integer != 0)
    {
      Expect(Tags::True);
      Expect(Tags::Real);
      value.Reals[0] = Real();
    }
    break;
  case Kind::Text:
    value.Text = Text();
    break;
  case Kind::Word:
    value.Text = Word(Tags::Word);
    break;
  }
  return !m_Failed;
}

auto Reader::Brace(bool open) -> bool
{
  return TakeTag(open ? Tags::Open : Tags::Close);
}

auto Reader::ReadEnd() -> bool
{
  return m_At < m_Bytes.size() && static_cast<std::uint8_t>(m_Bytes[m_At]) == Tags::End;
}

auto Reader::End() -> bool
{
  return TakeTag(Tags::End);
}
} // namespace DwgIO::Acis::Sab
