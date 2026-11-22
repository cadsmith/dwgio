/***********************************************************************************************************************
 *  @file      tests/fuzz/bits.cpp
 *  @brief     Fuzzes the bit reader with every bit code.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/reader.hpp"
#include "encoding/encoding.hpp"

#include "dwgio/version.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

/**
 *  Reads every bit code in turn from whatever libFuzzer hands over, until the reader fails.
 */
extern "C" auto LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size) -> int
{
  DwgIO::Bits::Reader in{std::as_bytes(std::span{data, size})};
  const DwgIO::Encoding::Scheme utf8 = DwgIO::Encoding::Utf8();
  while (!in.HasFailed())
  {
    static_cast<void>(in.B());
    static_cast<void>(in.BB());
    static_cast<void>(in.B3());
    static_cast<void>(in.RC());
    static_cast<void>(in.RS());
    static_cast<void>(in.RL());
    static_cast<void>(in.RLL());
    static_cast<void>(in.RD());
    static_cast<void>(in.BS());
    static_cast<void>(in.BL());
    static_cast<void>(in.BLL());
    static_cast<void>(in.BD());
    static_cast<void>(in.MC());
    static_cast<void>(in.UMC());
    static_cast<void>(in.MS());
    static_cast<void>(in.H());
    static_cast<void>(in.DD(1.5));
    static_cast<void>(in.BT(DwgIO::Version::R2000));
    static_cast<void>(in.BE(DwgIO::Version::R2000));
    static_cast<void>(in.Point2());
    static_cast<void>(in.Point3());
    static_cast<void>(in.CMC(DwgIO::Version::R2004));
    static_cast<void>(in.ENC(DwgIO::Version::R2004));
    static_cast<void>(in.OT(DwgIO::Version::R2010));
    static_cast<void>(in.T(utf8));
    static_cast<void>(in.TU());
    static_cast<void>(in.TV(DwgIO::Version::R2007, utf8));
    static_cast<void>(in.Bytes(3));
  }
  return 0;
}
