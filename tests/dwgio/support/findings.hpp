/***********************************************************************************************************************
 *  @file      tests/dwgio/support/findings.hpp
 *  @brief     What the audit tests read from a report, and the entities they slip into a drawing past Edit.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TESTS_SUPPORT_FINDINGS_HPP
#define DWGIO_TESTS_SUPPORT_FINDINGS_HPP

#include "dwgio/audit.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/handle.hpp"

#include <string>
#include <utility>
#include <vector>

namespace DwgIO
{
/**
 *  The text of each finding of a report, in its order.
 */
inline auto FindingTexts(const Audit::Report &report) -> std::vector<std::string>
{
  std::vector<std::string> texts;
  for (const Audit::Finding &finding : report.Findings)
  {
    texts.push_back(finding.What);
  }
  return texts;
}

/**
 *  Appends an entity under an owner by hand, past Edit, so that the owner does not list it: a sequence member, or an
 *  orphan. Gives its handle.
 */
template <class EntityT>
auto AppendEntity(Drawing &drawing, Handle owner) -> Handle
{
  EntityT entity;
  entity.Base.Self = Edit::MakeHandle(drawing);
  entity.Base.Owner = owner;
  entity.Common.Layer = drawing.Header.Clayer;
  entity.Common.Space = Space::Model;
  const Handle self = entity.Base.Self;
  drawing.Objects.emplace_back(std::move(entity));
  return self;
}
} // namespace DwgIO

#endif /* DWGIO_TESTS_SUPPORT_FINDINGS_HPP */
