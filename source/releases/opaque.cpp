/***********************************************************************************************************************
 *  @file      source/releases/opaque.cpp
 *  @brief     An untyped object's own bits recast to another release by a layout.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/opaque.hpp"

#include "bits/reader.hpp"
#include "bits/reference.hpp"
#include "bits/writer.hpp"
#include "releases/palette.hpp"

#include "dwgio/color.hpp"
#include "dwgio/tag.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
constexpr int Chunk = 32;

/**
 *  The most times a count repeats its fields: more than any object holds.
 */
constexpr std::int64_t Most = 65536;

/**
 *  The bits of a block element's release, up to R2004 and from R2007.
 */
constexpr std::int32_t OlderRelease = 25;
constexpr std::int32_t OlderMaintenance = 104;
constexpr std::int32_t NewerRelease = 33;
constexpr std::int32_t NewerMaintenance = 377;

/**
 *  The version of the object context data, up to R2007 and from R2010.
 */
constexpr int OlderContext = 3;
constexpr int NewerContext = 4;

/**
 *  The types of an evaluation expression's value the files hold: none, a real, a short and a long, as DXF's groups 40,
 *  70 and 90.
 */
constexpr int NoValue = -9999;
constexpr int Real = 40;
constexpr int Short = 70;
constexpr int Long = 90;

/**
 *  The bits a MASK counts.
 */
constexpr std::uint32_t Masked = 0xF;

/**
 *  The code of the null handle only R14 and R2000 hold: a hard owner.
 */
constexpr int Owner = 3;

/**
 *  Half-open.
 */
struct Stretch
{
  std::size_t From{};
  std::size_t To{};
};

void Copy(const BitString &run, Stretch bits, Bits::Writer &to)
{
  Bits::Reader from{run.Bytes};
  from.Seek(bits.From);
  for (std::size_t at = bits.From; at < bits.To;)
  {
    const auto count = static_cast<int>(std::min<std::size_t>(Chunk, bits.To - at));
    to.Bits(from.Bits(count), count);
    at += static_cast<std::size_t>(count);
  }
}

auto RunOf(const Bits::Writer &writer) -> BitString
{
  const std::span<const std::byte> bytes = writer.Bytes();
  return BitString{.Bytes = {bytes.begin(), bytes.end()}, .Size = writer.Position()};
}

auto IsWide(Version release) -> bool
{
  return release >= Version::R2007;
}

/**
 *  Walks the own bits of one release by a layout, writing them as another holds them.
 */
class Recaster
{
public:
  Recaster(const Opaque &own, const RecastContext &with)
      : m_Own{&own}, m_With{with}, m_Data{own.Data.Bytes}, m_Strings{own.Strings.Bytes}, m_Handles{own.Handles.Bytes}
  {
  }

  /**
   *  False at the first token that does not read.
   */
  auto Run(std::span<const std::string_view> tokens) -> bool
  {
    for (std::size_t at = 0; at < tokens.size() && m_Good; ++at)
    {
      if (tokens[at] == "{")
      {
        at = Repeat(tokens, at);
      }
      else
      {
        CopyToken(tokens[at]);
      }
      m_Good = m_Good && !m_Data.HasFailed() && !m_Strings.HasFailed() && !m_Handles.HasFailed() &&
               m_Data.Position() <= m_Own->Data.Size && m_Strings.Position() <= m_Own->Strings.Size &&
               m_Handles.Position() <= m_Own->Handles.Size;
    }
    return m_Good;
  }

  /**
   *  Nothing unless the walk read every bit of the data and the strings.
   */
  [[nodiscard]] auto Result() const -> std::optional<Opaque>
  {
    if (!m_Good || m_Data.Position() != m_Own->Data.Size || m_Strings.Position() != m_Own->Strings.Size)
    {
      return std::nullopt;
    }
    Bits::Writer handles = m_Linked;
    Copy(m_Own->Handles, Stretch{.From = m_Handles.Position(), .To = m_Own->Handles.Size}, handles);
    return Opaque{.Release = m_With.To, .Data = RunOf(m_Out), .Handles = RunOf(handles), .Strings = RunOf(m_Text)};
  }

private:
  /**
   *  From either side.
   */
  [[nodiscard]] auto Crosses(Version boundary) const -> bool
  {
    return (m_Own->Release >= boundary) != (m_With.To >= boundary);
  }

