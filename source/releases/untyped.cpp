/***********************************************************************************************************************
 *  @file      source/releases/untyped.cpp
 *  @brief     The untyped objects a save carries to another release, and those it leaves out.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/untyped.hpp"

#include "releases/opaque.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <unordered_set>
#include <variant>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
/**
 *  A mask of the releases from which AutoCAD writes a class differently from the one before.
 */
using Changes = unsigned;

constexpr Changes None = 0;
constexpr Changes Added = 1U << static_cast<unsigned>(Version::R2004);
constexpr Changes Text = 1U << static_cast<unsigned>(Version::R2007);
constexpr Changes Context = 1U << static_cast<unsigned>(Version::R2010);
constexpr Changes Moved = 1U << static_cast<unsigned>(Version::R2013);
constexpr Changes Latest = 1U << static_cast<unsigned>(Version::R2018);

/**
 *  From which releases a class's own bits change, and the layout that recasts them across those changes, empty where
 *  none does.
 */
struct Lineage
{
  std::string_view Class;
  Changes Changed{};
  std::string_view Layout;
  Changes Unread{}; /**< The changes the layout does not read: a class crossing one is not carried. */
};

/**
 *  Field groups several classes share: the evaluation expression every block element and history node opens with, the
 *  block element, the grip, the parameter of one point and of two, the action, the model documentation view style every
 *  view style opens with, a history node (its matrix and color, then what its shape holds), and below R2013 the
 *  dependency, the associative action, an action's value parameter and the surface action body.
 */
struct Fragment
{
  std::string_view Name;
  std::string_view Layout;
};

constexpr std::array<Fragment, 13> Fragments{{
    {.Name = "EVAL", .Layout = "BL BL BL X BL"},
    {.Name = "ELEM", .Layout = "EVAL T E BL"},
    {.Name = "GRIP", .Layout = "ELEM BL BL 3BD B BL"},
    {.Name = "PARAM", .Layout = "ELEM B B"},
    {.Name = "P1", .Layout = "PARAM 3BD N { BL T } N { BL T } BL"},
    {.Name = "P2", .Layout = "PARAM 3BD 3BD N { BL T } N { BL T } N { BL T } N { BL T } BL BL BL BL BS"},
    {.Name = "ACTION", .Layout = "ELEM 3BD BL N { BL }"},
    {.Name = "VIEW", .Layout = "BS T B18 T18 BL18"},
    {.Name = "NODE", .Layout = "EVAL BL BL BD BD BD BD BD BD BD BD BD BD BD BD BD BD BD BD CMC REST"},
    {.Name = "DEPENDENCY", .Layout = "BL BL B B B B BL B BL"},
    {.Name = "ASSOCIATION", .Layout = "BL BL BL BL N { B }"},
    {.Name = "VALUE", .Layout = "BL T BL N { X }"},
    {.Name = "SURFACE", .Layout = "BL BL BL N { } BL N { VALUE } BL B BL B BS BL"},
}};

