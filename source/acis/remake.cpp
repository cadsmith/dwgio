/***********************************************************************************************************************
 *  @file      source/acis/remake.cpp
 *  @brief     What AutoCAD gives a body it remakes on reading.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "acis/remake.hpp"

#include "acis/grammar.hpp"
#include "acis/model.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

namespace DwgIO::Acis
{
namespace
{
/**
 *  Where a vertex's edge and long lie; an edge's start vertex, coedge and convexity; a coedge's partner and loop; a
 *  loop's face.
 */
constexpr std::size_t VertexEdge = Prefix::Entity;
constexpr std::size_t VertexLong = Prefix::Entity + 1;
constexpr std::size_t EdgeStart = Prefix::Entity;
constexpr std::size_t EdgeCoedge = Prefix::Entity + 4;
constexpr std::size_t EdgeConvexity = Prefix::Entity + 7;
constexpr std::size_t CoedgePartner = Prefix::Entity + 2;
constexpr std::size_t CoedgeLoop = Prefix::Entity + 5;
constexpr std::size_t LoopFace = Prefix::Entity + 2;

/**
 *  The face a coedge's loop belongs to, or -1.
 */
auto Face(const Model &model, std::int64_t coedge) -> std::int64_t
{
  const Record *record = FindRecord(model, coedge, "coedge");
  const Record *loop = record == nullptr ? nullptr : FindRecord(model, record->Values[CoedgeLoop].Integer, "loop");
  return loop == nullptr ? -1 : loop->Values[LoopFace].Integer;
}

/**
 *  Whether an edge is a seam: its coedge and that coedge's partner on one face.
 */
auto Seam(const Model &model, const Record &edge) -> bool
{
  const std::int64_t coedge = edge.Values[EdgeCoedge].Integer;
  const Record *record = FindRecord(model, coedge, "coedge");
  const std::int64_t face = Face(model, coedge);
  return record != nullptr && face >= 0 && Face(model, record->Values[CoedgePartner].Integer) == face;
}
} // namespace

void Remade(Model &model)
{
  for (std::size_t at = 0; at < model.Records.size(); ++at)
  {
    Record &record = model.Records[at];
    if (record.Name == "vertex" && !record.Values[VertexLong].Given)
    {
      const Record *edge = FindRecord(model, record.Values[VertexEdge].Integer, "edge");
      record.Values[VertexLong].Integer =
          edge != nullptr && std::cmp_equal(edge->Values[EdgeStart].Integer, at) ? 0 : 1;
    }
    if (record.Name == "edge" && !record.Values[EdgeConvexity].Given && Seam(model, record))
    {
      record.Values[EdgeConvexity].Text = "tangent";
    }
  }
}
} // namespace DwgIO::Acis