  /**
   *  The fields between the brace at `open` and its match, repeated the last count's times; the match's index.
   */
  auto Repeat(std::span<const std::string_view> tokens, std::size_t open) -> std::size_t
  {
    std::size_t close = open + 1;
    for (int depth = 1; close < tokens.size(); ++close)
    {
      if (tokens[close] == "{")
      {
        ++depth;
      }
      else if (tokens[close] == "}")
      {
        --depth;
      }
      if (depth == 0)
      {
        break;
      }
    }
    if (m_Counts.empty() || close >= tokens.size())
    {
      m_Good = false;
      return close;
    }
    const std::int64_t count = m_Counts.back();
    m_Counts.pop_back();
    const std::span<const std::string_view> body = tokens.subspan(open + 1, close - open - 1);
    for (std::int64_t done = 0; done < count && m_Good; ++done)
    {
      static_cast<void>(Run(body));
    }
    return close;
  }

  /**
   *  A count, kept for the braces after it.
   */
  void PushCount(std::int64_t count)
  {
    m_Good = m_Good && count >= 0 && count <= Most;
    m_Counts.push_back(count);
  }

  void CopyToken(std::string_view token)
  {
    const std::size_t start = m_Data.Position();
    if (token == "T")
    {
      CopyText();
      return;
    }
    if (token == "E" || token == "V" || token == "CMC" || token == "LOOKUP")
    {
      ChangeField(token);
      return;
    }
    if (token == "REST")
    {
      CopyRest();
      return;
    }
    if (token == "T18" || token == "BL18")
    {
      ConvertLatest(token == "T18");
      return;
    }
    if (token == "H" || token == "H14" || token == "B04")
    {
      CopyLink(token);
      return;
    }
    if (token == "B18")
    {
      const bool set = m_Data.B();
      m_Out.B(set && m_With.To >= Version::R2018);
      return;
    }
    if (token == "N" || token == "MASK")
    {
      const auto value = static_cast<std::uint32_t>(m_Data.BL());
      PushCount(token == "N" ? static_cast<std::int32_t>(value) : std::popcount(value & Masked));
    }
    else if (token == "X")
    {
      CopyValue();
    }
    else
    {
      CopyPlain(token);
    }
    Copy(m_Own->Data, Stretch{.From = start, .To = m_Data.Position()}, m_Out);
  }

  /**
   *  A handle of the handle stream, copied; one only R14 and R2000 hold, a null AutoCAD writes as a hard owner, dropped
   *  when upgrading across R2004 and made when downgrading; a bit AutoCAD writes 1 from R2004, followed from R2013 by
   *  one it writes 0, made and dropped likewise. Any other value does not read.
   */
  void CopyLink(std::string_view token)
  {
    if (token == "H")
    {
      m_Linked.H(m_Handles.H());
      return;
    }
    const bool had = m_Own->Release >= Version::R2004;
    const bool has = m_With.To >= Version::R2004;
    if (token == "H14")
    {
      if (!had && m_Handles.H() != Bits::Reference{.Code = Owner, .Value = {}})
      {
        m_Good = false;
      }
      if (!has)
      {
        m_Linked.H(Bits::Reference{.Code = Owner, .Value = {}});
      }
      return;
    }
    if (had)
    {
      const bool set = m_Data.B();
      const bool cleared = m_Own->Release < Version::R2013 || !m_Data.B();
      if (!set || !cleared)
      {
        m_Good = false;
      }
    }
    if (has)
    {
      m_Out.B(true);
    }
    if (m_With.To >= Version::R2013)
    {
      m_Out.B(false);
    }
  }

  /**
   *  An evaluation expression's value: its type, then nothing for none, a BD for a real, a BS for a short, a BL for a
   *  long; another type does not read.
   */
  void CopyValue()
  {
    const int type = m_Data.BS();
    if (type == Real)
    {
      static_cast<void>(m_Data.BD());
    }
    else if (type == Short)
    {
      static_cast<void>(m_Data.BS());
    }
    else if (type == Long)
    {
      static_cast<void>(m_Data.BL());
    }
    else if (type != NoValue)
    {
      m_Good = false;
    }
  }