// AutoCAD changes a class's own bits at the releases listed; a class with none listed writes them alike in every
// release. CELLSTYLEMAP, TABLECONTENT and TABLEGEOMETRY are made again at each save.
//
// - Block elements of dynamic blocks and dimension associations keep their text in the data up to R2004 and in the
//   string stream from R2007; a block element also holds the release it was saved in.
// - Object context data hold a version, 3 up to R2007 and 4 from R2010, as chapter 20.4.89 has it.
// - Shape history nodes write their color as a CMC of the release.
// - View styles write their colors as a CMC of the release and their text as block elements do; from R2018 they
//   hold a display name and flags after the description, which below R2018 AutoCAD keeps in a record of their
//   extension dictionary (releases/roundtrip.cpp moves them).
// - A constraint group lists its dependencies in no fixed order, which differs from one save to the next; its bits
//   are otherwise alike in R2013 and R2018.
// - Break data hold for each break point a null handle more up to R2000, and a bit from R2004, followed by another
//   from R2013.
// - The associative network keeps its text in the string stream from R2007; at R2013 it moves its parameters
//   between its objects, which no layout here reads. The value dependencies, the variables, the action parameters of
//   edges and paths and the surface action bodies cross R2007 below R2013 by their layouts.
// - The rest change in ways no layout here reads: the table geometry at R2018, the cell style map is gone from
//   R2010, the geometry dependencies name their persistent subentity's class as a text up to R2010 and otherwise from
//   R2013, the constraint groups write the class names of their nodes among the nodes' data, and the constraint
//   parameters, properties tables and dependency bodies carry text at places in their data that are not known.
constexpr std::array<Lineage, 75> Lineages{{
    {.Class = "ACAD_EVALUATION_GRAPH", .Changed = None, .Layout = ""},
    {.Class = "ACDB_BLOCKREPRESENTATION_DATA", .Changed = None, .Layout = ""},
    {.Class = "ACDB_DYNAMICBLOCKPROXYNODE", .Changed = None, .Layout = ""},
    {.Class = "ACDB_DYNAMICBLOCKPURGEPREVENTER_VERSION", .Changed = None, .Layout = ""},
    {.Class = "ACSH_HISTORY_CLASS", .Changed = None, .Layout = ""},
    {.Class = "SECTION_MANAGER", .Changed = None, .Layout = ""},
    {.Class = "SECTION_SETTINGS", .Changed = None, .Layout = ""},
    {.Class = "ACDB_BLKREFOBJECTCONTEXTDATA_CLASS", .Changed = Context, .Layout = "V REST"},
    {.Class = "ACDB_MTEXTATTRIBUTEOBJECTCONTEXTDATA_CLASS", .Changed = Context, .Layout = "V REST"},
    {.Class = "ACSH_BOOLEAN_CLASS", .Changed = Added, .Layout = "NODE"},
    {.Class = "ACSH_BOX_CLASS", .Changed = Added, .Layout = "NODE"},
    {.Class = "ACSH_CONE_CLASS", .Changed = Added, .Layout = "NODE"},
    {.Class = "ACSH_CYLINDER_CLASS", .Changed = Added, .Layout = "NODE"},
    {.Class = "ACSH_EXTRUSION_CLASS", .Changed = Added, .Layout = "NODE"},
    {.Class = "ACSH_LOFT_CLASS", .Changed = Added, .Layout = "NODE"},
    {.Class = "ACSH_PYRAMID_CLASS", .Changed = Added, .Layout = "NODE"},
    {.Class = "ACSH_REVOLVE_CLASS", .Changed = Added, .Layout = "NODE"},
    {.Class = "ACSH_SPHERE_CLASS", .Changed = Added, .Layout = "NODE"},
    {.Class = "ACSH_SWEEP_CLASS", .Changed = Added, .Layout = "NODE"},
    {.Class = "ACSH_TORUS_CLASS", .Changed = Added, .Layout = "NODE"},
    {.Class = "ACSH_WEDGE_CLASS", .Changed = Added, .Layout = "NODE"},
    {.Class = "DIMASSOC", .Changed = Text, .Layout = "MASK B RC { T RC N { BS BL } BD BD 3BD B }"},
    {.Class = "BLOCKGRIPLOCATIONCOMPONENT", .Changed = Text, .Layout = "EVAL BL T"},
    {.Class = "BLOCKALIGNMENTGRIP", .Changed = Text, .Layout = "GRIP 3BD"},
    {.Class = "BLOCKFLIPGRIP", .Changed = Text, .Layout = "GRIP BL 3BD"},
    {.Class = "BLOCKLINEARGRIP", .Changed = Text, .Layout = "GRIP 3BD"},
    {.Class = "BLOCKLOOKUPGRIP", .Changed = Text, .Layout = "GRIP"},
    {.Class = "BLOCKPOLARGRIP", .Changed = Text, .Layout = "GRIP"},
    {.Class = "BLOCKROTATIONGRIP", .Changed = Text, .Layout = "GRIP"},
    {.Class = "BLOCKVISIBILITYGRIP", .Changed = Text, .Layout = "GRIP"},
    {.Class = "BLOCKXYGRIP", .Changed = Text, .Layout = "GRIP"},
    {.Class = "BLOCKALIGNMENTPARAMETER", .Changed = Text, .Layout = "P2 B"},
    {.Class = "BLOCKBASEPOINTPARAMETER", .Changed = Text, .Layout = "P1 3BD 3BD"},
    {.Class = "BLOCKFLIPPARAMETER", .Changed = Text, .Layout = "P2 T T T T 3BD BL T"},
    {.Class = "BLOCKLINEARPARAMETER", .Changed = Text, .Layout = "P2 T T BD BL BD BD BD N { BD }"},
    {.Class = "BLOCKLOOKUPPARAMETER", .Changed = Text, .Layout = "P1 BL T T"},
    {.Class = "BLOCKPOINTPARAMETER", .Changed = Text, .Layout = "P1 T T 3BD"},
    {.Class = "BLOCKPOLARPARAMETER",
     .Changed = Text,
     .Layout = "P2 T T T T BD BL BD BD BD N { BD } BL BD BD BD N { BD }"},
    {.Class = "BLOCKROTATIONPARAMETER", .Changed = Text, .Layout = "P2 3BD T T BD BL BD BD BD N { BD }"},
    {.Class = "BLOCKVISIBILITYPARAMETER", .Changed = Text, .Layout = "P1 B T T B N N { T N N }"},
    {.Class = "BLOCKXYPARAMETER",
     .Changed = Text,
     .Layout = "P2 T T T T BD BD BL BD BD BD N { BD } BL BD BD BD N { BD }"},
    {.Class = "BLOCKARRAYACTION", .Changed = Text, .Layout = "ACTION BL T BL T BL T BL T BD BD"},
    {.Class = "BLOCKFLIPACTION", .Changed = Text, .Layout = "ACTION BL T BL T BL T BL T"},
    {.Class = "BLOCKLOOKUPACTION", .Changed = Text, .Layout = "ACTION LOOKUP { BL BL BL B T B T } B"},
    {.Class = "BLOCKMOVEACTION", .Changed = Text, .Layout = "ACTION BL T BL T BD BD RC"},
    {.Class = "BLOCKPOLARSTRETCHACTION",
     .Changed = Text,
     .Layout = "ACTION BL T BL T BL T BL T BL T BL T N { 2RD } N { N { BS } } BL N { N { BS } } BS BD BD BS"},
    {.Class = "BLOCKROTATEACTION", .Changed = Text, .Layout = "ACTION 3BD BL T BL T B 3BD BL T"},
    {.Class = "BLOCKSCALEACTION", .Changed = Text, .Layout = "ACTION 3BD BL T BL T B 3BD BL T BL T BL T RC"},
    {.Class = "BLOCKSTRETCHACTION",
     .Changed = Text,
     .Layout = "ACTION BL T BL T N { 2RD } N { N { BS } } N { BL N { BS } } BD BD RC"},
    {.Class = "BLOCKHORIZONTALCONSTRAINTPARAMETER", .Changed = Text, .Layout = ""},
    {.Class = "BLOCKVERTICALCONSTRAINTPARAMETER", .Changed = Text, .Layout = ""},
    {.Class = "BLOCKRADIALCONSTRAINTPARAMETER", .Changed = Text, .Layout = ""},
    {.Class = "BLOCKUSERPARAMETER", .Changed = Text, .Layout = ""},
    {.Class = "BLOCKPROPERTIESTABLE", .Changed = Text, .Layout = ""},
    {.Class = "BLOCKPROPERTIESTABLEGRIP", .Changed = Text, .Layout = ""},
    {.Class = "BLOCKPARAMDEPENDENCYBODY", .Changed = Text, .Layout = ""},
    {.Class = "ASSOCDIMDEPENDENCYBODY", .Changed = Text, .Layout = ""},
    {.Class = "ACDBASSOCVALUEDEPENDENCY", .Changed = Text, .Layout = "DEPENDENCY BL T X"},
    {.Class = "ACDBASSOCGEOMDEPENDENCY", .Changed = Text | Moved, .Layout = ""},
    {.Class = "ACDBASSOCVARIABLE",
     .Changed = Text | Moved,
     .Layout = "ASSOCIATION BL T T T T X B B N { BL }",
     .Unread = Moved},
    {.Class = "ACDBASSOC2DCONSTRAINTGROUP", .Changed = Text | Moved, .Layout = ""},
    {.Class = "ACDBASSOCDEPENDENCY", .Changed = Moved, .Layout = ""},
    {.Class = "ACDBASSOCNETWORK", .Changed = Moved, .Layout = ""},
    {.Class = "ACDBASSOCACTION", .Changed = Moved, .Layout = ""},
    {.Class = "ACDBASSOCPERSSUBENTMANAGER", .Changed = Moved, .Layout = ""},
    {.Class = "ACDBPERSSUBENTMANAGER", .Changed = Moved, .Layout = ""},
    {.Class = "ACDBASSOCEDGEACTIONPARAM",
     .Changed = Text | Moved,
     .Layout = "BL T BL BL B BL 3BD 3BD",
     .Unread = Moved},
    {.Class = "ACDBASSOCPATHACTIONPARAM", .Changed = Text | Moved, .Layout = "BL T BL BL N { } BL", .Unread = Moved},
    {.Class = "ACDBASSOCEXTRUDEDSURFACEACTIONBODY", .Changed = Text | Moved, .Layout = "SURFACE BL", .Unread = Moved},
    {.Class = "ACDBASSOCLOFTEDSURFACEACTIONBODY", .Changed = Text | Moved, .Layout = "SURFACE BL", .Unread = Moved},
    {.Class = "ACDBASSOCREVOLVEDSURFACEACTIONBODY", .Changed = Text | Moved, .Layout = "SURFACE BL B", .Unread = Moved},
    {.Class = "ACDBASSOCSWEPTSURFACEACTIONBODY", .Changed = Text | Moved, .Layout = "SURFACE BL", .Unread = Moved},
    {.Class = "ACDBDETAILVIEWSTYLE",
     .Changed = Added | Text | Latest,
     .Layout = "VIEW BS BL CMC BD T BD RC CMC BD BL CMC CMC BD BL BD BL T BL CMC BL CMC RC"},
    {.Class = "ACDBSECTIONVIEWSTYLE",
     .Changed = Added | Text | Latest,
     .Layout = "VIEW BS BL CMC BD CMC BD T BD BL CMC BL CMC BD BD CMC BD BL BD BL T CMC CMC T BD BL B B BL BD BL BD N "
               "{ BD }"},
    {.Class = "BREAKDATA",
     .Changed = Added | Moved,
     .Layout = "BS H N { BL B04 BS BS BL 3BD 3BD H H14 N { BL BL BL H } }"},
}};

