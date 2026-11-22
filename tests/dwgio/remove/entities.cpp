/***********************************************************************************************************************
 *  @file      tests/dwgio/remove/entities.cpp
 *  @brief     Removing entities: chains, lists, sequences, groups and reactors kept whole.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "edit/unhook.hpp"
#include "support/acad.hpp"
#include "support/fixtures.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/arc.hpp"
#include "dwgio/entities/attrib.hpp"
#include "dwgio/entities/block.hpp"
#include "dwgio/entities/circle.hpp"
#include "dwgio/entities/dimension.hpp"
#include "dwgio/entities/hatch.hpp"
#include "dwgio/entities/insert.hpp"
#include "dwgio/entities/leader.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/entities/mtext.hpp"
#include "dwgio/entities/polyline.hpp"
#include "dwgio/entities/seqend.hpp"
#include "dwgio/entities/vertex.hpp"
#include "dwgio/entities/viewport.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/group.hpp"
#include "dwgio/objects/layout.hpp"
#include "dwgio/objects/sortents.hpp"
#include "dwgio/save.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/style.hpp"
#include "dwgio/tables/table.hpp"
#include "dwgio/tables/view.hpp"
#include "dwgio/tables/viewportheader.hpp"
#include "dwgio/version.hpp"
#include "dwgio/view.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <string>
#include <unordered_set>
#include <vector>

namespace DwgIO
{
namespace
{
/**
 *  The handles of the objects of a kind, in file order.
 */
template <class KindT>
auto All(const Drawing &drawing) -> std::vector<Handle>
{
  std::vector<Handle> handles;
  for (const Object &object : drawing.Objects)
  {
    if (Holds<KindT>(object))
    {
      handles.push_back(BaseOf(object).Self);
    }
  }
  return handles;
}

template <class RecordT>
auto RecordOf(const Drawing &drawing, const char *name) -> Handle
{
  const RecordT *record = View::Record<RecordT>(drawing, name);
  REQUIRE_MESSAGE(record, name);
  return record->Record.Base.Self;
}

/**
 *  A record's name as the drawing spells it: a file before R2000 keeps names in capitals.
 */
template <class RecordT>
auto RecordNameOf(const Drawing &drawing, const char *name) -> std::string
{
  return View::Record<RecordT>(drawing, name)->Record.Name;
}

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

/**
 *  Whether any object still names the handle as its own, its owner, a reactor or an extension dictionary.
 */
auto IsNamed(const Drawing &drawing, Handle handle) -> bool
{
  return std::ranges::any_of(drawing.Objects,
                             [handle](const Object &object) -> bool
                             {
                               const Base &base = BaseOf(object);
                               return base.Self == handle || base.Owner == handle || base.Dictionary == handle ||
                                      std::ranges::contains(base.Reactors, handle);
                             });
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

auto CircleOf(const Drawing &drawing, double radius) -> Handle
{
  for (const Handle circle : All<Entities::Circle>(drawing))
  {
    if (Get<Entities::Circle>(*Find(drawing, circle)).Radius == radius)
    {
      return circle;
    }
  }
  FAIL("no circle of that radius");
  return {};
}
} // namespace

TEST_CASE("remove/entities: an entity leaves its block's chain or list with its neighbours joined")
{
  for (const std::string name : {"entities-r14", "entities-r2000", "entities-r2004", "entities-r2018"})
  {
    CAPTURE(name);
    Drawing drawing = LoadFixture(name + ".dwg").Drawing;
    const std::vector<Handle> drawn = Drawn(drawing);
    REQUIRE(drawn.size() > 3);
    const Handle middle = drawn[2];
    REQUIRE(Edit::Remove(drawing, middle));
    CHECK(Find(drawing, middle) == nullptr);
    std::vector<Handle> expected = drawn;
    std::erase(expected, middle);
    CHECK(Drawn(drawing) == expected);
    REQUIRE(Edit::Remove(drawing, drawn.front()));
    REQUIRE(Edit::Remove(drawing, drawn.back()));
    expected.erase(expected.begin());
    expected.pop_back();
    CHECK(Drawn(drawing) == expected);
    const Tables::BlockRecord &model = Model(drawing);
    if (drawing.Release >= Version::R2004)
    {
      CHECK(model.Entities == expected);
      CHECK(model.First == Handle{});
    }
    else
    {
      CHECK(model.First == expected.front());
      CHECK(model.Last == expected.back());
      CHECK(CommonOf(*Find(drawing, expected.front()))->Previous == Handle{});
      CHECK(CommonOf(*Find(drawing, expected[1]))->Previous == expected.front());
      CHECK(CommonOf(*Find(drawing, expected.back()))->Next == Handle{});
    }
    Sound(drawing, name + "-entity");
  }
}

