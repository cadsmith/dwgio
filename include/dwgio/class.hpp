/***********************************************************************************************************************
 *  @file      dwgio/class.hpp
 *  @brief     An entry of the classes section, which names the object types of 500 and up.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_CLASS_HPP
#define DWGIO_CLASS_HPP

#include "dwgio/api.hpp"

#include <string>

namespace DwgIO
{
/**
 *  A class of the drawing's classes section: what an object type of 500 and up is.
 */
struct DWGIO_API Class
{
  int Number{};              /**< 500 and up.                                                       */
  int ProxyFlags{};          /**< What a program without the class may do to its objects.           */
  std::string Application{}; /**< Such as "ObjectDBX Classes".                                      */
  std::string CppName{};     /**< Such as "AcDbDictionaryWithDefault".                              */
  std::string DxfName{};     /**< Such as "ACDBDICTIONARYWDFLT".                                    */
  bool WasZombie{};          /**< A proxy of R13.                                                   */
  int ItemClass{};           /**< 0x1F2 for a class of entities, 0x1F3 for a class of objects.      */
  int Instances{};           /**< From R2004.                                                       */
  int DwgVersion{};          /**< From R2004.                                                       */
  int Maintenance{};         /**< From R2004.                                                       */
  int Unknown1{};            /**< Unnamed in the specification; 0 as AutoCAD writes it. From R2004. */
  int Unknown2{};            /**< Unnamed in the specification; 0 as AutoCAD writes it. From R2004. */

  auto operator==(const Class &) const -> bool = default;
};
} // namespace DwgIO

#endif /* DWGIO_CLASS_HPP */