/**
 *  Made again at each save, so listed apart: they are compared by place rather than handle.
 */
constexpr std::array<Lineage, 3> Remade{{
    {.Class = "CELLSTYLEMAP", .Changed = Text | Context, .Layout = ""},
    {.Class = "TABLECONTENT", .Changed = Text, .Layout = ""},
    {.Class = "TABLEGEOMETRY", .Changed = Latest, .Layout = ""},
}};

/**
 *  Each crossed release is the later of the two the save lies between.
 */
auto ChangesBetween(Version from, Version to) -> Changes
{
  const auto low = static_cast<unsigned>(std::min(from, to));
  const auto high = static_cast<unsigned>(std::max(from, to));
  Changes crossed = None;
  for (unsigned at = low + 1; at <= high; ++at)
  {
    crossed = static_cast<Changes>(crossed | (1U << at));
  }
  return crossed;
}

auto Expand(std::string_view layout) -> std::string
{
  std::string out;
  for (const std::string_view token : CollectTokens(layout))
  {
    const auto *const fragment = std::ranges::find(Fragments, token, &Fragment::Name);
    out += fragment != Fragments.end() ? Expand(fragment->Layout) : std::string{token};
    out += ' ';
  }
  return out;
}

auto IsNetworked(const Object &object) -> bool
{
  const auto *unknown = GetIf<Unknown>(&object);
  const std::string_view name = unknown != nullptr ? unknown->Class.View() : std::string_view{};
  return name.starts_with("ACDBASSOC") || name.ends_with("DEPENDENCYBODY");
}

