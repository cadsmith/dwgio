/***********************************************************************************************************************
 *  @file      source/tables/shapes/layer.hpp
 *  @brief     How a LAYER record lies in the file, its flags packed from R2000.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TABLES_SHAPES_LAYER_HPP
#define DWGIO_TABLES_SHAPES_LAYER_HPP

#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/nested.hpp"
#include "fields/since.hpp"
#include "objects/shape.hpp"
#include "tables/shapes/record.hpp"

#include "dwgio/tables/layer.hpp"
#include "dwgio/version.hpp"

#include <array>
#include <cstdint>
#include <string_view>
#include <utility>

namespace DwgIO::Tables::Shapes
{
constexpr unsigned FrozenBit = 1;

/**
 *  Set when the layer is off, though chapter 20.4.54 calls it "on".
 */
constexpr unsigned OffBit = 2;

constexpr unsigned FrozenNewBit = 4;

constexpr unsigned LockedBit = 8;

constexpr unsigned PlotBit = 16;

constexpr unsigned WidthShift = 5;

constexpr unsigned WidthMask = 0x1F;

inline auto ReadOff(Layer &layer, const Fields::Input &in) -> bool
{
  layer.On = !in.In->B();
  return !in.In->HasFailed();
}

inline auto WriteOff(const Layer &layer, const Fields::Output &to) -> bool
{
  to.To->B(!layer.On);
  return true;
}

/**
 *  From R2000 one short: the four R14 bits, the plot bit, and the lineweight code above them.
 */
inline auto ReadValues(Layer &layer, const Fields::Input &in) -> bool
{
  const auto values = static_cast<unsigned>(static_cast<std::uint16_t>(in.In->BS()));
  layer.Frozen = (values & FrozenBit) != 0;
  layer.On = (values & OffBit) == 0;
  layer.FrozenNew = (values & FrozenNewBit) != 0;
  layer.Locked = (values & LockedBit) != 0;
  layer.Plot = (values & PlotBit) != 0;
  layer.Lineweight = static_cast<Lineweight>((values >> WidthShift) & WidthMask);
  return !in.In->HasFailed();
}

inline auto WriteValues(const Layer &layer, const Fields::Output &to) -> bool
{
  const unsigned values =
      (static_cast<unsigned>(layer.Frozen) * FrozenBit) | (static_cast<unsigned>(!layer.On) * OffBit) |
      (static_cast<unsigned>(layer.FrozenNew) * FrozenNewBit) | (static_cast<unsigned>(layer.Locked) * LockedBit) |
      (static_cast<unsigned>(layer.Plot) * PlotBit) |
      ((std::to_underlying(layer.Lineweight) & WidthMask) << WidthShift);
  to.To->BS(static_cast<int>(values));
  return true;
}
} // namespace DwgIO::Tables::Shapes

namespace DwgIO::Objects
{
/**
 *  Chapter 20.4.54.
 *
 *  AutoCAD's files before R2013 do not hold the unnamed handle the chapter lists last.
 */
template <>
struct Shape<Tables::Layer>
{
  static constexpr std::array<std::uint16_t, 1> Types{51};
  static constexpr std::string_view Name = "LAYER";

  static auto Layout()
  {
    using Tables::Layer;
    using namespace Tables::Shapes;
    return Fields::Layout(Fields::Part(&Layer::Record, RecordLayout()),
                          Fields::Until(Version::R14, Fields::Bit(&Layer::Frozen), Fields::Custom(ReadOff, WriteOff),
                                        Fields::Bit(&Layer::FrozenNew), Fields::Bit(&Layer::Locked)),
                          Fields::Since(Version::R2000, Fields::Custom(ReadValues, WriteValues)),
                          Fields::CmColor(&Layer::Color),
                          Fields::Since(Version::R2000, Fields::Link(&Layer::Style, HardPointer)),
                          Fields::Since(Version::R2007, Fields::Link(&Layer::Material, HardPointer)),
                          Fields::Link(&Layer::Linetype, HardPointer),
                          Fields::Since(Version::R2013, Fields::Link(&Layer::Unknown1, HardPointer)));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_TABLES_SHAPES_LAYER_HPP */
