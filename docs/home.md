# Home

## Install {#install}

::: code-group

```cmake [CMake]
add_subdirectory(dwgio)
target_link_libraries(app PRIVATE dwgio::dwgio)
```

```cpp [C++]
#include "dwgio/dwgio.hpp"

auto loaded = DwgIO::LoadFile("plan.dwg");
```

:::

## Example {#example}

::: code-group

```cpp [Load]
#include "dwgio/dwgio.hpp"

auto loaded = DwgIO::LoadFile("plan.dwg");
if (!loaded)
{
  std::println("byte {}: {}", loaded.error().Offset.value_or(0), loaded.error().What);
  return 1;
}
DwgIO::Drawing &drawing = loaded->Drawing;
std::println("{} objects, {} classes, {} issues", drawing.Objects.size(),
             drawing.Classes.size(), loaded->Issues.size());
```

```cpp [Walk the objects]
for (const DwgIO::Object &object : drawing.Objects)
{
  const DwgIO::Base &base = DwgIO::BaseOf(object);
  std::println("{:>6} {} owned by {}", DwgIO::HexOf(base.Self), DwgIO::TypeOf(object),
               DwgIO::HexOf(base.Owner));
}
```

```cpp [Save]
const auto saved = DwgIO::SaveFile(drawing, "plan-copy.dwg");
if (!saved)
{
  std::println("{}", saved.error().What);
  return 1;
}
```

:::
