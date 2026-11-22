/***********************************************************************************************************************
 *  @file      tests/dwgio/edit/attributes.cpp
 *  @brief     Attributes added to an insert in each release's form, chain or list, closed by a SEQEND.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "support/acad.hpp"
#include "support/findings.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/attdef.hpp"
#include "dwgio/entities/attrib.hpp"
#include "dwgio/entities/insert.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/entities/seqend.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/save.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/version.hpp"
#include "dwgio/view.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <expected>
#include <string>
#include <variant>
#include <vector>

namespace DwgIO
{
namespace
{
auto MakeAttrib(const char *tag, double height) -> Entities::Attrib
{
  Entities::Attrib attribute;
  attribute.Tag = tag;
  attribute.Lettering.Value = tag;
  attribute.Lettering.Height = height;
  return attribute;
}

/**
 *  The attributes an insert names, in their order: its list from R2004, its chain before.
 */
auto Attributes(const Drawing &drawing, const Entities::Insert &insert) -> std::vector<Handle>
{
  if (drawing.Release >= Version::R2004)
  {
    return insert.Owned;
  }
  std::vector<Handle> chained;
  for (Handle at = insert.First; at;)
  {
    chained.push_back(at);
    at = at == insert.Last ? Handle{} : CommonOf(*Find(drawing, at))->Next;
  }
  return chained;
}

/**
 *  The position of an object among the drawing's, which is the file's order.
 */
auto Place(const Drawing &drawing, Handle self) -> std::size_t
{
  std::size_t at = 0;
  while (BaseOf(drawing.Objects[at]).Self != self)
  {
    ++at;
  }
  return at;
}
} // namespace

TEST_CASE("edit/attributes: an insert takes its attributes in its release's form closed by a SEQEND")
{
  for (const Version release : {Version::R14, Version::R2000, Version::R2004, Version::R2018})
  {
    CAPTURE(NameOf(release));
    Drawing drawing = Create(release);
    const Handle door = *Edit::AddBlock(drawing, "Door");
    Entities::AttDef number;
    number.Tag = "NUMBER";
    number.Prompt = "Door number";
    number.Lettering.Height = 0.25;
    REQUIRE(Edit::Add(drawing, number, "Door"));
    Tables::Layer doors;
    doors.Record.Name = "Doors";
    doors.Linetype = drawing.Header.LTypeContinuous;
    const Handle layer = *Edit::Add(drawing, doors);
    Entities::Insert placed;
    placed.Block = door;
    placed.Common.Layer = layer;
    const Handle insert = *Edit::Add(drawing, placed);

    const std::expected<Handle, Failure> first = Edit::AddAttribute(drawing, insert, MakeAttrib("NUMBER", 0.5));
    REQUIRE(first);
    Entities::Attrib styled = MakeAttrib("SIDE", 0.5);
    styled.Common.Layer = drawing.Header.Clayer;
    styled.Common.Lineweight = Lineweight::Mm018;
    styled.Lettering.Style = Handle{.Value = 0x501};
    const std::expected<Handle, Failure> second = Edit::AddAttribute(drawing, insert, styled);
    REQUIRE(second);
    Get<Entities::Attrib>(*Find(drawing, *second)).Lettering.Style = drawing.Header.Textstyle;

    const auto &head = Get<Entities::Insert>(*Find(drawing, insert));
    CHECK(head.Follows);
    CHECK(Attributes(drawing, head) == std::vector<Handle>{*first, *second});
    CHECK((drawing.Release >= Version::R2004 ? !head.First : head.Owned.empty()));
    const auto &one = Get<Entities::Attrib>(*Find(drawing, *first));
    CHECK(one.Base.Owner == insert);
    CHECK(one.Common.Space == Space::Owner);
    CHECK(one.Common.Layer == layer);
    CHECK(one.Common.Lineweight == Lineweight::ByLayer);
    CHECK(one.Lettering.Style == drawing.Header.Textstyle);
    CHECK(one.Common.Previous == Handle{});
    const auto &two = Get<Entities::Attrib>(*Find(drawing, *second));
    CHECK(two.Common.Layer == drawing.Header.Clayer);
    CHECK(two.Common.Lineweight == Lineweight::Mm018);
    CHECK(two.Common.Previous == (drawing.Release >= Version::R2004 ? Handle{} : *first));
    const Object *seqend = Find(drawing, head.SeqEnd);
    REQUIRE(seqend);
    CHECK(Holds<Entities::SeqEnd>(*seqend));
    CHECK(BaseOf(*seqend).Owner == insert);
    CHECK(CommonOf(*seqend)->Layer == layer);
    CHECK(Place(drawing, *first) < Place(drawing, *second));
    CHECK(Place(drawing, *second) < Place(drawing, head.SeqEnd));
    CHECK(FindingTexts(Audit::Check(drawing)) == std::vector<std::string>{});

    const std::expected<Saved, Failure> saved = Save(drawing);
    REQUIRE(saved);
    CHECK(saved->Issues.empty());
    const std::expected<Loaded, Failure> back = Load(saved->Bytes);
    REQUIRE(back);
    CHECK(back->Issues.empty());
    CHECK(FindingTexts(Audit::Check(back->Drawing)) == std::vector<std::string>{});
    const auto &read = Get<Entities::Insert>(*Find(back->Drawing, insert));
    CHECK(read.Follows);
    CHECK(Attributes(back->Drawing, read) == std::vector<Handle>{*first, *second});
    CHECK(Get<Entities::Attrib>(*Find(back->Drawing, *second)).Tag == "SIDE");
    CHECK(read.SeqEnd == head.SeqEnd);
    if (release == Version::R2000 || release == Version::R2018)
    {
      QueueAudit(saved->Bytes, std::string{"dwgio-added-attributes-"} + std::string{NameOf(release)} + ".dwg");
    }
  }
}