TEST_CASE("remove/entities: an insert takes its attributes and SEQEND and leaves its block's list")
{
  for (const std::string name : {"entities-r14", "entities-r2000", "entities-r2010", "entities-r2018"})
  {
    CAPTURE(name);
    Drawing drawing = LoadFixture(name + ".dwg").Drawing;
    const Handle tag = RecordOf<Tables::BlockRecord>(drawing, "Tag");
    Handle insert;
    for (const Handle one : All<Entities::Insert>(drawing))
    {
      insert = Get<Entities::Insert>(*Find(drawing, one)).Block == tag ? one : insert;
    }
    REQUIRE(insert);
    const Entities::Insert placed = Get<Entities::Insert>(*Find(drawing, insert));
    REQUIRE(placed.Follows);
    const std::vector<Handle> attributes = All<Entities::Attrib>(drawing);
    REQUIRE(!attributes.empty());
    CHECK(CheckRefusal(drawing, attributes.front()) ==
          std::format("ATTRIB {} belongs to INSERT {}", HexOf(attributes.front()), HexOf(insert)));
    CHECK(CheckRefusal(drawing, placed.SeqEnd) ==
          std::format("SEQEND {} belongs to INSERT {}", HexOf(placed.SeqEnd), HexOf(insert)));
    CHECK(CheckRefusal(drawing, tag) == "block " + RecordNameOf<Tables::BlockRecord>(drawing, "Tag") + " in use");
    REQUIRE(Edit::Remove(drawing, insert));
    CHECK(All<Entities::Attrib>(drawing).empty());
    CHECK(All<Entities::SeqEnd>(drawing).empty());
    CHECK(!IsNamed(drawing, insert));
    CHECK(!std::ranges::contains(Get<Tables::BlockRecord>(*Find(drawing, tag)).Inserts, insert));
    Sound(drawing, name + "-insert");
  }
}

TEST_CASE("remove/entities: an old-style polyline takes its vertices and SEQEND")
{
  for (const std::string name : {"curves-r14", "curves-r2000", "curves-r2004", "curves-r2018"})
  {
    CAPTURE(name);
    Drawing drawing = LoadFixture(name + ".dwg").Drawing;
    const std::vector<Handle> polylines = All<Entities::Polyline>(drawing);
    REQUIRE(polylines.size() > 2);
    const std::size_t vertices = All<Entities::Vertex>(drawing).size();
    const std::size_t seqends = All<Entities::SeqEnd>(drawing).size();
    const Entities::Polyline first = Get<Entities::Polyline>(*Find(drawing, polylines.front()));
    const Handle vertex = drawing.Release >= Version::R2004 ? first.Vertices.front() : first.First;
    CHECK(CheckRefusal(drawing, vertex) ==
          std::format("VERTEX_2D {} belongs to POLYLINE_2D {}", HexOf(vertex), HexOf(polylines.front())));
    std::size_t owned = 0;
    for (const Handle one : All<Entities::Vertex>(drawing))
    {
      owned += BaseOf(*Find(drawing, one)).Owner == polylines.front() ? 1U : 0U;
    }
    REQUIRE(owned > 0);
    REQUIRE(Edit::Remove(drawing, polylines.front()));
    CHECK(All<Entities::Vertex>(drawing).size() == vertices - owned);
    CHECK(All<Entities::SeqEnd>(drawing).size() == seqends - 1);
    CHECK(!IsNamed(drawing, polylines.front()));
    for (const Handle mesh : {polylines[polylines.size() - 1], polylines[polylines.size() - 2]})
    {
      REQUIRE(Edit::Remove(drawing, mesh));
    }
    Sound(drawing, name + "-polyline");
  }
}

TEST_CASE("remove/entities: a dimension takes its extension dictionary and association and leaves no reactors")
{
  for (const std::string name : {"dimensions-r2000", "dimensions-r2007", "dimensions-r2018"})
  {
    CAPTURE(name);
    Drawing drawing = LoadFixture(name + ".dwg").Drawing;
    const Handle dimension = All<Entities::ArcDimension>(drawing).front();
    const Handle dictionary = BaseOf(*Find(drawing, dimension)).Dictionary;
    REQUIRE(dictionary);
    const Object *association =
        View::Entry(drawing, Get<Objects::Dictionary>(*Find(drawing, dictionary)), "ACAD_DIMASSOC");
    REQUIRE(association);
    const Handle bound = BaseOf(*association).Self;
    Handle arc;
    for (const Handle one : All<Entities::Arc>(drawing))
    {
      arc = std::ranges::contains(BaseOf(*Find(drawing, one)).Reactors, bound) ? one : arc;
    }
    REQUIRE(arc);
    Drawing measured = drawing;
    REQUIRE(Edit::Remove(drawing, dimension));
    CHECK(Find(drawing, dictionary) == nullptr);
    CHECK(!IsNamed(drawing, bound));
    CHECK(!IsNamed(drawing, dimension));
    Sound(drawing, name + "-dimension");

    REQUIRE(Edit::Remove(measured, arc));
    CHECK(Find(measured, bound) == nullptr);
    CHECK(Get<Objects::Dictionary>(*Find(measured, dictionary)).Entries.empty());
    Drawing unbound = measured;
    REQUIRE(Edit::Remove(unbound, dictionary));
    CHECK(BaseOf(*Find(unbound, dimension)).Dictionary == Handle{});
    Sound(measured, name + "-measured");
    Sound(unbound, name + "-unbound");
  }
}

