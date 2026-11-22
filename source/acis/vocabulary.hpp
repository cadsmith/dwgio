/***********************************************************************************************************************
 *  @file      source/acis/vocabulary.hpp
 *  @brief     The words SAT writes logicals and enumerations with.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ACIS_VOCABULARY_HPP
#define DWGIO_ACIS_VOCABULARY_HPP

#include "acis/model.hpp"

#include <array>
#include <string_view>

/**
 *  The words AutoCAD's SAT writes logicals and enumerations with.
 */
namespace DwgIO::Acis::Vocabulary
{
/**
 *  A sense, which R14 writes as a number but on a face.
 */
inline constexpr Words Sense{.False = "forward", .True = "reversed", .Numeric = true};
/**
 *  A face's sense, a word in every release.
 */
inline constexpr Words FaceSense{.False = "forward", .True = "reversed", .Numeric = false};
/**
 *  Whether a face is single or double sided.
 */
inline constexpr Words Sides{.False = "single", .True = "double", .Numeric = false};
/**
 *  Which side of a double-sided face is inside.
 */
inline constexpr Words Containment{.False = "out", .True = "in", .Numeric = false};
/**
 *  A plane's or sphere's sense.
 */
inline constexpr Words SurfaceSense{.False = "forward_v", .True = "reversed_v", .Numeric = true};
/**
 *  A transform's three properties.
 */
inline constexpr Words Rotation{.False = "no_rotate", .True = "rotate", .Numeric = true};
inline constexpr Words Reflection{.False = "no_reflect", .True = "reflect", .Numeric = true};
inline constexpr Words Shearing{.False = "no_shear", .True = "shear", .Numeric = true};
/**
 *  A logical of ACIS 21500 in a spline surface, which no SAT AutoCAD writes holds.
 */
inline constexpr Words Unnamed{.False = "F", .True = "T", .Numeric = false};

/**
 *  A spline's closure along a direction.
 */
inline constexpr std::array<std::string_view, 3> ClosureNames{"open", "closed", "periodic"};
inline constexpr Table Closure{.Names = ClosureNames, .Numeric = false};
/**
 *  A spline surface's singularity along a direction.
 */
inline constexpr std::array<std::string_view, 1> SingularityNames{"none"};
inline constexpr Table Singularity{.Names = SingularityNames, .Numeric = false};
/**
 *  What becomes of an attribute when its owner splits, merges, is transformed or is copied.
 */
inline constexpr std::array<std::string_view, 3> SplitNames{"", "", "copy"};
inline constexpr Table Split{.Names = SplitNames, .Numeric = true};
inline constexpr std::array<std::string_view, 5> MergeNames{"", "", "", "", "keep_all"};
inline constexpr Table Merge{.Names = MergeNames, .Numeric = true};
inline constexpr std::array<std::string_view, 2> TransformNames{"", "ignore"};
inline constexpr Table Transformed{.Names = TransformNames, .Numeric = true};
inline constexpr Table Copied{.Names = SplitNames, .Numeric = true};
/**
 *  Enumerations of ACIS 20800 and later, which no SAT AutoCAD writes holds: a spline's approximation and extension.
 */
inline constexpr std::array<std::string_view, 1> FullNames{"full"};
inline constexpr Table Approximation{.Names = FullNames, .Numeric = false};
inline constexpr std::array<std::string_view, 1> UnextendedNames{"unextended"};
inline constexpr Table Extension{.Names = UnextendedNames, .Numeric = false};
} // namespace DwgIO::Acis::Vocabulary

#endif /* DWGIO_ACIS_VOCABULARY_HPP */
