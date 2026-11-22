/***********************************************************************************************************************
 *  @file      tests/dwgio/remove/objects.cpp
 *  @brief     Removing objects: untyped reactors, named objects, page setups and handles the drawing lacks.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "support/acad.hpp"
#include "support/fixtures.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/block.hpp"
#include "dwgio/entities/vertex.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/layout.hpp"
#include "dwgio/objects/mlinestyle.hpp"
#include "dwgio/objects/xrecord.hpp"
#include "dwgio/save.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/style.hpp"
#include "dwgio/tables/table.hpp"
#include "dwgio/tables/view.hpp"
#include "dwgio/unknown.hpp"
#include "dwgio/view.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <expected>
#include <format>
#include <string>
#include <vector>

namespace DwgIO
{
namespace
{
constexpr Handle Nowhere{.Value = 0x999};

auto Model(const Drawing &drawing) -> const Tables::BlockRecord &
{
  return *View::Block(drawing, "*Model_Space");
}

/**
 *  The drawing's entities in model space, as drawn.
 */
auto Drawn(const Drawing &drawing) -> std::vector<Handle>
{
  std::vector<Handle> handles;
  for (const Object *object : View::Entities(drawing, Model(drawing)))
  {
    handles.push_back(BaseOf(*object).Self);
  }
  return handles;
}

auto CheckRefusal(Drawing &drawing, Handle object) -> std::string
{
  const Drawing before = drawing;
  const std::expected<void, Failure> removed = Edit::Remove(drawing, object);
  REQUIRE(!removed);
  CHECK(drawing == before);
  const Failure &failure = removed.error();
  const FailureCode code = failure.What.ends_with(" in use")    ? FailureCode::InUse
                           : failure.What.ends_with(" missing") ? FailureCode::Missing
                                                                : FailureCode::Refused;
  CHECK(failure.Code == code);
  CHECK_FALSE(failure.Offset);
  return failure.What;
}

/**
 *  The drawing checks clean, saves and loads back with no issue, and AutoCAD's audit finds nothing when asked.
 */
void Sound(const Drawing &drawing, const std::string &name)
{
  CAPTURE(name);
  for (const Audit::Finding &finding : Audit::Check(drawing).Findings)
  {
    CAPTURE(finding.What);
    CHECK(false);
  }
  const std::expected<Saved, Failure> saved = Save(drawing);
  REQUIRE(saved);
  for (const Issue &issue : saved->Issues)
  {
    CAPTURE(issue.What);
    CHECK(false);
  }
  const std::expected<Loaded, Failure> back = Load(saved->Bytes);
  REQUIRE(back);
  for (const Issue &issue : back->Issues)
  {
    CAPTURE(issue.What);
    CHECK(false);
  }
  CHECK(Audit::Check(back->Drawing).Findings.empty());
  QueueAudit(saved->Bytes, "dwgio-remove-" + name + ".dwg");
}
} // namespace

TEST_CASE("remove/objects: an untyped object reacting to the removed one is rewritten without it")
{
  for (const std::string name : {"entities-r2000", "entities-r2018"})
  {
    CAPTURE(name);
    Drawing drawing = LoadFixture(name + ".dwg").Drawing;
    const std::vector<Handle> drawn = Drawn(drawing);
    REQUIRE(drawn.size() > 2);
    std::vector<Unknown *> untyped;
    for (Object &object : drawing.Objects)
    {
      if (auto *unknown = GetIf<Unknown>(&object); unknown != nullptr && unknown->Body && !unknown->Bytes.empty())
      {
        untyped.push_back(unknown);
      }
    }
    REQUIRE(untyped.size() > 1);
    Unknown &rewritten = *untyped.front();
    Unknown &kept = *untyped.back();
    kept.Body.reset(); // Own bits not told apart: its bytes stay, and the link with them.
    const Handle self = rewritten.Base.Self;
    for (Unknown *unknown : {&rewritten, &kept})
    {
      unknown->Base.Reactors.push_back(drawn[1]);
    }
    const std::vector<std::byte> bytes = kept.Bytes;
    REQUIRE(Edit::Remove(drawing, drawn[1]));
    const auto &after = Get<Unknown>(*Find(drawing, self));
    CHECK(after.Bytes.empty());
    CHECK(std::ranges::find(after.Base.Reactors, drawn[1]) == after.Base.Reactors.end());
    CHECK(Get<Unknown>(*Find(drawing, kept.Base.Self)).Bytes == bytes);
    const std::expected<Saved, Failure> saved = Save(drawing);
    REQUIRE(saved);
    const std::expected<Loaded, Failure> back = Load(saved->Bytes);
    REQUIRE(back);
    const auto &read = Get<Unknown>(*Find(back->Drawing, self));
    CHECK(std::ranges::find(read.Base.Reactors, drawn[1]) == read.Base.Reactors.end());
  }
}

