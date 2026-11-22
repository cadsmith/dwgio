/***********************************************************************************************************************
 *  @file      source/acis/read.cpp
 *  @brief     An ACIS body, SAT or SAB, read into a model.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "acis/read.hpp"

#include "acis/grammar.hpp"
#include "acis/model.hpp"
#include "acis/reader.hpp"
#include "acis/sab/reader.hpp"
#include "acis/sat/reader.hpp"
#include "acis/source.hpp"
#include "octets.hpp"

#include "dwgio/entities/geometry.hpp"

#include <expected>
#include <format>
#include <memory>
#include <string>
#include <utility>

namespace DwgIO::Acis
{
namespace
{
/**
 *  The format of a body that is SAT.
 */
constexpr int Text = 1;
} // namespace

auto Read(const Entities::Acis &acis) -> std::expected<Model, std::string>
{
  const std::string data = TextOf(acis.Data);
  std::unique_ptr<Source> source;
  if (acis.Version == Text)
  {
    source = std::make_unique<Sat::Reader>(data);
  }
  else
  {
    source = std::make_unique<Sab::Reader>(data);
  }
  Model model;
  // A release before every gate reads no field, and a record of fields to its end would never end.
  if (!source->ReadHead(model.Head) || model.Head.Release < Always.Since)
  {
    return std::unexpected("head not read");
  }
  for (std::string name = source->Name(); !name.empty(); name = source->Name())
  {
    Record record{.Name = name, .Values = {}};
    FieldReader reading{*source, model.Head.Release, record};
    if (!Walk(reading, name))
    {
      return std::unexpected(std::format("record {} not known", name));
    }
    if (!reading.Failure().empty())
    {
      return std::unexpected(reading.Failure());
    }
    if (!source->End())
    {
      return std::unexpected(std::format("record {} holds more than known", name));
    }
    model.Records.push_back(std::move(record));
  }
  return model;
}
} // namespace DwgIO::Acis
