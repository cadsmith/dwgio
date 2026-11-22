/***********************************************************************************************************************
 *  @file      source/acis/write.cpp
 *  @brief     An ACIS model written as SAT or SAB of a release.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "acis/write.hpp"

#include "acis/grammar.hpp"
#include "acis/model.hpp"
#include "acis/parameters.hpp"
#include "acis/remake.hpp"
#include "acis/sab/writer.hpp"
#include "acis/sat/writer.hpp"
#include "acis/sink.hpp"
#include "acis/writer.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <expected>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace DwgIO::Acis
{
namespace
{
/**
 *  The flags AutoCAD's SAB of each release carries; SAT carries none.
 */
constexpr std::array<std::pair<int, int>, 5> Flagged{
    {{Releases::R2004, 0}, {Releases::R2007, 26}, {Releases::R2010, 24}, {Releases::R2013, 12}, {Releases::R2018, 4}}};

/**
 *  What AutoCAD 2026 names itself and its modeler with.
 */
constexpr std::string_view Product = "Autodesk AutoCAD";
constexpr std::string_view Modeler = "ASM 231.0.0.65535 OSX";
/**
 *  The record a body of ACIS 20800 and later opens with.
 */
constexpr std::string_view Header = "asmheader";
constexpr std::string_view HeaderVersion = "231.0.0.65535";

/**
 *  The bodies AutoCAD's SAT and SAB count.
 */
constexpr int SatBodies = 1;
constexpr int SabBodies = 2;

/**
 *  The time now as ctime writes it, the date a body is saved with.
 */
auto Now() -> std::string
{
  return std::format("{:%a %b %e %H:%M:%S %Y}",
                     std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));
}

/**
 *  The head a release is written with: the model's product, modeler, date and tolerances, AutoCAD's own where it has
 *  none, and the release's counts and flags.
 */
auto MakeHead(const Head &model, int release) -> Head
{
  Head head = model;
  head.Release = release;
  head.Records = 0;
  head.Bodies = release >= Releases::R2004 ? SabBodies : SatBodies;
  head.Flags = 0;
  for (const auto &[flagged, flags] : Flagged)
  {
    head.Flags = flagged == release ? flags : head.Flags;
  }
  if (!model.Named)
  {
    head.Named = true;
    head.Product = Product;
    head.Modeler = Modeler;
    head.Date = Now();
  }
  return head;
}

/**
 *  The header record a body of ACIS 20800 and later opens with, for one that lacks it.
 */
auto Opener() -> Record
{
  return Record{.Name = std::string{Header},
                .Values = {Value{.Kind = Kind::Pointer, .Integer = -1}, Value{.Kind = Kind::Long, .Integer = -1},
                           Value{.Kind = Kind::Text, .Text = std::string{HeaderVersion}}}};
}
} // namespace

auto Write(Model model, int release, bool remade) -> std::expected<std::string, std::string>
{
  if (remade)
  {
    Remade(model);
  }
  const bool binary = release >= Releases::R2004;
  if (binary && model.Head.Release < Releases::R2004)
  {
    if (std::string missing = Parameters(model); !missing.empty())
    {
      return std::unexpected(std::move(missing));
    }
  }
  const bool opened = !model.Records.empty() && model.Records.front().Name == Header;
  if (binary && !opened)
  {
    model.Records.insert(model.Records.begin(), Opener());
  }
  const std::int64_t shift = (binary ? 1 : 0) - (opened ? 1 : 0);
  std::unique_ptr<Sink> sink;
  if (binary)
  {
    sink = std::make_unique<Sab::Writer>();
  }
  else
  {
    sink = std::make_unique<Sat::Writer>();
  }
  sink->WriteHead(MakeHead(model.Head, release));
  for (const Record &record : model.Records)
  {
    if (!binary && record.Name == Header)
    {
      continue;
    }
    sink->Name(record.Name);
    FieldWriter writing{*sink, Plan{.Release = release, .Shift = shift}, record};
    static_cast<void>(Walk(writing, record.Name));
    if (!writing.Failure().empty())
    {
      return std::unexpected(writing.Failure());
    }
    sink->End();
  }
  return sink->Finish();
}
} // namespace DwgIO::Acis
