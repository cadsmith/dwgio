/***********************************************************************************************************************
 *  @file      dwgio/tables/block.hpp
 *  @brief     The BLOCK_RECORD: a block header that owns a block's entities.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TABLES_BLOCK_HPP
#define DWGIO_TABLES_BLOCK_HPP

#include "dwgio/api.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/tables/record.hpp"
#include "dwgio/vector.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace DwgIO::Tables
{
/**
 *  A BLOCK_RECORD, a block header in the file: owns a block's entities, and for model or paper space names its layout.
 */
struct DWGIO_API BlockRecord
{
  Tables::Record Record{};
  bool Anonymous{};  /**< The 1 bit of the BLOCK entity's group 70.           */
  bool Attributes{}; /**< Holds attribute definitions: the 2 bit of group 70. */
  bool Xref{};       /**< The 4 bit of group 70.                              */
  bool Overlaid{};   /**< The 8 bit of group 70.                              */
  bool Loaded{};     /**< From R2000.                                         */
  Vector Origin{};
  /**
   *  An external reference's; empty for the drawing's own blocks. Group 1 of the BLOCK entity.
   */
  std::string Path{};
  std::string Description{};        /**< Group 4 of the BLOCK entity, from R2000.                     */
  std::vector<std::byte> Preview{}; /**< As the file keeps it; empty for none. Group 310, from R2000. */
  int Units{};                      /**< From R2007, as $INSUNITS numbers them. Group 70.             */
  bool Explodable{true};            /**< From R2007. Group 280.                                       */
  int Scaling{};                    /**< From R2007: 0 any, 1 uniform. Group 281.                     */
  Handle Block{};
  Handle First{};                 /**< Up to R2000; null for a block with none or an external reference. */
  Handle Last{};                  /**< Up to R2000; null for a block with none or an external reference. */
  std::vector<Handle> Entities{}; /**< From R2004; First and Last before.                                */
  Handle EndBlk{};
  std::vector<Handle> Inserts{}; /**< From R2000. Group 331 in a 102 {BLKREFS brace.                               */
  Handle Layout{};               /**< For a model or paper space block; null for any other. Group 340, from R2000. */

  auto operator==(const BlockRecord &) const -> bool = default;
};
} // namespace DwgIO::Tables

#endif /* DWGIO_TABLES_BLOCK_HPP */
