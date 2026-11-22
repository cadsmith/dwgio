# Auditing a drawing

`Audit::Check` finds what AutoCAD would refuse or repair in a drawing, and `Audit::Repair` fixes it. The examples break a drawing, check it, and repair it before saving. A drawing made with `Create` and filled with `Edit` passes the check unchanged. The audit is for drawings written by other programs, for a `Drawing` whose objects were edited directly, and for mistakes like these.

## Break a drawing {#break}

`Edit::Add` gives an entity a handle, an owner and a place in its chain, but accepts whatever layer, linetype or block it names. That is enough to break a new drawing four ways: a handle seed below the handles in use, a line on a missing layer, a line with a missing linetype, and an insert of a missing block.

```cpp
DwgIO::Drawing drawing = DwgIO::Create();
if (!DwgIO::Edit::Add(drawing, DwgIO::Entities::Line{}))
{
  return 1;
}
DwgIO::Entities::Line wallLine;
wallLine.Common.Layer = DwgIO::Handle{.Value = 0x999};
DwgIO::Entities::Line dashedLine;
dashedLine.Common.LinetypeOrigin = DwgIO::Origin::Record;
dashedLine.Common.Linetype = DwgIO::Handle{.Value = 0x999};
DwgIO::Entities::Insert doorInsert;
doorInsert.Block = DwgIO::Handle{.Value = 0x999};
if (!DwgIO::Edit::Add(drawing, wallLine) || !DwgIO::Edit::Add(drawing, dashedLine) || !DwgIO::Edit::Add(drawing, doorInsert))
{
  return 1;
}
drawing.Header.Handseed = DwgIO::Handle{.Value = 5};
```

## Check {#check}

`Audit::Check` leaves the drawing unchanged and lists its findings in the order the checks run: handles and the seed, tables and dictionaries, names, references, chains, then classes and the remaining checks. An error is something AutoCAD would refuse or recover the drawing for, or that silently loses an object; with only warnings, AutoCAD opens the drawing without changing it. `IsClean` is false while an error is unfixed.

```cpp
const auto print = [](const DwgIO::Audit::Report &report) -> void
{
  for (const DwgIO::Audit::Finding &finding : report.Findings)
  {
    std::println("  {} {}{}", finding.Level == DwgIO::Audit::Severity::Error ? "error  " : "warning", finding.What,
                 finding.Fixed ? ", fixed" : "");
  }
  std::println("  clean: {}", report.IsClean());
};

std::println("Check:");
print(DwgIO::Audit::Check(drawing));
```

```text [Output]
Check:
  error   handle seed 5 not past 2E
  error   layer 999 of LINE 2C missing
  error   linetype 999 of LINE 2D missing
  error   block 999 of INSERT 2E missing, left out
  clean: false
```

## Repair {#repair}

`Audit::Repair` runs the same checks and fixes what they find, in place. The seed moves past the highest handle, the first line moves to layer 0, the second draws ByLayer, and the insert is removed and the model space chain closed over it.

> [!IMPORTANT]
> `Repair` erases what it removes from `Drawing::Objects`, which [invalidates](/guide/objects/#handles) pointers the view gave before. Ask the view again.

```cpp
std::println("Repair:");
print(DwgIO::Audit::Repair(drawing));

const DwgIO::Tables::BlockRecord *model = DwgIO::View::ModelSpace(drawing);
for (const DwgIO::Object *entity : DwgIO::View::Entities(drawing, *model))
{
  std::println("  {} {}", DwgIO::TypeOf(*entity), DwgIO::HexOf(DwgIO::BaseOf(*entity).Self));
}
```

```text [Output]
Repair:
  error   handle seed 5 not past 2E, fixed
  error   layer 999 of LINE 2C missing, fixed
  error   linetype 999 of LINE 2D missing, fixed
  error   block 999 of INSERT 2E missing, left out, fixed
  clean: true
  LINE 2B
  LINE 2C
  LINE 2D
```

## Save {#save}

A second check of the repaired drawing finds nothing, and neither does AutoCAD's audit of the saved file.

```cpp
std::println("Check again: {} findings", DwgIO::Audit::Check(drawing).Findings.size());

if (const auto saved = DwgIO::SaveFile(drawing, "audited.dwg"); !saved)
{
  std::println("{}", saved.error().What);
  return 1;
}
```

```text [Output]
Check again: 0 findings
```
