# C++ Guidelines

## Rules

- Functions and methods must not accept more than 3 parameters.
- Each class must have a single, well-defined responsibility.
- Each file must be dedicated to a single responsibility.
- File names must consist of a single word describing their job.
- File names must be lowercase.
- Directory hierarchies must be used instead of compound file names.
- Namespaces must reflect the directory hierarchy.
- Private member variables must be prefixed with `m_`.
- Namespaces, types, functions, members and constants must use PascalCase.
- Arguments and local variables must use camelCase.
- Template type parameters must end in `T`.
- A type that holds a kind of object takes the name DXF gives the kind,
  split into the words and abbreviations it is made of, each capitalised,
  without underscores or an `ACDB` prefix; a leading digit moves to the end:
  `MText`, `XLine`, `AttDef`, `Attrib`, `SeqEnd`, `Block` and `EndBlk`,
  `LType`, `VPort`, `DimStyle`, `AppId`, `LwPolyline`, `AcadTable`,
  `ImageDefReactor`, `Placeholder`, `Face3d`. An abbreviation stays one word:
  `Attrib`, not `AttRib`. It stays abbreviated when AutoCAD uses it on its own,
  as a command, a system variable or a table, so a reader takes it as a word:
  `LType`, `VPort`, `DimStyle`, `AttDef`, `MText`, `SeqEnd`, `EndBlk`. One that
  packs several words into a few letters found nowhere else is spelled out. A
  kind that differs from another only by a member is that kind with the member:
  `ACDBDICTIONARYWDFLT` is a `Dictionary` whose `Default` is set. A kind DXF
  does not export spells out the words of its name in the Open Design
  Specification for .dwg files: `VP_ENT_HDR` is `ViewportEntityHeader`. The
  names are the ones a DXF file, AutoCAD's reference and the specification
  use, and no spelled-out form holds without an exception:
  `Viewport` is the VIEWPORT entity, not a VPORT.
- Any other public type is named in whole words for what it holds, never with
  a name a kind takes: `Section`, `PreviewImage`, `Tag`, `SaveOptions`.
- A header variable takes its DXF name without the `$`, only its first
  letter capitalised: `$DIMSTYLE` is `Dimstyle` and `$TEXTSTYLE` is `Textstyle`. An
  item of the header DXF does not write is named by the specification's words,
  a kind's name written as the type is: `LTypeByLayer`, `VPortControlObject`.
- A member the specification leaves unnamed is `Unknown` and its place among
  the record's unnamed items, counted from 1 in the file's order: `Unknown1`,
  `Unknown2`. A number stays with its item, so naming one leaves a gap. Its
  note opens with "Unnamed in the specification" and says what AutoCAD writes.
- A namespace that holds the parts of one type takes that type's name in the
  plural: `Objects::MLeaderStyles` for `MLeaderStyle`, `Objects::TableStyles`
  for `TableStyle`, `Entities::Edges` for the alternatives of `Edge`.
- A function that gives one property of its argument is named for the
  property followed by `Of`: `TypeOf`, `NameOf`, `ReleaseOf`. One that reads
  text into a value is named `Parse` and the value's type: `ParseHandle`.
- Any other function is named by a verb in the imperative and what it acts on,
  never by a past participle or a gerund, one verb per role:

  | Role | Verb | Examples |
  | :--- | :--- | :--- |
  | Decodes bits or bytes | `Read` | `ReadHead`, `ReadObjects` |
  | Encodes bits or bytes | `Write` | `WriteHead`, `WriteClasses` |
  | Looks up what may be missing, null or empty for none | `Find` | `FindRecord`, `FindScheme` |
  | Gathers a list | `Collect` | `CollectLeftOut`, `CollectOwned` |
  | Builds a value and gives it | `Make` | `MakeLayout`, `MakeScheme` |
  | Brings a value to another release or form | `Convert`, `Raise` up, `Lower` down | `ConvertObject`, `RaiseLeader` |
  | Changes its argument in place | the change | `ClearPointers`, `Relink`, `Bridge` |
  | Notes an issue or a finding | `Note` | `NoteFixed`, `NoteWarning` |
  | Checks what may fail a walk | `Check` | `CheckCount`, `CheckSequence` |

- A function that answers yes or no starts with `Is`, `Has` or `Can`, or is a
  verb in the third person: `IsRequired`, `HasGuid`, `MovesHistory`.
- A type is named in nouns for what it holds, an argument struct too:
  `Fields::Input`, `CarryContext`, `Ownership`, never `Reading` or `Walked`.

## Comments

- A comment says only what reading the code cannot: a DWG format fact, what
  AutoCAD does or refuses, a unit, range, sentinel or coordinate system, a
  contract (what is refused, what a failure or a null means, what is noted as an
  issue, how long a pointer stays valid), a precondition, the reason for code
  that looks wrong. A declaration whose name and type say it all has no doc
  block, and a member whose name says it all has no note.
- A chapter a comment cites is one of the Open Design Specification for .dwg
  files; a doc block that would only cite the chapter its file's brief cites
  is left out.
- A file opens with a framed banner: `@file`, a `@brief` naming its one
  responsibility, `@copyright`.
- A doc block on a declaration: a summary, details after an empty line, then
  its tags as one aligned table, a tag only where it adds to the signature.
- A note on a member or an enumerator is a trailing `/**< */`.
- Inside a function, a short `//` line gives a reason or a format fact.
- `python3 -I tools/format/style.py` formats them, `--check` lists what it
  would change.

```cpp
/**********************************************************************************************************************
 *  @file      dwgio/entities/arc.hpp
 *  @brief     The ARC entity.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 *********************************************************************************************************************/

namespace DwgIO::Entities
{
/**
 *  An ARC: drawn counterclockwise from its start angle to its end angle.
 */
struct DWGIO_API Arc
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  double Thickness{};
  Vector Center{}; /**< In the object coordinate system Extrusion defines; its Z is the elevation. */
  double Radius{};
  Vector Extrusion{Up};
  double Start{}; /**< In radians; group 50 gives degrees. */
  double End{};   /**< In radians; group 51 gives degrees. */
};
} // namespace DwgIO::Entities

namespace DwgIO
{
/**
 *  Reads a DWG file of any release from R14 to R2018.
 *
 *  Anything the reader can skip or repair becomes an issue, and a file cut short keeps the objects before the cut.
 *
 *  @return A failure on another release, on bytes that are no DWG, or on missing header variables or classes.
 */
[[nodiscard]] DWGIO_API auto Load(std::span<const std::byte> bytes) -> std::expected<Loaded, Failure>;
} // namespace DwgIO
```

## Examples

```cpp
namespace Foo
{
void Write(Bar bar);
void Write(Bar bar, Baz baz);
void Write(Bar bar, Baz baz, Qux qux);

class Bar
{
public:
  [[nodiscard]] auto IsEmpty() const -> bool;

private:
  bool m_Empty{};
};
} // namespace Foo

namespace Foo::Bar
{
class Baz
{
public:
  [[nodiscard]] auto HasQux() const -> bool;

private:
  bool m_Qux{};
};
} // namespace Foo::Bar
```

```txt
foo/bar.hpp
foo/baz.hpp

foo/bar/baz.hpp
foo/bar/qux.hpp
```