TEST_CASE("remove/entities: an entity leaves its groups and the draw order and a group its members' reactors")
{
  for (const std::string name : {"objects-r14", "objects-r2000", "objects-r2004", "objects-r2018"})
  {
    CAPTURE(name);
    Drawing drawing = LoadFixture(name + ".dwg").Drawing;
    Handle pair;
    for (const Handle group : All<Objects::Group>(drawing))
    {
      pair = Get<Objects::Group>(*Find(drawing, group)).Description == "A line and a circle" ? group : pair;
    }
    REQUIRE(pair);
    const std::vector<Handle> members = Get<Objects::Group>(*Find(drawing, pair)).Members;
    REQUIRE(members.size() == 2);
    const Handle line = members.front();
    REQUIRE(Edit::Remove(drawing, line));
    CHECK(Get<Objects::Group>(*Find(drawing, pair)).Members == std::vector<Handle>{members.back()});
    for (const Handle order : All<Objects::SortEntsTable>(drawing))
    {
      for (const Objects::SortEntsTables::Entry &entry : Get<Objects::SortEntsTable>(*Find(drawing, order)).Entries)
      {
        CHECK(entry.Entity != line);
      }
    }
    CHECK(!IsNamed(drawing, line));
    REQUIRE(Edit::Remove(drawing, pair));
    CHECK(!IsNamed(drawing, pair));
    CHECK(View::Entry(drawing, *View::Dictionary(drawing, "ACAD_GROUP"), "Pair") == nullptr);
    Sound(drawing, name + "-group");
  }
}

TEST_CASE("remove/entities: an entity in a group of the entities fixture is removed with its xdata")
{
  for (const std::string name : {"entities-r14", "entities-r2000", "entities-r2018"})
  {
    CAPTURE(name);
    Drawing drawing = LoadFixture(name + ".dwg").Drawing;
    const Handle dwgio = RecordOf<Tables::AppId>(drawing, "DWGIO");
    CHECK(CheckRefusal(drawing, dwgio) == "application DWGIO in use");
    const Handle group = All<Objects::Group>(drawing).front();
    const Handle line = Get<Objects::Group>(*Find(drawing, group)).Members.front();
    REQUIRE(!BaseOf(*Find(drawing, line)).Extended.empty());
    REQUIRE(Edit::Remove(drawing, line));
    CHECK(Get<Objects::Group>(*Find(drawing, group)).Members.size() == 1);
    REQUIRE(Edit::Remove(drawing, dwgio));
    CHECK(View::Record<Tables::AppId>(drawing, "DWGIO") == nullptr);
    Sound(drawing, name + "-grouped");
  }
}

TEST_CASE("remove/entities: removing a hatch's boundary unbinds it and a leader's note unannotates it")
{
  for (const std::string name : {"curves-r14", "curves-r2000", "curves-r2018"})
  {
    CAPTURE(name);
    Drawing drawing = LoadFixture(name + ".dwg").Drawing;
    const Handle hatch = All<Entities::Hatch>(drawing).front();
    const Entities::Hatch &bound = Get<Entities::Hatch>(*Find(drawing, hatch));
    REQUIRE(bound.Associative);
    std::vector<Handle> sources;
    for (const Entities::Path &path : bound.Paths)
    {
      sources.insert(sources.end(), path.Sources.begin(), path.Sources.end());
    }
    REQUIRE(!sources.empty());
    REQUIRE(Edit::Remove(drawing, sources.front()));
    const Entities::Hatch &unbound = Get<Entities::Hatch>(*Find(drawing, hatch));
    CHECK(!unbound.Associative);
    CHECK(std::ranges::all_of(unbound.Paths, [](const Entities::Path &path) -> bool { return path.Sources.empty(); }));
    for (const Handle kept : std::vector<Handle>{sources.begin() + 1, sources.end()})
    {
      CHECK(!std::ranges::contains(BaseOf(*Find(drawing, kept)).Reactors, hatch));
    }

    const Handle leader = All<Entities::Leader>(drawing).front();
    const Handle note = Get<Entities::Leader>(*Find(drawing, leader)).Note;
    REQUIRE(Holds<Entities::MText>(*Find(drawing, note)));
    REQUIRE(Edit::Remove(drawing, note));
    CHECK(Get<Entities::Leader>(*Find(drawing, leader)).Note == Handle{});
    CHECK(Get<Entities::Leader>(*Find(drawing, leader)).Annotation == 3);
    CHECK(!IsNamed(drawing, note));
    Sound(drawing, name + "-hatch");
  }
}