  /**
   *  A field kept as it is in every release.
   */
  void CopyPlain(std::string_view token)
  {
    if (token == "B")
    {
      static_cast<void>(m_Data.B());
    }
    else if (token == "RC")
    {
      static_cast<void>(m_Data.RC());
    }
    else if (token == "BS")
    {
      static_cast<void>(m_Data.BS());
    }
    else if (token == "BL")
    {
      static_cast<void>(m_Data.BL());
    }
    else if (token == "BD")
    {
      static_cast<void>(m_Data.BD());
    }
    else if (token == "2RD")
    {
      static_cast<void>(m_Data.RD());
      static_cast<void>(m_Data.RD());
    }
    else if (token == "3BD")
    {
      static_cast<void>(m_Data.Point3());
    }
    else
    {
      m_Good = false;
    }
  }

  /**
   *  The fields whose bits a release changes: a block element's release, a context's version, a color, the cells of a
   *  lookup table.
   */
  void ChangeField(std::string_view token)
  {
    const std::size_t start = m_Data.Position();
    if (token == "E")
    {
      static_cast<void>(m_Data.BL());
      static_cast<void>(m_Data.BL());
      if (Crosses(Version::R2007))
      {
        const bool newer = IsWide(m_With.To);
        m_Out.BL(newer ? NewerRelease : OlderRelease);
        m_Out.BL(newer ? NewerMaintenance : OlderMaintenance);
        return;
      }
    }
    else if (token == "V")
    {
      static_cast<void>(m_Data.BS());
      if (Crosses(Version::R2010))
      {
        m_Out.BS(m_With.To >= Version::R2010 ? NewerContext : OlderContext);
        return;
      }
    }
    else if (token == "CMC")
    {
      CopyColor();
      return;
    }
    else
    {
      CopyLookup();
      return;
    }
    Copy(m_Own->Data, Stretch{.From = start, .To = m_Data.Position()}, m_Out);
  }

  /**
   *  A color: as it is within the releases that write it alike, else read as one writes it and written as the other
   *  does.
   */
  void CopyColor()
  {
    const std::size_t start = m_Data.Position();
    const std::size_t named = m_Strings.Position();
    const Version from = m_Own->Release;
    const Color color = IsWide(from) ? m_Data.CMC(from, m_Strings) : m_Data.CMC(from);
    if (!Crosses(Version::R2004) && !Crosses(Version::R2007))
    {
      Copy(m_Own->Data, Stretch{.From = start, .To = m_Data.Position()}, m_Out);
      Copy(m_Own->Strings, Stretch{.From = named, .To = m_Strings.Position()}, m_Text);
      return;
    }
    const Color moved = m_With.To >= Version::R2004 ? RaiseColor(color) : LowerColor(color);
    if (IsWide(m_With.To))
    {
      m_Out.CMC(moved, m_With.To, m_Text);
    }
    else
    {
      m_Out.CMC(moved, m_With.To);
    }
  }

  /**
   *  A field only R2018 holds, a text or a BL: copied within it, read into what the caller takes when downgrading from
   *  it, written from what it gives when upgrading to it, an empty text or 0 where it gives none; nothing below it.
   */
  void ConvertLatest(bool text)
  {
    const bool had = m_Own->Release >= Version::R2018;
    const bool has = m_With.To >= Version::R2018;
    if (had == has)
    {
      if (had)
      {
        CopyField(text);
      }
      return;
    }
    if (had)
    {
      Content value = text ? Content{m_Strings.TU()} : Content{std::int64_t{m_Data.BL()}};
      if (m_With.Taken != nullptr)
      {
        m_With.Taken->push_back(std::move(value));
      }
      return;
    }
    const Content *given = m_Next < m_With.Given.size() ? &m_With.Given[m_Next] : nullptr;
    ++m_Next;
    if (text)
    {
      const auto *value = given != nullptr ? std::get_if<std::string>(given) : nullptr;
      if (m_Text.TU(value != nullptr ? *value : std::string{}).has_value())
      {
        m_Good = false; // Cut to what a count holds: not the text given.
      }
      return;
    }
    const auto *value = given != nullptr ? std::get_if<std::int64_t>(given) : nullptr;
    m_Out.BL(value != nullptr ? static_cast<std::int32_t>(*value) : 0);
  }

