# Errors and issues

dwgio reports two things. A `Failure` means the call gave no result: a failed load returns no drawing, a failed save writes no file, and a refused edit leaves the drawing unchanged. An `Issue` comes with a result: the load or save went through, and the issue says what it repaired, kept or left out on the way.

## Failures {#failure}

`Load`, `LoadFile`, `Save`, `SaveFile` and the `Edit` functions return a `std::expected` whose error is a `Failure`: `Code` says what kind it is, `What` describes the failure, and `Offset` is the byte of the file at fault, when one is. None of them throws for a bad file or a refused edit.

| `FailureCode` | When |
| :--- | :--- |
| `Io` | A stream or a file that cannot be read or written |
| `NotDwg` | Bytes that are not a DWG file |
| `UnsupportedRelease` | A release other than R14 to R2018 |
| `Encrypted` | A drawing saved with a password |
| `Damaged` | A file too damaged to read, or an edit that meets a broken chain |
| `Missing` | Something an edit needs, such as a table, a block or a handle |
| `Exists` | A name or a handle an edit would give is taken |
| `InUse` | An object that a removal would delete and that other objects still need |
| `Refused` | An edit the drawing cannot take, such as removing layer 0 |

A load fails only when it can read nothing: a file that does not open, bytes that are not a DWG, a release other than R14 to R2018, a file encrypted with a password, or a header or classes section past the end of the file.

```cpp
const auto missing = DwgIO::LoadFile("missing.dwg");
std::println("{}", missing.error().What);
const std::string_view text = "not a drawing";
const auto parsed = DwgIO::Load(std::as_bytes(std::span{text}));
std::println("{}, not a DWG: {}", parsed.error().What, parsed.error().Code == DwgIO::FailureCode::NotDwg);
```

```text [Output]
cannot open missing.dwg
file too short, not a DWG: true
```

An R2000 drawing saves back to its own release byte for byte, so the examples below damage the bytes of a save instead of a file on disk. Cut short before its header variables, the file fails at the offset where they should be.

```cpp
const auto loaded = DwgIO::LoadFile("foundation-r2000.dwg");
if (!loaded)
{
  return 1;
}
const auto saved = DwgIO::Save(loaded->Drawing);
if (!saved)
{
  return 1;
}
const std::span<const std::byte> bytes = saved->Bytes;
const auto shortened = DwgIO::Load(bytes.first(10000));
std::println("byte {}: {}", shortened.error().Offset.value_or(0), shortened.error().What);
```

```text [Output]
byte 20187: header section beyond the file
```

A save fails only when it cannot write: a stream that fails, or a file that does not open.

```cpp
const auto failedSave = DwgIO::SaveFile(loaded->Drawing, "no/such/folder/plan.dwg");
std::println("{}", failedSave.error().What);
```

```text [Output]
cannot write no/such/folder/plan.dwg
```

## Refused edits {#edits}

An edit that would break the drawing is refused, and the drawing is left unchanged. `Edit::Add` refuses a table record under a name its table already has, ignoring case, and an entity for a block the drawing lacks; `Edit::Remove` refuses what a drawing needs, such as layer `0` or a space, and anything still in use.

```cpp
DwgIO::Drawing drawing = DwgIO::Create();
DwgIO::Tables::Layer walls;
walls.Record.Name = "Walls";
const auto layer = DwgIO::Edit::Add(drawing, walls);
DwgIO::Entities::Line wall;
wall.Common.Layer = layer.value_or(DwgIO::Handle{});
const auto line = DwgIO::Edit::Add(drawing, wall);
if (!layer || !line)
{
  return 1;
}
walls.Record.Name = "WALLS";
std::println("{}", DwgIO::Edit::Add(drawing, walls).error().What);
const DwgIO::Handle zero = DwgIO::View::Record<DwgIO::Tables::Layer>(drawing, "0")->Record.Base.Self;
std::println("{}", DwgIO::Edit::Remove(drawing, zero).error().What);
std::println("{}", DwgIO::Edit::Remove(drawing, *layer).error().What);
std::println("{}", DwgIO::Edit::Add(drawing, wall, "Door").error().What);
```

```text [Output]
layer WALLS exists
layer 0 required
layer Walls in use
block Door missing
```

Once the line is removed, nothing uses the layer, and it can be removed too.

```cpp
if (!DwgIO::Edit::Remove(drawing, *line) || !DwgIO::Edit::Remove(drawing, *layer))
{
  return 1;
}
for (const DwgIO::Tables::Layer *record : DwgIO::View::Records<DwgIO::Tables::Layer>(drawing))
{
  std::println("layer {}", record->Record.Name);
}
```

```text [Output]
layer 0
```

## Issues {#issues}