auto IsGraph(const Object *object) -> bool
{
  return object != nullptr && Holds<Unknown>(*object) && TypeOf(*object) == "ACAD_EVALUATION_GRAPH";
}

/**
 *  A solid's history owns its evaluation graph.
 */
auto IsHistory(const Object &object) -> bool
{
  return TypeOf(object) == "ACSH_HISTORY_CLASS";
}

/**
 *  Null for a class not listed, or for none: a type of chapter 20.3 dwgio does not type has no class.
 */
auto LineageOf(std::string_view name) -> const Lineage *
{
  if (name.empty())
  {
    return nullptr;
  }
  for (const auto &lineages : {std::span<const Lineage>{Lineages}, std::span<const Lineage>{Remade}})
  {
    const auto found = std::ranges::find(lineages, name, &Lineage::Class);
    if (found != lineages.end())
    {
      return &*found;
    }
  }
  return nullptr;
}
} // namespace

auto Carry(const Unknown &unknown, const CarryContext &with) -> std::optional<Opaque>
{
  if (unknown.Body && unknown.Body->Release == with.To)
  {
    return unknown.Body;
  }
  const Lineage *lineage = LineageOf(unknown.Class.View());
  if (!unknown.Body || unknown.Body->Release != with.From || lineage == nullptr)
  {
    return std::nullopt;
  }
  if ((lineage->Changed & ChangesBetween(with.From, with.To)) == None)
  {
    // The same bits, unless the object has text a crossing of R2007 moves.
    if ((with.From >= Version::R2007) != (with.To >= Version::R2007) && unknown.Body->Strings.Size != 0)
    {
      return std::nullopt;
    }
    Opaque own = *unknown.Body;
    own.Release = with.To;
    return own;
  }
  if (lineage->Layout.empty() || (lineage->Unread & ChangesBetween(with.From, with.To)) != None)
  {
    return std::nullopt;
  }
  return Recast(*unknown.Body, Expand(lineage->Layout),
                RecastContext{.To = with.To, .Text = with.Text, .Given = with.Given, .Taken = with.Taken});
}
} // namespace DwgIO::Releases

