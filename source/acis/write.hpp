/***********************************************************************************************************************
 *  @file      source/acis/write.hpp
 *  @brief     An ACIS model written as SAT or SAB of a release.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ACIS_WRITE_HPP
#define DWGIO_ACIS_WRITE_HPP

#include "acis/model.hpp"

#include <expected>
#include <string>

namespace DwgIO::Acis
{
/**
 *  The model written as that ACIS release as AutoCAD writes it: SAT before ACIS 20800, SAB from it, the head of the
 *  release, from ACIS 20800 the header record first, each record with the fields the release has, those the model lacks
 *  at the values AutoCAD gives them, as it gives them to a body it remakes on reading when remade; why not when a value
 *  has no way to be written.
 */
[[nodiscard]] auto Write(Model model, int release, bool remade) -> std::expected<std::string, std::string>;
} // namespace DwgIO::Acis

#endif /* DWGIO_ACIS_WRITE_HPP */
