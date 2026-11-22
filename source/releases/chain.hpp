/***********************************************************************************************************************
 *  @file      source/releases/chain.hpp
 *  @brief     An owner's list and chain of entities as another release keeps them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_CHAIN_HPP
#define DWGIO_RELEASES_CHAIN_HPP

#include "releases/lookup.hpp"
#include "releases/passage.hpp"

#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/handle.hpp"

#include <vector>

namespace DwgIO::Releases
{
/**
 *  What an owner keeps its entities by: the list from R2004, the chain's ends before, and the owner itself.
 */
struct Ownership
{
  std::vector<Handle> *List{};
  Handle *First{};
  Handle *Last{};
  Handle Owner{};
};

/**
 *  An entity's links as R2000 holds them, from its owner's list in the drawing: its neighbours there, null at the ends
 *  and when the owner lists it nowhere.
 */
void LinkEntity(Common &common, const Base &base, const Lookup &found);

/**
 *  An owner's list and chain across R2004: the list from the chain when upgrading, the chain's ends from the list when
 *  downgrading.
 *
 *  A chain cut before an owned object leaves that object out of the list, and the translation notes how many.
 */
void ConvertOwned(const Ownership &owning, const Passage &passage, const Crossings &crossing);
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_CHAIN_HPP */
