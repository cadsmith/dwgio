/***********************************************************************************************************************
 *  @file      source/objects/context.hpp
 *  @brief     What reading an object needs besides the file.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_CONTEXT_HPP
#define DWGIO_OBJECTS_CONTEXT_HPP

#include "dwgio/class.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/version.hpp"

#include <span>
#include <vector>

namespace DwgIO::Encoding
{
struct Scheme;
} // namespace DwgIO::Encoding

namespace DwgIO::Objects
{
struct FileContext
{
  Version Release{};
  const Encoding::Scheme *Text{nullptr}; /**< The file's text before R2007.  */
  std::span<const Class> Classes{};      /**< Names the types of 500 and up. */
  std::vector<Issue> *Issues{nullptr};   /**< Null drops them.               */
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_CONTEXT_HPP */