TEST_CASE("edit/attributes: only an insert with a whole chain takes an attribute under a free handle")
{
  Drawing drawing = Create();
  const Handle line = *Edit::Add(drawing, Entities::Line{});
  const Handle door = *Edit::AddBlock(drawing, "Door");
  Entities::Insert placed;
  placed.Block = door;
  const Handle insert = *Edit::Add(drawing, placed);
  const std::size_t count = drawing.Objects.size();
  const Handle seed = drawing.Header.Handseed;
  CHECK(Edit::AddAttribute(drawing, Handle{.Value = 0x999}, MakeAttrib("A", 1)).error().What == "handle 999 missing");
  CHECK(Edit::AddAttribute(drawing, line, MakeAttrib("A", 1)).error().What == "LINE " + HexOf(line) + " is no insert");
  Entities::Attrib taken = MakeAttrib("A", 1);
  taken.Base.Self = line;
  CHECK(Edit::AddAttribute(drawing, insert, taken).error().What == "handle " + HexOf(line) + " exists");
  auto &head = Get<Entities::Insert>(*Find(drawing, insert));
  head.Last = Handle{.Value = 0x999};
  CHECK(Edit::AddAttribute(drawing, insert, MakeAttrib("A", 1)).error().What ==
        "attributes of INSERT " + HexOf(insert) + " cut");
  head.Last = drawing.Header.Clayer;
  CHECK(Edit::AddAttribute(drawing, insert, MakeAttrib("A", 1)).error().What ==
        "attributes of INSERT " + HexOf(insert) + " cut");
  CHECK(drawing.Objects.size() == count);
  CHECK(drawing.Header.Handseed == seed);
  CHECK(!Get<Entities::Insert>(*Find(drawing, insert)).Follows);
  Get<Entities::Insert>(*Find(drawing, insert)).Last = {};
  Entities::Attrib own = MakeAttrib("A", 1);
  own.Base.Self = Handle{.Value = 0x500};
  CHECK(Edit::AddAttribute(drawing, insert, own) == Handle{.Value = 0x500});
  CHECK(Get<Entities::Insert>(*Find(drawing, insert)).First == Handle{.Value = 0x500});
}

TEST_CASE("edit/attributes: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO
