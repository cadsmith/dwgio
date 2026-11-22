/***********************************************************************************************************************
 *  @file      source/releases/store.hpp
 *  @brief     The ACIS bodies a save across R2013 moves into or out of the data store.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_STORE_HPP
#define DWGIO_RELEASES_STORE_HPP

#include "releases/remake.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/version.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace DwgIO::Releases
{
/**
 *  The drawing a save below R2013 writes from one of R2013 or later: each ACIS entity with its body taken out of the
 *  data store into the object, as SAB of the store's ACIS, and its bit for data in the store cleared, as AutoCAD's save
 *  writes it below R2013. Empty when the drawing has no store or the release has one.
 */
void TakeOutOfStore(Remake &remade, Version to);

/**
 *  The data store a save from below R2013 to R2013 or later writes: the body of each ACIS entity it writes, as AutoCAD
 *  writes it in that release, under the entity's handle.
 *
 *  Empty when the release has no store, the drawing already has one, or no entity has a body; an entity whose body
 *  cannot be written is left out by CollectLeftOut.
 */
[[nodiscard]] auto MakeStore(const Drawing &drawing, Version to, std::span<const Handle> leftOut)
    -> std::optional<std::vector<std::byte>>;

/**
 *  The data store of an R2018 drawing as an R2013 save writes it: each body of ACIS 22300 as AutoCAD writes it in
 *  R2013, in place, its records of 21800 alike and its size kept; one that cannot be is kept as it was, and noted.
 */
[[nodiscard]] auto LowerStore(std::span<const std::byte> store, std::vector<Issue> &issues) -> std::vector<std::byte>;
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_STORE_HPP */
