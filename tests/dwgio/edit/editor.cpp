/***********************************************************************************************************************
 *  @file      tests/dwgio/edit/editor.cpp
 *  @brief     The editor: the same answers as the Edit functions, at the speed of a large drawing.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/edit/editor.hpp"

#include "support/acad.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/attdef.hpp"
#include "dwgio/entities/attrib.hpp"
#include "dwgio/entities/circle.hpp"
#include "dwgio/entities/insert.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/entities/seqend.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/placeholder.hpp"
#include "dwgio/save.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/tables/control.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/tables/table.hpp"
#include "dwgio/vector.hpp"
#include "dwgio/version.hpp"
#include "dwgio/view.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace DwgIO
{
namespace
{
auto LineAt(double x) -> Entities::Line
{
  Entities::Line line;
  line.Start = Vector{.X = x, .Y = 0, .Z = 0};
  line.End = Vector{.X = x, .Y = 1, .Z = 0};
  return line;
}

auto MakeAttrib(const char *tag) -> Entities::Attrib
{
  Entities::Attrib attribute;
  attribute.Tag = tag;
  attribute.Lettering.Value = tag;
  attribute.Lettering.Height = 1;
  return attribute;
}

/**
 *  The same edits through the functions of Edit, or through an editor when one is given.
 */
struct Edits
{
  Drawing *Drawn{};
  Edit::Editor *Editor{};

  [[nodiscard]] auto Add(const Object &entity, Space space = Space::Model) const -> std::expected<Handle, Failure>
  {
    return Editor != nullptr ? Editor->Add(entity, space) : Edit::Add(*Drawn, entity, space);
  }

  [[nodiscard]] auto Add(const Object &entity, std::string_view block) const -> std::expected<Handle, Failure>
  {
    return Editor != nullptr ? Editor->Add(entity, block) : Edit::Add(*Drawn, entity, block);
  }

  [[nodiscard]] auto Layer(const Tables::Layer &layer) const -> std::expected<Handle, Failure>
  {
    return Editor != nullptr ? Editor->Add(layer) : Edit::Add(*Drawn, layer);
  }

  [[nodiscard]] auto AppId(const Tables::AppId &appid) const -> std::expected<Handle, Failure>
  {
    return Editor != nullptr ? Editor->Add(appid) : Edit::Add(*Drawn, appid);
  }

  [[nodiscard]] auto AddAttribute(Handle insert, const Entities::Attrib &attribute) const
      -> std::expected<Handle, Failure>
  {
    return Editor != nullptr ? Editor->AddAttribute(insert, attribute) : Edit::AddAttribute(*Drawn, insert, attribute);
  }

  [[nodiscard]] auto AddBlock(std::string_view name) const -> std::expected<Handle, Failure>
  {
    return Editor != nullptr ? Editor->AddBlock(name) : Edit::AddBlock(*Drawn, name);
  }

  [[nodiscard]] auto Remove(Handle object) const -> std::expected<void, Failure>
  {
    return Editor != nullptr ? Editor->Remove(object) : Edit::Remove(*Drawn, object);
  }

  [[nodiscard]] auto MakeHandle() const -> Handle
  {
    return Editor != nullptr ? Editor->MakeHandle() : Edit::MakeHandle(*Drawn);
  }
};

auto Said(const std::expected<Handle, Failure> &result) -> std::string
{
  return result ? HexOf(*result) : result.error().What;
}

auto Said(const std::expected<void, Failure> &result) -> std::string
{
  return result ? "removed" : result.error().What;
}

/**
 *  A run of additions, removals and refusals, each answer in its order.
 */