  /**
   *  A field of R2018 copied as it is: a text in the string stream or a BL.
   */
  void CopyField(bool text)
  {
    if (text)
    {
      CopyText();
      return;
    }
    const std::size_t start = m_Data.Position();
    static_cast<void>(m_Data.BL());
    Copy(m_Own->Data, Stretch{.From = start, .To = m_Data.Position()}, m_Out);
  }

  /**
   *  A lookup table: its row and column counts, then a text per cell; its columns the count.
   */
  void CopyLookup()
  {
    const std::size_t start = m_Data.Position();
    const std::int32_t rows = m_Data.BL();
    const std::int32_t columns = m_Data.BL();
    Copy(m_Own->Data, Stretch{.From = start, .To = m_Data.Position()}, m_Out);
    const std::int64_t cells = static_cast<std::int64_t>(rows) * columns;
    if (rows < 0 || columns < 0 || cells > Most)
    {
      m_Good = false;
    }
    for (std::int64_t cell = 0; cell < cells && m_Good; ++cell)
    {
      CopyText();
    }
    PushCount(columns);
  }

  /**
   *  A text, moved between the data and the string stream across R2007, else kept as its bits are.
   */
  void CopyText()
  {
    const Version from = m_Own->Release;
    if (!Crosses(Version::R2007))
    {
      Bits::Reader &source = IsWide(from) ? m_Strings : m_Data;
      const std::size_t start = source.Position();
      static_cast<void>(IsWide(from) ? source.TU() : source.T(*m_With.Text));
      Copy(IsWide(from) ? m_Own->Strings : m_Own->Data, Stretch{.From = start, .To = source.Position()},
           IsWide(from) ? m_Text : m_Out);
      return;
    }
    const std::string text = IsWide(from) ? m_Strings.TU() : m_Data.T(*m_With.Text);
    if (IsWide(m_With.To) ? m_Text.TU(text).has_value() : m_Out.T(text, *m_With.Text).has_value())
    {
      m_Good = false; // Cut to what a count holds: not the text read.
    }
  }

  /**
   *  The rest of the data as it is, and of the strings within the releases that keep them alike; across R2007 there
   *  must be none.
   */
  void CopyRest()
  {
    Copy(m_Own->Data, Stretch{.From = m_Data.Position(), .To = m_Own->Data.Size}, m_Out);
    m_Data.Seek(m_Own->Data.Size);
    if (!Crosses(Version::R2007))
    {
      Copy(m_Own->Strings, Stretch{.From = m_Strings.Position(), .To = m_Own->Strings.Size}, m_Text);
      m_Strings.Seek(m_Own->Strings.Size);
    }
  }

  const Opaque *m_Own;
  RecastContext m_With;
  Bits::Reader m_Data;
  Bits::Reader m_Strings;
  Bits::Reader m_Handles;
  Bits::Writer m_Out;
  Bits::Writer m_Linked; /**< The handles the layout walked, as the release holds them. */
  Bits::Writer m_Text;
  std::vector<std::int64_t> m_Counts;
  std::size_t m_Next{}; /**< The next of the values given for the fields only R2018 holds. */
  bool m_Good{true};
};
} // namespace

auto CollectTokens(std::string_view layout) -> std::vector<std::string_view>
{
  std::vector<std::string_view> tokens;
  for (std::size_t at = 0; at < layout.size();)
  {
    const std::size_t end = std::min(layout.find(' ', at), layout.size());
    if (end > at)
    {
      tokens.push_back(layout.substr(at, end - at));
    }
    at = end + 1;
  }
  return tokens;
}

auto Recast(const Opaque &own, std::string_view layout, const RecastContext &with) -> std::optional<Opaque>
{
  const std::vector<std::string_view> tokens = CollectTokens(layout);
  Recaster recaster{own, with};
  static_cast<void>(recaster.Run(tokens));
  return recaster.Result();
}
} // namespace DwgIO::Releases
