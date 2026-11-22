/***********************************************************************************************************************
 *  @file      source/acis/model.hpp
 *  @brief     The model ACIS bodies are read into and written from.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ACIS_MODEL_HPP
#define DWGIO_ACIS_MODEL_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/**
 *  One model for SAT text and SAB binary alike, written as any release of ACIS AutoCAD writes.
 */
namespace DwgIO::Acis
{
/**
 *  The ACIS releases AutoCAD writes: 1.06 in R14, 4.00 in R2000, then 20800 in R2004, 21200 in R2007, 21500 in R2010,
 *  21800 in R2013 and 22300 in R2018, which keeps an R2013 body as it was.
 */
namespace Releases
{
constexpr int R14 = 106;
constexpr int R2000 = 400;
constexpr int R2004 = 20800;
constexpr int R2007 = 21200;
constexpr int R2010 = 21500;
constexpr int R2013 = 21800;
constexpr int R2018 = 22300;
} // namespace Releases

/**
 *  A field is in every release from `Since` on: no field AutoCAD's files hold went away again.
 */
struct Gate
{
  int Since{};

  [[nodiscard]] constexpr auto Has(int release) const -> bool { return release >= Since; }
};

constexpr Gate Always{};

enum class Kind : std::uint8_t
{
  Pointer, /**< The index of another record, or -1. */
  Long,
  Real,
  Position,  /**< A point: three doubles.     */
  Direction, /**< A direction: three doubles. */
  Text,
  Logical, /**< A logical, which SAT writes as one of two words.        */
  Choice,  /**< An enumeration, which SAT writes as a word.             */
  Bound,   /**< An end of an interval: infinite, or finite at a double. */
  Word,    /**< A word: a subtype's name, a curve's form, a null.       */
  Huge,    /**< A 64-bit integer.                                       */
};

/**
 *  One value of a record, of its kind; Given false when the source had no such field and the value is its default.
 */
struct Value
{
  Acis::Kind Kind{};
  /**
   *  A pointer, long, logical, enumeration or 64-bit integer; for a bound, 1 when finite.
   */
  std::int64_t Integer{};
  /**
   *  A double in the first, a position or a direction in all three; a finite bound's double in the first.
   */
  std::array<double, 3> Reals{};
  std::string Text{};
  bool Given{true};

  auto operator==(const Value &) const -> bool = default;
};

/**
 *  A record: its name, such as `face` or `persubent-acadSolidHistory-attrib`, and its values in the order of the fields
 *  of every release, those a release lacks among them.
 */
struct Record
{
  std::string Name{}; /**< Its parts joined by hyphens. */
  std::vector<Value> Values{};

  auto operator==(const Record &) const -> bool = default;
};

/**
 *  What opens a body: the release, the counts, the flags, the product that wrote it and the tolerances.
 */
struct Head
{
  int Release{};
  int Records{}; /**< What SAT gives, 0 in SAB.                     */
  int Bodies{};  /**< 1 in SAT and 2 in SAB as AutoCAD writes them. */
  int Flags{};
  bool Named{}; /**< Whether the source gave the product, modeler and date: R14's SAT does not. */
  std::string Product{};
  std::string Modeler{};
  std::string Date{};                               /**< As ctime writes it.                                */
  std::array<double, 3> Tolerances{1, 1e-6, 1e-10}; /**< The units, the absolute and the normal tolerances. */
};

struct Model
{
  Acis::Head Head{};
  std::vector<Record> Records{};
};

/**
 *  The record a pointer names, when it has that name; else null.
 */
[[nodiscard]] inline auto FindRecord(const Model &model, std::int64_t pointer, std::string_view name) -> const Record *
{
  if (pointer < 0 || std::cmp_greater_equal(pointer, model.Records.size()))
  {
    return nullptr;
  }
  const Record &record = model.Records[static_cast<std::size_t>(pointer)];
  return record.Name == name ? &record : nullptr;
}

/**
 *  The two words SAT writes a logical as, false first; R14's SAT writes some logicals as 0 and 1 instead.
 */
struct Words
{
  std::string_view False;
  std::string_view True;
  bool Numeric{};
};

/**
 *  The words SAT writes an enumeration's values as, by value, empty for a value AutoCAD's files never hold; R14's SAT
 *  writes some enumerations as numbers instead.
 */
struct Table
{
  std::span<const std::string_view> Names;
  bool Numeric{};
};
} // namespace DwgIO::Acis

#endif /* DWGIO_ACIS_MODEL_HPP */
