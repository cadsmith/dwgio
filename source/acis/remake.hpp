/***********************************************************************************************************************
 *  @file      source/acis/remake.hpp
 *  @brief     What AutoCAD gives a body it remakes on reading.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ACIS_REMAKE_HPP
#define DWGIO_ACIS_REMAKE_HPP

#include "acis/model.hpp"

namespace DwgIO::Acis
{
/**
 *  What AutoCAD gives a body it remakes on reading, as it does a procedural surface's, where the body's release lacks
 *  it: a vertex's long of ACIS 21200, 0 when the vertex starts its edge and 1 when it ends it, and an edge's convexity
 *  of ACIS 20800, tangent along a seam, whose two coedges lie on one face.
 *
 *  A body AutoCAD reads as it is keeps the 2 and the unknown it gives a vertex and an edge it knows nothing of.
 */
void Remade(Model &model);
} // namespace DwgIO::Acis

#endif /* DWGIO_ACIS_REMAKE_HPP */