namespace DwgIO::Releases
{
auto CollectTorn(const Drawing &drawing, std::span<const Handle> leftOut) -> std::vector<Handle>
{
  std::unordered_set<std::uint64_t> out;
  std::unordered_set<std::uint64_t> graphs;
  for (const Handle one : leftOut)
  {
    out.insert(one.Value);
  }
  for (const Object &object : drawing.Objects)
  {
    if (IsGraph(&object))
    {
      graphs.insert(BaseOf(object).Self.Value);
    }
  }
  bool network = false;
  std::unordered_set<std::uint64_t> torn;
  for (const Object &object : drawing.Objects)
  {
    const Base &base = BaseOf(object);
    if (out.contains(base.Self.Value))
    {
      network = network || IsNetworked(object);
      const std::uint64_t graph = graphs.contains(base.Self.Value) ? base.Self.Value : base.Owner.Value;
      if (graphs.contains(graph))
      {
        torn.insert(graph);
      }
    }
  }
  // A solid's history is left out with its graph: AutoCAD refuses a history that owns a graph the file lacks, and
  // keeps the solid without either.
  std::unordered_set<std::uint64_t> owners;
  for (const Object &object : drawing.Objects)
  {
    if (torn.contains(BaseOf(object).Self.Value))
    {
      owners.insert(BaseOf(object).Owner.Value);
    }
  }
  std::vector<Handle> with;
  for (const Object &object : drawing.Objects)
  {
    const Base &base = BaseOf(object);
    if ((network && IsNetworked(object)) || torn.contains(base.Self.Value) || torn.contains(base.Owner.Value) ||
        (owners.contains(base.Self.Value) && IsHistory(object)))
    {
      with.push_back(base.Self);
    }
  }
  return with;
}

auto KinOf(const Object &object, const Drawing &drawing) -> std::string_view
{
  if (IsNetworked(object))
  {
    return "associative network";
  }
  if (Holds<Unknown>(object) && (IsGraph(&object) || IsHistory(object) || IsGraph(Find(drawing, BaseOf(object).Owner))))
  {
    return "evaluation graph";
  }
  return {};
}
} // namespace DwgIO::Releases
