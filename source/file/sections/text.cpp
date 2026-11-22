/***********************************************************************************************************************
 *  @file      source/file/sections/text.cpp
 *  @brief     The texts and numbers of the file's own sections.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/sections/text.hpp"

#include "encoding/units.hpp"

#include <algorithm>
#include <cstdint>
#include <string>

namespace DwgIO::File::Sections
{
namespace
{
/**
 *  The most characters a short counts, the NUL after the text among them.
 */
constexpr std::size_t MostEnded = 0xFFFF;

constexpr std::uint16_t Low = 0xFF;
} // namespace

TextReader::TextReader(std::span<const std::byte> bytes, Version release, const Encoding::Scheme &text)
    : m_In{bytes}, m_Wide{release >= Version::R2007}, m_Text{&text}
{
}

auto TextReader::Text(TextCount counted) -> std::string
{
  // A count of characters, or of bytes, two a character from R2007; each read while the bytes last.
  const std::uint32_t count = counted == TextCount::Ended ? m_In.RS() : m_In.RL();
  const std::uint32_t characters = m_Wide && counted == TextCount::Sized ? count / 2 : count;
  std::u16string units;
  std::string raw;
  for (std::uint32_t at = 0; at < characters && !m_In.HasFailed(); ++at)
  {
    if (m_Wide)
    {
      units.push_back(m_In.RS());
    }
    else
    {
      raw.push_back(static_cast<char>(m_In.RC()));
    }
  }
  if (counted == TextCount::Ended && !units.empty() && units.back() == 0)
  {
    units.pop_back();
  }
  if (counted == TextCount::Ended && !raw.empty() && raw.back() == '\0')
  {
    raw.pop_back();
  }
  return m_Wide ? Encoding::FromUnits(units) : Encoding::Decode(*m_Text, raw);
}

auto TextReader::Short() -> std::uint16_t
{
  return m_In.RS();
}

auto TextReader::Long() -> std::uint32_t
{
  return m_In.RL();
}

auto TextReader::HasFailed() const -> bool
{
  return m_In.HasFailed();
}

TextWriter::TextWriter(Version release, const Encoding::Scheme &text) : m_Wide{release >= Version::R2007}, m_Text{&text}
{
}

void TextWriter::Text(const std::string &text, TextCount counted)
{
  std::u16string units;
  if (m_Wide)
  {
    units = Encoding::ToUnits(text);
  }
  else
  {
    for (const char one : Encoding::Encode(*m_Text, text))
    {
      units.push_back(static_cast<char16_t>(static_cast<unsigned char>(one)));
    }
  }
  if (counted == TextCount::Ended)
  {
    units.resize(std::min(units.size(), MostEnded - 1));
    units.push_back(0);
    m_Out.RS(static_cast<std::uint16_t>(units.size()));
  }
  else
  {
    m_Out.RL(static_cast<std::uint32_t>(m_Wide ? units.size() * 2 : units.size()));
  }
  for (const char16_t unit : units)
  {
    if (m_Wide)
    {
      m_Out.RS(unit);
    }
    else
    {
      m_Out.RC(static_cast<std::uint8_t>(unit & Low));
    }
  }
}

void TextWriter::Byte(std::uint8_t value)
{
  m_Out.RC(value);
}

void TextWriter::Short(std::uint16_t value)
{
  m_Out.RS(value);
}

void TextWriter::Long(std::uint32_t value)
{
  m_Out.RL(value);
}

auto TextWriter::Take() -> std::vector<std::byte>
{
  return m_Out.Take();
}
} // namespace DwgIO::File::Sections
