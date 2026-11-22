/***********************************************************************************************************************
 *  @file      source/acis/convert.cpp
 *  @brief     An ACIS body converted to the release a DWG release holds.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "acis/convert.hpp"

#include "acis/model.hpp"
#include "acis/read.hpp"
#include "acis/write.hpp"
#include "octets.hpp"

#include "dwgio/entities/geometry.hpp"
#include "dwgio/version.hpp"

#include <algorithm>
#include <array>
#include <expected>
#include <format>
#include <string>
#include <utility>

namespace DwgIO::Acis
{
namespace
{
/**
 *  The ACIS release of each DWG release, oldest first.
 */
constexpr std::array<int, 7> Released{Releases::R14,   Releases::R2000, Releases::R2004, Releases::R2007,
                                      Releases::R2010, Releases::R2013, Releases::R2018};

/**
 *  The formats of a body an object holds.
 */
constexpr int Sat = 1;
constexpr int Sab = 2;
} // namespace

auto ReleaseOf(Version release) -> int
{
  return Released.at(static_cast<std::size_t>(release));
}

auto Convert(const Entities::Acis &acis, Version to, bool remade) -> std::expected<Entities::Acis, std::string>
{
  if (acis.Version != Sat && acis.Version != Sab)
  {
    return acis;
  }
  std::expected<Model, std::string> model = Read(acis);
  if (!model)
  {
    return std::unexpected(std::move(model.error()));
  }
  const int from = model->Head.Release;
  const int release = ReleaseOf(to);
  if (from == release || (from == Releases::R2013 && release == Releases::R2018))
  {
    return acis;
  }
  if (!std::ranges::contains(Released, from))
  {
    return std::unexpected(std::format("ACIS {} not known", from));
  }
  std::expected<std::string, std::string> written = Write(std::move(*model), release, remade);
  if (!written)
  {
    return std::unexpected(std::move(written.error()));
  }
  const bool binary = release >= Releases::R2004;
  return Entities::Acis{.Version = binary ? Sab : Sat, .Textual = !binary, .Data = BytesOf(*written)};
}
} // namespace DwgIO::Acis