`Loaded::Issues` and `Saved::Issues` list what the call noted and recovered from. Each `Issue` has a `Code`; the byte `Offset` it concerns, none in a save; the handle of the `Subject` it concerns, null when it concerns no one object; and `What`. A file AutoCAD wrote loads with none.

| `IssueCode` | What was noted |
| :--- | :--- |
| `Damaged` | Structure the reader found broken, then skipped or repaired |
| `Truncated` | Data cut short; what fit is kept |
| `Checksum` | A CRC or checksum that does not match |
| `Unknown` | A value or a kind dwgio does not know, kept or skipped |
| `OutOfRange` | A value the writer could not write unchanged, so it was clamped or cut |
| `NotKept` | Something the target release or file cannot hold, left out |
| `Moved` | A block the writer placed elsewhere than `Drawing::Order` said |

A load notes what it repairs or leaves out and carries on: a CRC mismatch, a wrong sentinel, a block past the end of the file, bytes no block claims. A file cut after its header loses its object map, and the reader walks the objects instead, keeping those before the cut.

```cpp
const auto report = [](const std::vector<DwgIO::Issue> &issues) -> void
{
  for (const DwgIO::Issue &issue : issues)
  {
    std::string where = issue.Offset ? std::format("byte {}", *issue.Offset) : "";
    if (issue.Subject)
    {
      where += (where.empty() ? "object " : ", object ") + DwgIO::HexOf(issue.Subject);
    }
    std::println("  {}: {}", where, issue.What);
  }
};
const auto truncated = DwgIO::Load(bytes.first(100000));
std::println("{} objects of {}, {} issues", truncated->Drawing.Objects.size(), loaded->Drawing.Objects.size(),
             truncated->Issues.size());
report(truncated->Issues);
```

```text [Output]
194 objects of 219, 3 issues
  byte 137708: map block beyond the file
  byte 138565: measurement block beyond the file
  byte 21723: object map missing, 194 objects walked
```

An object whose bytes do not match their CRC is kept and noted with its handle. To see it, find the bytes of the section view style in the file, which its `Unknown` keeps, and flip one bit:

```cpp
const auto &style = DwgIO::Get<DwgIO::Unknown>(*DwgIO::Find(loaded->Drawing, DwgIO::ParseHandle("21A").value()));
std::vector<std::byte> damaged{bytes.begin(), bytes.end()};
const auto at = std::ranges::search(damaged, style.Bytes).begin();
at[20] ^= std::byte{1};
const auto corrupted = DwgIO::Load(damaged);
report(corrupted->Issues);
```

```text [Output]
  byte 31088, object 21A: object 21A CRC mismatch
```

A save writes what the load kept. The truncated file saves as a complete file holding the objects before the cut; the damaged style is kept with its original bytes and CRC, and the next load notes it again.

```cpp
const auto truncatedReloaded = DwgIO::Load(DwgIO::Save(truncated->Drawing)->Bytes);
const auto corruptedReloaded = DwgIO::Load(DwgIO::Save(corrupted->Drawing)->Bytes);
std::println("{} objects, {} issues; {} issue", truncatedReloaded->Drawing.Objects.size(),
             truncatedReloaded->Issues.size(), corruptedReloaded->Issues.size());
```

```text [Output]
194 objects, 0 issues; 1 issue
```

## Saving to another release {#releases}

A save to another release notes what that release cannot hold and leaves it out. R14 has no plot styles, so a line given one loses it:

```cpp
DwgIO::Entities::Line styledLine;
styledLine.Common.PlotstyleOrigin = DwgIO::Origin::Record;
styledLine.Common.Plotstyle = DwgIO::View::Dictionary(drawing, "ACAD_PLOTSTYLENAME")->Entries.front().Object;
if (!DwgIO::Edit::Add(drawing, styledLine))
{
  return 1;
}
const auto savedAsR14 = DwgIO::Save(drawing, {.Release = DwgIO::Version::R14});
report(savedAsR14->Issues);
```

```text [Output]
  object 2D: plot style of LINE 2D not kept by R14
```

[Compatibility](/concepts/compatibility/) lists what each step between releases converts and drops, and [Unknown objects](/guide/objects/#unknown) which untyped objects a save can carry to another release.

## Reporting them {#report}

Show a `Failure` unchanged: `What` is a short phrase in lower case, such as `layer 0 required`, meant to follow a prefix such as `load failed: `, and for a load `Offset` says where in the file to look. Log issues rather than stop for them: the drawing loaded, and what was cut or damaged is left out or kept with its original bytes. A file AutoCAD wrote loads with no issues, so issues mean the file was damaged or written by another program. Before saving such a file, run [`Audit::Check`](/tutorials/auditing/): it finds what AutoCAD would complain about, which the reader does not look for.
