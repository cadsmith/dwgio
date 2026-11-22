/***********************************************************************************************************************
 *  @file      source/file/paged/layout.cpp
 *  @brief     A paged file cut into its sections, and written from them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/paged/layout.hpp"

#include "file/names.hpp"
#include "file/paged/names.hpp"

#include <algorithm>
#include <format>
#include <functional>
#include <optional>
#include <set>
#include <utility>

namespace DwgIO::File::Paged
{
namespace
{
constexpr std::uint32_t Packed = 2;
/**
 *  The largest page size a section may name; a page's zeros are made to it before its bytes are kept.
 */
constexpr std::uint32_t LargestPage = 0x100000;

/**
 *  A section's bytes gathered from its pages.
 */
class Gatherer
{
public:
  Gatherer(const Pager &pager, std::span<const Page> pages) : m_Pager(&pager), m_Pages(pages) {}

  /**
   *  The section's bytes, or the failure naming the page at fault.
   */
  [[nodiscard]] auto Of(const Description &description) -> std::expected<std::vector<std::byte>, Failure>
  {
    std::vector<std::byte> bytes;
    for (const SectionPage &one : description.Pages)
    {
      if (const std::optional<Failure> failed = Add(description, one, bytes))
      {
        return std::unexpected(*failed);
      }
    }
    bytes.resize(static_cast<std::size_t>(std::min<std::uint64_t>(description.Size, bytes.size())));
    return bytes;
  }

private:
  [[nodiscard]] auto Add(const Description &description, const SectionPage &one, std::vector<std::byte> &bytes) const
      -> std::optional<Failure>
  {
    const Page *page = FindPage(m_Pages, one.Number);
    if (page == nullptr)
    {
      return Failure{.Code = FailureCode::Damaged,
                     .What = std::format("section {} {}", description.Name, MakeMissingPage(one.Number).What)};
    }
    if (one.Start > bytes.size())
    {
      bytes.resize(static_cast<std::size_t>(one.Start), std::byte{0});
    }
    if (!m_Pager->DataPage(page->Address, description, bytes))
    {
      return Failure{.Code = FailureCode::Damaged,
                     .Offset = page->Address,
                     .What = std::format("section {} page {} unreadable", description.Name, one.Number)};
    }
    return std::nullopt;
  }

  const Pager *m_Pager;
  std::span<const Page> m_Pages;
};

/**
 *  Where a section's bytes start in the stream, for the order; after everything when it has no page.
 */
auto StartOf(std::span<const Page> pages, const Description &description) -> std::size_t
{
  if (description.Pages.empty())
  {
    return static_cast<std::size_t>(-1);
  }
  return FindPage(pages, description.Pages.front().Number)->Address; // Gathered already, so located.
}

/**
 *  Lays the pages of a file out from 0x100.
 */
class Writer
{
public:
  explicit Writer(Head &head) : m_Head(&head) {}

  void Add(const Section &section)
  {
    Description description{.Size = section.Bytes.size(),
                            .PageSize = section.PageSize,
                            .Unknown1 = 1,
                            .Compressed = section.Compressed ? Packed : 1,
                            .Id = section.Id,
                            .Encrypted = 0,
                            .Name = AcdbOf(section.Name),
                            .Pages = {}};
    if (!section.Compressed)
    {
      // A stored section takes one page: its page size, or a multiple of it when the bytes outgrow it, as AutoCAD
      // sizes the preview's.
      const std::size_t unit = std::max<std::size_t>(section.PageSize, Alignment);
      const std::size_t least = std::max<std::size_t>(section.Bytes.size(), 1);
      description.PageSize = static_cast<std::uint32_t>(((least + unit - 1) / unit) * unit);
    }
    const std::span<const std::byte> bytes = section.Bytes;
    if (section.Name == Names::Preview)
    {
      m_Head->Image = static_cast<std::uint32_t>(m_Address + DataHeader);
    }
    else if (section.Name == Names::SummaryInfo)
    {
      m_Head->Summary = static_cast<std::uint32_t>(m_Address + DataHeader);
    }
    for (std::size_t start = 0; start < bytes.size(); start += description.PageSize)
    {
      const std::span<const std::byte> page =
          bytes.subspan(start, std::min<std::size_t>(description.PageSize, bytes.size() - start));
      // AutoCAD compresses every page whole, zeros after the section's last byte, and refuses one that decompresses
      // short of the page size.
      std::vector<std::byte> whole(page.begin(), page.end());
      if (section.Compressed)
      {
        whole.resize(description.PageSize, std::byte{0});
      }
      const Placement where{.Number = m_Number, .Address = m_Address, .Start = start};
      const Stored stored = WriteDataPage(where, description, whole);
      description.Pages.push_back(SectionPage{.Number = m_Number, .Size = stored.Size, .Start = start});
      Place(stored.Bytes);
    }
    m_Descriptions.push_back(std::move(description));
  }