TEST_CASE("remove/entities: a viewport leaves its layout and before R2000 its viewport header is removed too")
{
  for (const std::string name : {"objects-r14", "objects-r2000", "objects-r2018"})
  {
    CAPTURE(name);
    Drawing drawing = LoadFixture(name + ".dwg").Drawing;
    std::vector<Handle> floating;
    Handle clip;
    Handle clipped;
    for (const Handle one : All<Entities::Viewport>(drawing))
    {
      const auto &viewport = Get<Entities::Viewport>(*Find(drawing, one));
      clip = viewport.Clip ? viewport.Clip : clip;
      clipped = viewport.Clip ? one : clipped;
      if (!viewport.Clip && (!viewport.Header || viewport.Header != drawing.Header.CurrentViewportEntityHeader))
      {
        floating.push_back(one);
      }
    }
    REQUIRE(!floating.empty());
    const std::size_t headers = All<Tables::ViewportEntityHeader>(drawing).size();
    REQUIRE(Edit::Remove(drawing, floating.back()));
    if (clip)
    {
      CHECK(CheckRefusal(drawing, clip) == std::format("{} {} in use", TypeOf(*Find(drawing, clip)), HexOf(clip)));
      REQUIRE(Edit::Remove(drawing, clipped));
      REQUIRE(Edit::Remove(drawing, clip));
    }
    for (const Objects::Layout *layout : View::Layouts(drawing))
    {
      CHECK(!std::ranges::contains(layout->Viewports, floating.back()));
      CHECK((!clipped || !std::ranges::contains(layout->Viewports, clipped)));
      CHECK((!clipped || layout->Viewport != clipped));
    }
    CHECK(!IsNamed(drawing, floating.back()));
    if (drawing.Release < Version::R2000)
    {
      CHECK(All<Tables::ViewportEntityHeader>(drawing).size() == headers - 1);
      for (const Handle one : All<Tables::ViewportEntityHeader>(drawing))
      {
        const Handle previous = Get<Tables::ViewportEntityHeader>(*Find(drawing, one)).Previous;
        CHECK((!previous || Find(drawing, previous) != nullptr));
      }
      const Handle current = drawing.Header.CurrentViewportEntityHeader;
      const std::string word = Get<Tables::ViewportEntityHeader>(*Find(drawing, current)).Record.Name;
      CHECK(CheckRefusal(drawing, current) == "viewport header " + word + " in use");
    }
    Sound(drawing, name + "-viewport");
  }
}

TEST_CASE("remove/entities: removed viewport headers whose chain loops leave the one after them at the end")
{
  const auto header = [](std::uint64_t self, std::uint64_t previous) -> Tables::ViewportEntityHeader
  {
    Tables::ViewportEntityHeader made;
    made.Record.Base.Self = Handle{self};
    made.Previous = Handle{previous};
    return made;
  };
  Drawing drawing;
  drawing.Objects.emplace_back(header(0x10, 0x11));
  drawing.Objects.emplace_back(header(0x11, 0x10));
  drawing.Objects.emplace_back(header(0x12, 0x10));
  Edit::UnhookRemoved(drawing, {0x10, 0x11});
  CHECK(Get<Tables::ViewportEntityHeader>(drawing.Objects.back()).Previous == Handle{});
}

TEST_CASE("remove/entities: a circle in an R2004 block's list is removed and the rest keep their order")
{
  Drawing drawing = Create(Version::R2004);
  const Handle first = *Edit::Add(drawing, Entities::Circle{});
  const Handle second = *Edit::Add(drawing, Entities::Circle{});
  const Handle third = *Edit::Add(drawing, Entities::Circle{});
  REQUIRE(Edit::Remove(drawing, second));
  CHECK(Model(drawing).Entities == std::vector<Handle>{first, third});
  Sound(drawing, "created-r2004");
  CHECK(CircleOf(drawing, 0) == first);
}

TEST_CASE("remove/entities: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO
