# Saving a drawing

`Save` writes a `Drawing` as a DWG file to bytes or a stream; `SaveFile` writes it to a path. Every release from R14 to R2018 is written. A loaded drawing saves back to its own release, byte for byte in R14 and R2000 and section for section from R2004, and to any other release with the differences between the two translated.

## Save {#save}

```cpp
const auto loaded = DwgIO::LoadFile("foundation-r2000.dwg");
if (!loaded)
{
  return 1;
}
const auto saved = DwgIO::Save(loaded->Drawing);
if (!saved)
{
  std::println("{}", saved.error().What);
  return 1;
}
std::println("{} bytes, {} issues", saved->Bytes.size(), saved->Issues.size());
```

```text [Output]
138569 bytes, 0 issues
```

`SaveOptions::Release` picks the release to write; when empty, the drawing's own release is written.

```cpp
const auto savedAsR2007 = DwgIO::Save(loaded->Drawing, {.Release = DwgIO::Version::R2007});
std::println("{}, {} issues", DwgIO::NameOf(DwgIO::Load(savedAsR2007->Bytes)->Drawing.Release), savedAsR2007->Issues.size());
```

```text [Output]
R2007, 0 issues
```

## What a save keeps {#keeps}

A save to the drawing's own release writes the header variables and the classes from their fields, every object, the object map and the second header computed over the new offsets, and the blocks dwgio does not type (the auxiliary header, the pad, the measurement and the second header's constant bytes) unchanged. An unchanged R14 or R2000 drawing gives back the original file byte for byte.

Changes are written through. Erase an object from `Drawing::Objects` and the map, the second header and every offset are recomputed; AutoCAD opens the result and its audit finds nothing to fix.

```cpp
DwgIO::Drawing withoutLines = loaded->Drawing;
std::erase_if(withoutLines.Objects, [](const DwgIO::Object &object) { return DwgIO::TypeOf(object) == "LINE"; });
const auto savedWithoutLines = DwgIO::Save(withoutLines);
std::println("{} objects, {} bytes", DwgIO::Load(savedWithoutLines->Bytes)->Drawing.Objects.size(),
             savedWithoutLines->Bytes.size());
```

```text [Output]
218 objects, 138511 bytes
```

## Across releases {#releases}

A typed kind is written from its fields in any release, translated where the releases store the same thing differently:

- an entity's lineweight: a code in R2000, extended data in R14;
- its ByBlock or Continuous linetype: a flag in R2000, the record's handle in R14;
- the dimension arrow blocks: by block record in R2000, by name in R14.

What the target release cannot hold, such as a plot style or a layer's lineweight in R14, is dropped with an issue; [below](#older) is a save that drops more.

An `Unknown` crosses to another release only where AutoCAD writes its class the same way in both or dwgio converts the fields that differ, as for the dynamic block objects across R2007; otherwise it is left out with an issue. [Unknown objects](/guide/objects/#unknown) explains how, and [Compatibility](/concepts/compatibility/#unknown) lists the classes.

R14 stores names in upper case, cutting long ones to 26 characters, and records the original names separately. A save from R14 reads those records, as AutoCAD does, so the names come back whole.

```cpp
const auto r14Drawing = DwgIO::LoadFile("foundation-r14.dwg");
const auto savedAsR2000 = DwgIO::Save(r14Drawing->Drawing, {.Release = DwgIO::Version::R2000});
const auto reloadedR2000 = DwgIO::Load(savedAsR2000->Bytes);
std::print("{} issues, the layouts", savedAsR2000->Issues.size());
for (const DwgIO::Objects::Entry &entry : DwgIO::View::Dictionary(reloadedR2000->Drawing, "ACAD_LAYOUT")->Entries)
{
  std::print(" {}", entry.Name);
}
std::println("");
```

```text [Output]
0 issues, the layouts Layout1 Layout2 Model
```

Saved to R14, a new drawing holds nothing R14 lacks except the layers' plot styles, which are dropped without an issue. An entity with a plot style of its own is noted.

```cpp
DwgIO::Drawing drawing = DwgIO::Create();
DwgIO::Entities::Line line;
line.Common.PlotstyleOrigin = DwgIO::Origin::Record;
line.Common.Plotstyle = DwgIO::Handle{.Value = 0xF};
line.Common.Lineweight = DwgIO::Lineweight::Mm018;
const auto lineHandle = DwgIO::Edit::Add(drawing, line);
if (!lineHandle)
{
  return 1;
}
const auto savedAsR14 = DwgIO::Save(drawing, {.Release = DwgIO::Version::R14});
std::println("{} issue: {}", savedAsR14->Issues.size(), savedAsR14->Issues.front().What);
const auto reloaded = DwgIO::Load(savedAsR14->Bytes);
const DwgIO::Object *lineObject = DwgIO::Find(reloaded->Drawing, *lineHandle);
std::println("{} back as {}, the line's lineweight {}", DwgIO::NameOf(reloaded->Drawing.Release),
             reloaded->Drawing.Objects.size(),
             DwgIO::CommonOf(*lineObject)->Lineweight
                 ? std::string{"in Common"}
                 : std::format("in {} extended data record", DwgIO::BaseOf(*lineObject).Extended.size()));
```

```text [Output]
1 issue: plot style of LINE 2B not kept by R14
R14 back as 40, the line's lineweight in 1 extended data record
```

## R2004 {#r2004}

An R2004 file is paged: its sections are cut into pages, each compressed separately, and located through a page map and a section map. A drawing loaded from one saves back with every section identical once decompressed: the summary from `Drawing::SummaryInfo`, the file dependencies from `Drawing::FileDependencies`, and the sections dwgio does not type, such as the revision history, unchanged. The exception is the application information, which names the program that saved the file: every save writes it again, naming dwgio.

```cpp
const auto r2004Drawing = DwgIO::LoadFile("foundation-r2004.dwg");
const auto savedAsR2004 = DwgIO::Save(r2004Drawing->Drawing);
std::println("{} {} objects, saved in {} bytes with {} issues", DwgIO::NameOf(r2004Drawing->Drawing.Release),
             r2004Drawing->Drawing.Objects.size(), savedAsR2004->Bytes.size(), savedAsR2004->Issues.size());
```

```text [Output]
R2004 211 objects, saved in 54988 bytes with 0 issues
```

Saved as R2000, untyped objects are carried where their class crosses and left out with an issue where it does not; what R2000 cannot hold of the typed objects is dropped and noted. The sections only a paged file has are removed: the summary moves into the `DWGPROPS` record where R2000 keeps it, and the file dependencies are left out and noted. Saved as R2004, an R2000 drawing gets the three sections AutoCAD requires and an application information naming dwgio; its summary moves from that record into its section, its block chains become lists, and the header variables R2004 added take their defaults.

```cpp
const auto r2004AsR2000 = DwgIO::Save(r2004Drawing->Drawing, {.Release = DwgIO::Version::R2000});
const auto r2000AsR2004 = DwgIO::Save(loaded->Drawing, {.Release = DwgIO::Version::R2004});
std::println("{} issues to R2000, {} to R2004; {} back as {} with {} entities in model space",
             r2004AsR2000->Issues.size(), r2000AsR2004->Issues.size(),
             DwgIO::NameOf(DwgIO::Load(r2000AsR2004->Bytes)->Drawing.Release),
             DwgIO::Load(r2000AsR2004->Bytes)->Drawing.Objects.size(),
             DwgIO::View::Block(DwgIO::Load(r2000AsR2004->Bytes)->Drawing, "*Model_Space")->Entities.size());
```

```text [Output]
0 issues to R2000, 0 to R2004; R2004 back as 218 with 4 entities in model space
```

## R2010 to R2018 {#later}

R2010, R2013 and R2018 are paged like R2004. Inside each object the text moves to a stream of its own, and the kinds gain the fields those releases added. A drawing loaded from one saves back with every section identical, and saves as any other release: what the target lacks is dropped, what it adds takes AutoCAD's defaults, and a pointer to an object left out is cleared with an issue. R2007 holds its objects as R2010 does except for the type code, in a container of its own coded against damage.

## To an older release {#older}

A save to an older release drops what that release has no place for, and notes each value the drawing held. `curves-r2018.dwg` uses R2018's transparency and data store, which R14 lacks. Save it as R14 and count the issues by what they say, without the handle:

```cpp
const auto r2018Drawing = DwgIO::LoadFile("curves-r2018.dwg");
const auto r2018AsR14 = DwgIO::Save(r2018Drawing->Drawing, {.Release = DwgIO::Version::R14});
std::map<std::string, int> counts;
for (const DwgIO::Issue &issue : r2018AsR14->Issues)
{
  std::string what = issue.What;
  const std::string handle = " " + DwgIO::HexOf(issue.Subject);
  if (const std::size_t at = what.find(handle); issue.Subject && at != std::string::npos)
  {
    what.replace(at, handle.size(), "");
  }
  counts[what] += 1;
}
for (const auto &[what, count] : counts)
{
  std::println("{:>3} {}", count, what);
}
```

```text [Output]
  2 clip of VIEWPORT not kept by R14
  1 data store left out, the data of 2 objects in it lost
  4 transparency of ARC not kept by R2000
 20 transparency of LINE not kept by R2000
  9 transparency of MTEXT not kept by R2000
 28 transparency of POINT not kept by R2000
 13 transparency of SOLID not kept by R2000
  1 viewports of layout Layout1 not kept by R2000
```

An issue names the release that lacks the value: transparency is dropped below R2004, so the R14 save notes it as not kept by R2000. Saved as R2013, the same drawing has no issue. [Compatibility](/concepts/compatibility/) lists what each step between releases converts, drops and notes.