auto Built(const Edits &edits) -> std::vector<std::string>
{
  std::vector<std::string> said;
  const Handle seed = edits.Drawn->Header.Handseed;
  const std::expected<Handle, Failure> door = edits.AddBlock("Door");
  said.push_back(Said(door));
  said.push_back(Said(edits.AddBlock("door")));
  Entities::AttDef number;
  number.Tag = "NUMBER";
  number.Lettering.Height = 1;
  said.push_back(Said(edits.Add(number, "Door")));
  said.push_back(Said(edits.Add(LineAt(0), "DOOR")));
  said.push_back(Said(edits.Add(LineAt(0), "Window")));
  Tables::Layer walls;
  walls.Record.Name = "Walls";
  walls.Linetype = edits.Drawn->Header.LTypeContinuous;
  said.push_back(Said(edits.Layer(walls)));
  walls.Record.Name = "WALLS";
  said.push_back(Said(edits.Layer(walls)));
  Tables::AppId application;
  application.Record.Name = "DWGIO";
  application.Record.Base.Self = Handle{.Value = seed.Value + 0x10};
  said.push_back(Said(edits.AppId(application)));
  Entities::Line far = LineAt(1);
  far.Base.Self = Handle{.Value = seed.Value + 0x200};
  said.push_back(Said(edits.Add(far)));
  Entities::Line taken = LineAt(1);
  taken.Base.Self = edits.Drawn->Header.Clayer;
  said.push_back(Said(edits.Add(taken)));
  said.push_back(HexOf(edits.MakeHandle()));
  for (int at = 0; at < 30; ++at)
  {
    said.push_back(Said(edits.Add(LineAt(at), at % 5 == 0 ? Space::Paper : Space::Model)));
  }
  std::vector<Handle> inserts;
  for (int at = 0; at < 8; ++at)
  {
    Entities::Insert insert;
    insert.Block = *door;
    insert.Position = Vector{.X = static_cast<double>(at), .Y = 2, .Z = 0};
    inserts.push_back(*edits.Add(insert));
    said.push_back(HexOf(inserts.back()));
    said.push_back(Said(edits.AddAttribute(inserts.back(), MakeAttrib("A"))));
    said.push_back(Said(edits.AddAttribute(inserts.back(), MakeAttrib("B"))));
  }
  said.push_back(Said(edits.AddAttribute(inserts.front(), MakeAttrib("C"))));
  said.push_back(Said(edits.AddAttribute(edits.Drawn->Header.Clayer, MakeAttrib("C"))));
  said.push_back(Said(edits.Add(Objects::Placeholder{})));
  Entities::Circle circle;
  circle.Radius = 1;
  said.push_back(Said(edits.Add(circle, Space::Paper)));
  said.push_back(Said(edits.Remove(inserts.at(3))));
  said.push_back(Said(edits.Remove(edits.Drawn->Header.Clayer)));
  said.push_back(Said(edits.AddAttribute(inserts.at(4), MakeAttrib("D"))));
  said.push_back(Said(edits.Add(LineAt(99))));
  return said;
}
} // namespace

TEST_CASE("edit/editor: edits through an Editor give what the free functions give in every release")
{
  for (const Version release : {Version::R14, Version::R2000, Version::R2004, Version::R2018})
  {
    CAPTURE(NameOf(release));
    Drawing walked = Create(release);
    Drawing indexed = walked;
    const std::vector<std::string> answers = Built(Edits{.Drawn = &walked, .Editor = nullptr});
    Edit::Editor editor{indexed};
    CHECK(Built(Edits{.Drawn = &indexed, .Editor = &editor}) == answers);
    CHECK(indexed == walked);
    CHECK(std::ranges::count(answers, "block door exists") == 1);
    CHECK(std::ranges::count(answers, "layer WALLS exists") == 1);
    CHECK(std::ranges::count(answers, "removed") == 1);
    CHECK(Audit::Check(indexed).IsClean());
    const std::expected<Saved, Failure> saved = Save(indexed);
    REQUIRE(saved);
    CHECK(saved->Issues.empty());
    const std::expected<Loaded, Failure> back = Load(saved->Bytes);
    REQUIRE(back);
    CHECK(back->Issues.empty());
    QueueAudit(saved->Bytes, std::format("dwgio-editor-{}.dwg", NameOf(release)));
  }
}

TEST_CASE("edit/editor: a large drawing built one entity at a time loads back clean")
{
  for (const Version release : {Version::R14, Version::R2018})
  {
    CAPTURE(NameOf(release));
    Drawing drawing = Create(release);
    Edit::Editor editor{drawing};
    const Handle tag = *editor.AddBlock("Tag");
    for (int at = 0; at < 5000; ++at)
    {
      REQUIRE(editor.Add(LineAt(at)));
    }
    for (int at = 0; at < 500; ++at)
    {
      Entities::Insert insert;
      insert.Block = tag;
      insert.Position = Vector{.X = static_cast<double>(at), .Y = -1, .Z = 0};
      const std::expected<Handle, Failure> placed = editor.Add(insert);
      REQUIRE(placed);
      REQUIRE(editor.AddAttribute(*placed, MakeAttrib("NUMBER")));
    }
    CHECK(View::Entities(drawing, *View::Block(drawing, "*Model_Space")).size() == 5500);
    CHECK(Audit::Check(drawing).IsClean());
    const std::expected<Saved, Failure> saved = Save(drawing);
    REQUIRE(saved);
    const std::expected<Loaded, Failure> back = Load(saved->Bytes);
    REQUIRE(back);
    CHECK(back->Issues.empty());
    CHECK(back->Drawing.Objects.size() == drawing.Objects.size());
    QueueAudit(saved->Bytes, std::format("dwgio-editor-large-{}.dwg", NameOf(release)));
  }
}

