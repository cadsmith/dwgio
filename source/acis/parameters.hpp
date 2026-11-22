/***********************************************************************************************************************
 *  @file      source/acis/parameters.hpp
 *  @brief     The parameters at an edge's vertices that bodies before ACIS 20800 lack.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ACIS_PARAMETERS_HPP
#define DWGIO_ACIS_PARAMETERS_HPP

#include "acis/model.hpp"

#include <string>

namespace DwgIO::Acis
{
/**
 *  The parameters ACIS 20800 gives an edge at its vertices, which a body of ACIS 1.06 or 4.00 lacks, found from the
 *  edge's curve as AutoCAD finds them: along a straight line the distance from its root, around an ellipse the angle
 *  from its major axis, a whole turn when the edge closes on itself, and along a B-spline the knot at the end the
 *  vertex lies on.
 *
 *  Empty when every edge has its parameters, else why one cannot be found.
 */
[[nodiscard]] auto Parameters(Model &model) -> std::string;
} // namespace DwgIO::Acis

#endif /* DWGIO_ACIS_PARAMETERS_HPP */
