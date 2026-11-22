/***********************************************************************************************************************
 *  @file      source/acis/sat/reader.cpp
 *  @brief     A SAT body read word by word.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "acis/sat/reader.hpp"

#include "acis/model.hpp"
#include "acis/spec.hpp"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

namespace DwgIO::Acis::Sat
{
namespace
{
/**
 *  The first ACIS release whose SAT names its product and gives its tolerances after the first line.
 */
constexpr int NamedSince = 200;

auto Space(char one) -> bool
{
  return one == ' ' || one == '\t' || one == '\n' || one == '\r';
}

/**
 *  The whole of a word as a number of that type, or nothing.
 */
template <typename NumberT>
auto Number(std::string_view word) -> std::optional<NumberT>
{
  NumberT value{};
  const char *end = std::to_address(word.end());
  const auto [stop, error] = std::from_chars(std::to_address(word.begin()), end, value);
  if (word.empty() || error != std::errc{} || stop != end)
  {
    return std::nullopt;
  }
  return value;
}
} // namespace

Reader::Reader(std::string_view text) : m_Text(text) {}

auto Reader::Peek() -> std::string_view
{
  while (m_At < m_Text.size() && Space(m_Text[m_At]))
  {
    ++m_At;
  }
  std::size_t end = m_At;
  while (end < m_Text.size() && !Space(m_Text[end]))
  {
    ++end;
  }
  return m_Text.substr(m_At, end - m_At);
}

auto Reader::Word() -> std::string_view
{
  const std::string_view word = Peek();
  m_At += word.size();
  return word;
}

auto Reader::Pointer() -> std::optional<std::int64_t>
{
  const std::string_view word = Word();
  return word.starts_with('$') ? Number<std::int64_t>(word.substr(1)) : std::nullopt;
}

auto Reader::Integer() -> std::optional<std::int64_t>
{
  return Number<std::int64_t>(Word());
}

auto Reader::Real() -> std::optional<double>
{
  return Number<double>(Word());
}

auto Reader::Text() -> std::optional<std::string>
{
  std::string_view word = Word();
  if (word.starts_with('@'))
  {
    word.remove_prefix(1);
  }
  const std::optional<std::int64_t> size = Number<std::int64_t>(word);
  // One space parts the length from the text, which may hold spaces of its own.
  if (!size || *size < 0 || m_Text.size() - m_At < static_cast<std::size_t>(*size) + 1)
  {
    return std::nullopt;
  }
  std::string text{m_Text.substr(m_At + 1, static_cast<std::size_t>(*size))};
  m_At += static_cast<std::size_t>(*size) + 1;
  return text;
}

auto Reader::ReadHead(Head &head) -> bool
{
  for (int *count : {&head.Release, &head.Records, &head.Bodies, &head.Flags})
  {
    const std::optional<std::int64_t> read = Integer();
    if (!read)
    {
      return false;
    }
    *count = static_cast<int>(*read);
  }
  head.Named = head.Release >= NamedSince;
  if (!head.Named)
  {
    return true;
  }
  for (std::string *text : {&head.Product, &head.Modeler, &head.Date})
  {
    const std::optional<std::string> read = Text();
    if (!read)
    {
      return false;
    }
    *text = *read;
  }
  return std::ranges::all_of(head.Tolerances,
                             [this](double &tolerance) -> bool
                             {
                               const std::optional<double> read = Real();
                               tolerance = read.value_or(0);
                               return read.has_value();
                             });
}

auto Reader::Name() -> std::string
{
  const std::string_view word = Word();
  return word == "End-of-ACIS-data" ? std::string{} : std::string{word};
}

auto Reader::ParseChoice(const Spec &spec) -> std::optional<std::int64_t>
{
  const std::string_view word = Word();
  const bool numeric = spec.Kind == Kind::Logical ? spec.Words->Numeric : spec.Table->Numeric;
  if (numeric)
  {
    if (const std::optional<std::int64_t> number = Number<std::int64_t>(word))
    {
      return number;
    }
  }
  if (spec.Kind == Kind::Logical)
  {
    if (word == spec.Words->False)
    {
      return 0;
    }
    return word == spec.Words->True ? std::optional<std::int64_t>{1} : std::nullopt;
  }
  const auto found = std::ranges::find(spec.Table->Names, word);
  return word.empty() || found == spec.Table->Names.end()
             ? std::nullopt
             : std::optional<std::int64_t>{std::distance(spec.Table->Names.begin(), found)};
}

auto Reader::Read(const Spec &spec, Value &value) -> bool
{
  value = Value{.Kind = spec.Kind};
  std::optional<std::int64_t> integer;
  switch (spec.Kind)
  {
  case Kind::Pointer:
    integer = Pointer();
    break;
  case Kind::Long:
  case Kind::Huge:
    integer = Integer();
    break;
  case Kind::Logical:
  case Kind::Choice:
    integer = ParseChoice(spec);
    break;
  case Kind::Real:
  {
    const std::optional<double> real = Real();
    value.Reals[0] = real.value_or(0);
    return real.has_value();
  }
  case Kind::Position:
  case Kind::Direction:
    return std::ranges::all_of(value.Reals,
                               [this](double &real) -> bool
                               {
                                 const std::optional<double> read = Real();
                                 real = read.value_or(0);
                                 return read.has_value();
                               });
  case Kind::Bound:
  {
    const std::string_view word = Word();
    if (word == "I")
    {
      return true;
    }
    const std::optional<double> real = word == "F" ? Real() : std::nullopt;
    value.Integer = 1;
    value.Reals[0] = real.value_or(0);
    return real.has_value();
  }
  case Kind::Text:
  {
    const std::optional<std::string> text = Text();
    value.Text = text.value_or(std::string{});
    return text.has_value();
  }
  case Kind::Word:
    value.Text = Word();
    return !value.Text.empty();
  }
  value.Integer = integer.value_or(0);
  return integer.has_value();
}

auto Reader::Brace(bool open) -> bool
{
  return Word() == (open ? "{" : "}");
}

auto Reader::ReadEnd() -> bool
{
  return Peek() == "#";
}

auto Reader::End() -> bool
{
  return Word() == "#";
}
} // namespace DwgIO::Acis::Sat
