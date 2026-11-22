/***********************************************************************************************************************
 *  @file      source/audit/audit.cpp
 *  @brief     Checking and repairing a drawing as AutoCAD's AUDIT would.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/audit.hpp"

#include "audit/checks.hpp"

#include "dwgio/drawing.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace DwgIO::Audit
{
auto Report::IsClean() const noexcept -> bool
{
  return std::ranges::none_of(Findings, [](const Finding &finding) -> bool
                              { return finding.Level == Severity::Error && !finding.Fixed; });
}

auto Report::Errors() const noexcept -> std::size_t
{
  return static_cast<std::size_t>(
      std::ranges::count_if(Findings, [](const Finding &finding) -> bool { return finding.Level == Severity::Error; }));
}

auto Report::Warnings() const noexcept -> std::size_t
{
  return Findings.size() - Errors();
}

auto NoteFixed(Report &report, Finding finding) -> Finding &
{
  finding.Fixed = true;
  return report.Findings.emplace_back(std::move(finding));
}

auto NoteUnfixed(Report &report, Finding finding) -> Finding &
{
  return report.Findings.emplace_back(std::move(finding));
}

auto Check(const Drawing &drawing) -> Report
{
  Drawing copy = drawing;
  Report report = Repair(copy);
  for (Finding &finding : report.Findings)
  {
    finding.Fixed = false;
  }
  return report;
}

auto Repair(Drawing &drawing) -> Report
{
  Report report;
  Handles(drawing, report);
  Roots(drawing, report);
  Names(drawing, report);
  References(drawing, report);
  Chains(drawing, report);
  Extras(drawing, report);
  return report;
}
} // namespace DwgIO::Audit
