/***********************************************************************************************************************
 *  @file      source/symbol.cpp
 *  @brief     The pool of interned names that symbols share.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/symbol.hpp"

#include <algorithm>
#include <array>
#include <iterator>
#include <mutex>
#include <unordered_set>

namespace DwgIO
{
namespace
{
struct TextHash
{
  using is_transparent = void; // NOLINT(readability-identifier-naming): the name the standard looks for

  auto operator()(std::string_view text) const noexcept -> std::size_t { return std::hash<std::string_view>{}(text); }
};

struct Pool
{
  std::mutex Lock;
  std::unordered_set<std::string, TextHash, std::equal_to<>> Texts;
};

constexpr std::size_t Remembered = 4;

auto SharedPool() -> Pool &
{
  static Pool all;
  return all;
}

auto FindPooled(std::string_view text) -> const std::string *
{
  Pool &all = SharedPool();
  const std::scoped_lock guard{all.Lock};
  const auto found = all.Texts.find(text);
  return found == all.Texts.end() ? &*all.Texts.emplace(text).first : &*found;
}

auto Intern(std::string_view text) -> const std::string *
{
  static const std::string empty;
  if (text.empty())
  {
    return &empty;
  }
  thread_local std::array<const std::string *, Remembered> recent{};
  const auto *known =
      std::ranges::find_if(recent, [text](const std::string *one) -> bool { return one != nullptr && *one == text; });
  if (known != recent.end())
  {
    return *known;
  }
  std::ranges::rotate(recent, std::prev(recent.end()));
  recent.front() = FindPooled(text);
  return recent.front();
}
} // namespace

Symbol::Symbol() : m_Text(Intern({})) {}

Symbol::Symbol(std::string_view text) : m_Text(Intern(text)) {}

Symbol::Symbol(const std::string &text) : m_Text(Intern(text)) {}

Symbol::Symbol(const char *text) : m_Text(Intern(text)) {}

auto Symbol::View() const noexcept -> std::string_view
{
  return *m_Text;
}

auto Symbol::Str() const noexcept -> const std::string &
{
  return *m_Text;
}

auto Symbol::IsEmpty() const noexcept -> bool
{
  return m_Text->empty();
}

auto Symbol::operator==(std::string_view text) const -> bool
{
  return View() == text;
}
} // namespace DwgIO