TEST_CASE("edit/editor: NewHandle through an Editor clears every handle whatever the seed says")
{
  Drawing drawing = Create();
  const Handle seed = drawing.Header.Handseed;
  drawing.Header.Handseed = Handle{.Value = 5};
  Edit::Editor editor{drawing};
  const Handle given = editor.MakeHandle();
  CHECK(given == seed);
  CHECK(drawing.Header.Handseed.Value == seed.Value + 1);
  CHECK(editor.Find(given) == nullptr);
  CHECK(editor.Find(Handle{}) == nullptr);
  CHECK(editor.Find(drawing.Header.Clayer) == Find(drawing, drawing.Header.Clayer));
  Edit::Editor moved{std::move(editor)};
  Edit::Editor other{drawing};
  other = std::move(moved);
  CHECK(other.MakeHandle().Value == seed.Value + 1);
}

TEST_CASE("edit/editor: a drawing without its tables refuses every addition through the Editor")
{
  Drawing bare;
  Edit::Editor editor{bare};
  CHECK(editor.Add(LineAt(0)).error().What == "model space missing");
  CHECK(editor.Add(LineAt(0), "Door").error().What == "block Door missing");
  CHECK(editor.AddBlock("Door").error().What == "block table missing");
  CHECK(editor.Add(Tables::Layer{}).error().What == "layer table missing");
  CHECK(editor.Remove(Handle{.Value = 1}).error().What == "handle 1 missing");
  CHECK(bare.Objects.empty());
}

TEST_CASE("edit/editor: the Editor's index follows shared or missing handles and placed attributes")
{
  Drawing drawing = Create();
  const Handle door = *Edit::AddBlock(drawing, "Door");
  Entities::Insert placed;
  placed.Block = door;
  const Handle insert = *Edit::Add(drawing, placed);
  REQUIRE(Edit::AddAttribute(drawing, insert, MakeAttrib("A")));
  // The layer control moved last, a second layer control after it, and records and objects no addition would make.
  const auto layers = std::ranges::find_if(
      drawing.Objects, [](const Object &object) -> bool
      { return Holds<Tables::Control>(object) && Get<Tables::Control>(object).Table == Tables::Table::Layer; });
  std::rotate(layers, layers + 1, drawing.Objects.end());
  Get<Tables::Control>(drawing.Objects.back()).Entries.push_back(drawing.Header.DictionaryNamedObjects);
  GetIf<Tables::Control>(Find(drawing, drawing.Header.BlockControlObject))
      ->Entries.push_back(drawing.Header.DictionaryNamedObjects);
  CHECK(Edit::Add(drawing, LineAt(0), "Window").error().What == "block Window missing");
  Tables::Control second;
  second.Table = Tables::Table::Layer;
  second.Base.Self = Handle{.Value = 0x700};
  drawing.Objects.emplace_back(second);
  drawing.Objects.emplace_back(Objects::Placeholder{});
  const Handle shared{.Value = 0x701};
  Objects::Placeholder twin;
  twin.Base.Self = shared;
  drawing.Objects.emplace_back(twin);
  drawing.Objects.emplace_back(twin);
  drawing.Header.Handseed = Handle{.Value = 0x702};
  const std::size_t first = drawing.Objects.size() - 2;
  Edit::Editor editor{drawing};
  REQUIRE(editor.AddAttribute(insert, MakeAttrib("B")));
  REQUIRE(editor.AddAttribute(insert, MakeAttrib("C")));
  CHECK(editor.Find(shared) == &drawing.Objects[first + 2]);
  CHECK(Holds<Entities::SeqEnd>(*editor.Find(Get<Entities::Insert>(*editor.Find(insert)).SeqEnd)));
  CHECK(Holds<Entities::SeqEnd>(drawing.Objects[first - 2]));
  Tables::Layer walls;
  walls.Record.Name = "Walls";
  const std::expected<Handle, Failure> listed = editor.Add(walls);
  REQUIRE(listed);
  CHECK(Get<Tables::Control>(drawing.Objects[first - 1]).Entries.back() == *listed);
  CHECK(Get<Tables::Control>(drawing.Objects[first]).Entries.empty());
  walls.Record.Name = "0";
  CHECK(editor.Add(walls).error().What == "layer 0 exists");
  walls.Record.Name = "walls";
  CHECK(editor.Add(walls).error().What == "layer walls exists");
  CHECK(editor.Add(LineAt(0), "Window").error().What == "block Window missing");
  CHECK(editor.Find(*listed) == Find(drawing, *listed));
}

TEST_CASE("edit/editor: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO
