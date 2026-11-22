# Getting started

dwgio is a C++23 library that reads and writes AutoCAD DWG files of every release from R14 to R2018. It types the common objects, keeps every other object with its handles and bytes, saves a drawing back to its own release, byte for byte in R14 and R2000 and section for section from R2004, or to any other release, and can make a drawing from nothing.

## Add it to a project {#add}

dwgio is a CMake project that depends only on the standard library and needs C++23. With its source in your project, add it and link the target `dwgio::dwgio`:

```cmake [CMakeLists.txt]
add_subdirectory(dwgio)
target_link_libraries(app PRIVATE dwgio::dwgio)
```

[Installing dwgio](/guide/install/) gives the compilers it needs, the other ways to add it (`FetchContent`, or `find_package` after an install) and how to build it as a shared library.

## A first program {#first}

`dwgio/dwgio.hpp` includes the whole interface, in the namespace `DwgIO`. This program reads a drawing, prints what it holds and writes it again. Nothing throws: a failed load or save returns a `Failure` with what went wrong and, for a load, the byte offset.

```cpp [main.cpp]
#include "dwgio/dwgio.hpp"

#include <print>

auto main() -> int
{
  const auto loaded = DwgIO::LoadFile("foundation-r2000.dwg");
  if (!loaded)
  {
    std::println("byte {}: {}", loaded.error().Offset.value_or(0), loaded.error().What);
    return 1;
  }
  const DwgIO::Drawing &drawing = loaded->Drawing;
  std::println("{} {} objects, {} issues", DwgIO::NameOf(drawing.Release),
               drawing.Objects.size(), loaded->Issues.size());

  const auto saved = DwgIO::SaveFile(drawing, "first.dwg");
  if (!saved)
  {
    std::println("{}", saved.error().What);
    return 1;
  }
}
```

```text [Output]
R2000 219 objects, 0 issues
```

## What comes back {#loaded}

`Load` returns a `Loaded`: the `Drawing` and the `Issues` found while reading, each with its byte offset and the handle of the object it concerns. An issue is something the reader recovered from, such as a CRC mismatch, an unknown code page or bytes no part of the file claims. A `Failure` means there is no drawing. [Errors and issues](/guide/errors/) lists what makes a load fail and shows how to report both.

## Where to go next {#next}

The guide:

- [The object model](/guide/objects/): what a `Drawing` holds, how to read its objects and follow their handles.
- [Errors and issues](/guide/errors/): what a failed load, save or edit says, and what a successful one notes.

The tutorials, one task each:

- [Reading a drawing](/tutorials/reading/): the objects, their handles and the common entity data.
- [Model space and layouts](/tutorials/spaces/), [Blocks and attributes](/tutorials/blocks/), [Coordinates and units](/tutorials/coordinates/), [Layers, colors and styles](/tutorials/styles/): what the entities are drawn in, with and where.
- [Creating a drawing](/tutorials/creating/): a new drawing with a layer, a block, a line and an insert.
- [Editing a drawing](/tutorials/editing/): changing, adding, copying and removing objects in a loaded drawing.
- [Auditing a drawing](/tutorials/auditing/): finding and repairing what AutoCAD would complain about before saving.
- [Saving a drawing](/tutorials/saving/): what a save keeps, what it changes, and the releases it writes.

The [entities](/entities/curves/), one page per family, say what each kind holds. The concepts explain the model behind the code:

- [The drawing](/concepts/drawing/): how a `Drawing` maps to the file.
- [Releases](/concepts/releases/): what each DWG release changed.
- [Compatibility](/concepts/compatibility/): what a save to another release converts, drops and notes.
- [Quality](/concepts/quality/): how dwgio is tested, and how fast it is.
- [Limits](/concepts/limits/): threads, memory, and what dwgio does not do.

The [reference](/reference/) lists every type and function of the interface.
