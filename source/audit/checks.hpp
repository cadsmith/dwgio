/***********************************************************************************************************************
 *  @file      source/audit/checks.hpp
 *  @brief     The audit's checks, in the order a repair runs them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_AUDIT_CHECKS_HPP
#define DWGIO_AUDIT_CHECKS_HPP

#include "dwgio/audit.hpp"
#include "dwgio/drawing.hpp"

namespace DwgIO::Audit
{
/**
 *  Appends a finding as fixed, since every check runs as a repair, and gives it back.
 */
auto NoteFixed(Report &report, Finding finding) -> Finding &;
/**
 *  Appends a finding as it is, for what no check can fix.
 */
auto NoteUnfixed(Report &report, Finding finding) -> Finding &;

/**
 *  The checks, in the order Repair runs them; each fixes what it finds and may assume the earlier ones hold.
 */
void Handles(Drawing &drawing, Report &report);
void Roots(Drawing &drawing, Report &report);
void Names(Drawing &drawing, Report &report);
void References(Drawing &drawing, Report &report);
void Chains(Drawing &drawing, Report &report);
void Extras(Drawing &drawing, Report &report);
} // namespace DwgIO::Audit

#endif /* DWGIO_AUDIT_CHECKS_HPP */