  /**
   *  The section map, the page map and the head's record; the pages' bytes.
   */
  auto Finish() -> std::vector<std::byte>
  {
    const std::uint32_t sectionMap = m_Number + 2;
    const std::uint32_t pageMap = m_Number + 3;
    m_Number = sectionMap;
    // AutoCAD's section map runs from the highest id down after the nameless section.
    std::ranges::stable_sort(m_Descriptions.begin() + 1, m_Descriptions.end(), std::ranges::greater{},
                             &Description::Id);
    Place(WriteSystemPage(SectionMapType, WriteSectionMap(m_Descriptions)));
    m_Number = pageMap;
    const std::size_t at = m_Address;
    std::vector<std::byte> page;
    for (std::size_t guess = 0;;)
    {
      std::vector<Page> pages = m_Pages;
      pages.push_back(Page{.Number = static_cast<std::int32_t>(pageMap), .Address = at, .Size = guess});
      page = WriteSystemPage(PageMapType, WritePageMap(pages));
      if (page.size() == guess)
      {
        break;
      }
      guess = page.size();
    }
    Place(page);
    Record &record = m_Head->Record;
    record.PageCount = static_cast<std::uint32_t>(m_Pages.size());
    record.LastPageId = pageMap;
    record.LastPageEnd = m_Address;
    record.SecondHeader = m_Address;
    record.PageMapId = pageMap;
    record.PageMapAddress = at - Pages;
    record.SectionMapId = sectionMap;
    record.PageArraySize = pageMap;
    record.GapArraySize = 0;
    record.GapCount = 0;
    return std::move(m_Body);
  }

private:
  void Place(const std::vector<std::byte> &page)
  {
    m_Pages.push_back(Page{.Number = static_cast<std::int32_t>(m_Number), .Address = m_Address, .Size = page.size()});
    m_Body.insert(m_Body.end(), page.begin(), page.end());
    m_Address += page.size();
    ++m_Number;
  }

  Head *m_Head;
  std::vector<std::byte> m_Body{};
  std::vector<Page> m_Pages{};
  std::vector<Description> m_Descriptions{Description{.Size = 0,
                                                      .PageSize = UsualPage,
                                                      .Unknown1 = 1,
                                                      .Compressed = Packed,
                                                      .Id = 0,
                                                      .Encrypted = 0,
                                                      .Name = "",
                                                      .Pages = {}}};
  std::uint32_t m_Number{1};
  std::size_t m_Address{Pages};
};

/**
 *  The failure of a section whose pages start beyond its size or name one page twice, whose page size is beyond
 *  0x100000 or whose size its pages cannot hold; none when it fits. A hostile figure is refused before any buffer grows
 *  to it.
 */
auto Unfit(const Description &description) -> std::optional<Failure>
{
  if (description.Pages.empty())
  {
    return std::nullopt; // No page, no bytes: whatever the figures say, nothing grows to them.
  }
  std::set<std::uint32_t> seen;
  for (const SectionPage &one : description.Pages)
  {
    if (one.Start > description.Size)
    {
      return Failure{.Code = FailureCode::Damaged,
                     .What = std::format("section {} page {} starts beyond its size", description.Name, one.Number)};
    }
    if (!seen.insert(one.Number).second)
    {
      return Failure{.Code = FailureCode::Damaged,
                     .What = std::format("section {} page {} repeated", description.Name, one.Number)};
    }
  }
  if (description.PageSize > LargestPage)
  {
    return Failure{.Code = FailureCode::Damaged,
                   .What = std::format("section {} page size {} beyond 0x{:X}", description.Name, description.PageSize,
                                       LargestPage)};
  }
  if (description.Size > static_cast<std::uint64_t>(description.Pages.size()) * description.PageSize)
  {
    return Failure{.Code = FailureCode::Damaged,
                   .What = std::format("section {} size {} beyond its pages", description.Name, description.Size)};
  }
  return std::nullopt;
}
} // namespace

auto WriteLayout(Head &head, std::span<const Section> sections) -> std::vector<std::byte>
{
  Writer writer{head};
  for (const Section &section : sections)
  {
    writer.Add(section);
  }
  std::vector<std::byte> body = writer.Finish();
  std::vector<std::byte> file = WriteHead(head);
  file.insert(file.end(), body.begin(), body.end());
  const std::vector<std::byte> copy = WriteCopy(head);
  file.insert(file.end(), copy.begin(), copy.end());
  return file;
}

auto MakeMissingPage(std::uint32_t number) -> Failure
{
  return Failure{.Code = FailureCode::Damaged, .What = std::format("page {} missing", number)};
}

auto ReadLayout(std::span<const std::byte> file, std::vector<Issue> &issues) -> std::expected<Layout, Failure>
{
  const std::expected<Head, Failure> head = ReadHead(file, issues);
  if (!head)
  {
    return std::unexpected(head.error());
  }
  const Pager pager{file, issues};
  const std::expected<std::vector<Page>, Failure> pages = pager.PageMap(*head);
  if (!pages)
  {
    return std::unexpected(pages.error());
  }
  const std::expected<std::vector<Description>, Failure> descriptions = pager.SectionMap(*pages, *head);
  if (!descriptions)
  {
    return std::unexpected(descriptions.error());
  }
  Gatherer gatherer{pager, *pages};
  std::vector<std::pair<std::size_t, Section>> placed;
  for (const Description &description : *descriptions)
  {
    if (description.Name.empty() && description.Pages.empty())
    {
      continue;
    }
    if (const std::optional<Failure> unfit = Unfit(description))
    {
      return std::unexpected(*unfit);
    }
    std::expected<std::vector<std::byte>, Failure> bytes = gatherer.Of(description);
    if (!bytes)
    {
      return std::unexpected(bytes.error());
    }
    placed.emplace_back(StartOf(*pages, description), Section{.Name = SectionNameOf(description.Name),
                                                              .Bytes = std::move(*bytes),
                                                              .Compressed = description.Compressed == Packed,
                                                              .PageSize = description.PageSize,
                                                              .Id = description.Id});
  }
  std::ranges::stable_sort(placed, {}, &std::pair<std::size_t, Section>::first);
  Layout layout{.Head = *head, .Sections = {}};
  for (auto &[start, section] : placed)
  {
    layout.Sections.push_back(std::move(section));
  }
  return layout;
}
} // namespace DwgIO::File::Paged
