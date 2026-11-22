/***********************************************************************************************************************
 *  @file      source/acis/read.hpp
 *  @brief     An ACIS body, SAT or SAB, read into a model.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ACIS_READ_HPP
#define DWGIO_ACIS_READ_HPP

#include "acis/model.hpp"

#include "dwgio/entities/geometry.hpp"

#include <expected>
#include <string>

namespace DwgIO::Acis
{
/**
 *  The body an object holds, SAT or SAB, read into the model; why not when a record is unknown or broken.
 */
[[nodiscard]] auto Read(const Entities::Acis &acis) -> std::expected<Model, std::string>;
} // namespace DwgIO::Acis

#endif /* DWGIO_ACIS_READ_HPP */
