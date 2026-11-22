/***********************************************************************************************************************
 *  @file      source/acis/subtype.hpp
 *  @brief     The spline subtypes of intcurves, parameter curves and spline surfaces.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ACIS_SUBTYPE_HPP
#define DWGIO_ACIS_SUBTYPE_HPP

#include "acis/fields.hpp"

/**
 *  The subtypes AutoCAD's bodies hold between braces: the spline data of an intcurve, of a parameter curve and of a
 *  spline surface, each with its B-spline and what the releases added after it.
 */
namespace DwgIO::Acis::Subtype
{
/**
 *  An intcurve's subtype: `exactcur`, `exact_int_cur` from ACIS 21200, with its curve and null surfaces and parameter
 *  curves; or a reference to an earlier subtype.
 */
void Curve(const Fields &fields);

/**
 *  A parameter curve's subtype: `exppc`, `exp_par_cur` from ACIS 21200, with its curve in two dimensions and the spline
 *  surface it lies on.
 */
void Parameter(const Fields &fields);

/**
 *  A spline surface's subtype: `exactsur`, `exact_spl_sur` from ACIS 21200, with its surface; or a reference to an
 *  earlier subtype.
 */
void Surface(const Fields &fields);
} // namespace DwgIO::Acis::Subtype

#endif /* DWGIO_ACIS_SUBTYPE_HPP */