TEST_CASE("remove/objects: a named object is removed with what it owns unless it is required or in use")
{
  for (const std::string name : {"objects-r14", "objects-r2000", "objects-r2004", "objects-r2018"})
  {
    CAPTURE(name);
    Drawing drawing = LoadFixture(name + ".dwg").Drawing;
    const Objects::Dictionary *mine = View::Dictionary(drawing, "DWGIO_DICT");
    REQUIRE(mine);
    const Handle dictionary = mine->Base.Self;
    const Handle inner = BaseOf(*View::Entry(drawing, *mine, "INNER")).Self;
    const Handle one = BaseOf(*View::Entry(drawing, Get<Objects::Dictionary>(*Find(drawing, inner)), "ONE")).Self;
    const std::size_t count = drawing.Objects.size();
    REQUIRE(Edit::Remove(drawing, dictionary));
    CHECK(drawing.Objects.size() == count - 5);
    CHECK(Find(drawing, one) == nullptr);
    CHECK(View::Dictionary(drawing, "DWGIO_DICT") == nullptr);
    const Objects::Dictionary &hard = *View::Dictionary(drawing, "DWGIO_HARD");
    REQUIRE(Edit::Remove(drawing, BaseOf(*View::Entry(drawing, hard, "ITEM")).Self));
    CHECK(View::Dictionary(drawing, "DWGIO_HARD")->Entries.empty());
    CHECK(CheckRefusal(drawing, drawing.Header.DictionaryNamedObjects) == "root dictionary required");
    CHECK(CheckRefusal(drawing, drawing.Header.DictionaryAcadGroup) == "DICTIONARY ACAD_GROUP in use");
    const Objects::Dictionary &styles = *View::Dictionary(drawing, "ACAD_MLINESTYLE");
    const Objects::Entry &standard = styles.Entries.front();
    REQUIRE(View::Matches(standard.Name, "Standard"));
    CHECK(CheckRefusal(drawing, standard.Object) == "MLINESTYLE " + standard.Name + " in use");
    Sound(drawing, name + "-named");
  }
}

TEST_CASE("remove/objects: a page setup no layout uses leaves its dictionary")
{
  Drawing drawing = LoadFixture("dimensions-r2018.dwg").Drawing;
  const Objects::Dictionary &setups = *View::Dictionary(drawing, "ACAD_PLOTSETTINGS");
  const Handle check = BaseOf(*View::Entry(drawing, setups, "Check")).Self;
  REQUIRE(Edit::Remove(drawing, check));
  CHECK(View::Dictionary(drawing, "ACAD_PLOTSETTINGS")->Entries.empty());
  Sound(drawing, "dimensions-r2018-setup");
}

TEST_CASE("remove/objects: a missing handle is refused and an orphan vertex is removed")
{
  Drawing drawing = Create();
  CHECK(CheckRefusal(drawing, Nowhere) == "handle 999 missing");
  CHECK(CheckRefusal(drawing, Handle{}) == "handle 0 missing");
  const Tables::BlockRecord &model = Model(drawing);
  CHECK(CheckRefusal(drawing, model.Block) ==
        std::format("BLOCK {} belongs to block *Model_Space", HexOf(model.Block)));
  Entities::Vertex vertex;
  vertex.Base.Self = Edit::MakeHandle(drawing);
  vertex.Base.Owner = Nowhere;
  drawing.Objects.emplace_back(vertex);
  REQUIRE(Edit::Remove(drawing, vertex.Base.Self));
  CHECK(Find(drawing, vertex.Base.Self) == nullptr);

  Objects::XRecord loose;
  loose.Base.Self = Edit::MakeHandle(drawing);
  loose.Base.Owner = drawing.Header.DictionaryNamedObjects;
  drawing.Objects.emplace_back(loose);
  Get<Objects::Dictionary>(*Find(drawing, drawing.Header.DictionaryNamedObjects)).Default = loose.Base.Self;
  CHECK(CheckRefusal(drawing, loose.Base.Self) == std::format("XRECORD {} in use", HexOf(loose.Base.Self)));
}

TEST_CASE("remove/objects: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO
