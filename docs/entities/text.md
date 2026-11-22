# Text

TEXT, ATTDEF and ATTRIB hold one line of text and its placing in a `Lettering`; MTEXT holds paragraphs with formatting codes in its value. The examples read `entities-r2000.dwg`, which holds a plain text, a centered one in the style `Notes`, one written backwards, a two-line MTEXT, and a block with two attribute definitions inserted with one attribute, and `curves-r2000.dwg`, whose dimensions draw their text as MTEXT.

## Lettering {#lettering}

| Member | Holds |
| :--- | :--- |
| `Value` | The text, with any `%%` codes |
| `Position` | The first alignment point, on the baseline at the text's left, in the object coordinate system, its Z the elevation |
| `Alignment` | The second alignment point, used when the text is not left on its baseline; otherwise the same as `Position` |
| `Height` | The height of capital letters |
| `Rotation` | The angle of the baseline, in radians |
| `Width` | The width factor: 1 for the font's own width, less to narrow it |
| `Oblique` | The slant of the letters, in radians from the vertical |
| `Style` | The `Tables::Style` record that names the font |
| `Generation` | 2 written backwards, 4 upside down |
| `Horizontal`, `Vertical` | The justification: see below |
| `Thickness`, `Extrusion` | The extrusion along the normal, and the normal that sets the OCS |

The text's own `Width` and `Oblique` are those it is drawn with: AutoCAD copies the style's when the text is made, and a later change to the style changes neither. `Horizontal` (group 72) and `Vertical` (group 73) together name AutoCAD's justifications:

| `Vertical` | `Horizontal` `Left` | `Center` | `Right` |
| :--- | :--- | :--- | :--- |
| `Top` | TL | TC | TR |
| `Middle` | ML | MC | MR |
| `Bottom` | BL | BC | BR |
| `Baseline` | Left | Center | Right |

With `Vertical` `Baseline`, `Horizontal` may also be `Aligned`, `Middle` or `Fit`. A Left text stands at `Position`. An Aligned or Fit text fills the baseline from `Position` to `Alignment`. Any other stands at `Alignment`, and `Position` is where AutoCAD worked out the baseline starts.

```cpp
const auto entities = DwgIO::LoadFile("entities-r2000.dwg");
if (!entities)
{
  return 1;
}
const DwgIO::Drawing &drawing = entities->Drawing;
const auto justified = [](const DwgIO::Entities::Lettering &lettering) -> std::string
{
  static constexpr std::array<std::string_view, 6> across{"left", "center", "right", "aligned", "middle", "fit"};
  static constexpr std::array<std::string_view, 4> rows{"baseline", "bottom", "middle", "top"};
  const auto horizontal = static_cast<std::size_t>(lettering.Horizontal);
  const auto vertical = static_cast<std::size_t>(lettering.Vertical);
  return lettering.Vertical == DwgIO::Entities::VerticalAlignment::Baseline
             ? std::string(across.at(horizontal))
             : std::format("{} {}", rows.at(vertical), across.at(horizontal));
};
const auto degrees = [](double radians) -> double { return std::round(radians * 360 / DwgIO::Turn); };
const DwgIO::Tables::BlockRecord *model = DwgIO::View::ModelSpace(drawing);
for (const DwgIO::Object *entity : DwgIO::View::Entities(drawing, *model))
{
  if (const auto *text = DwgIO::GetIf<DwgIO::Entities::Text>(entity))
  {
    const DwgIO::Entities::Lettering &lettering = text->Lettering;
    const auto &style = DwgIO::Get<DwgIO::Tables::Style>(*DwgIO::Find(drawing, lettering.Style));
    const bool left = lettering.Horizontal == DwgIO::Entities::HorizontalAlignment::Left &&
                      lettering.Vertical == DwgIO::Entities::VerticalAlignment::Baseline;
    const DwgIO::Vector at = left ? lettering.Position : lettering.Alignment;
    std::println("\"{}\": {} at ({}, {}), height {}, turned {}, style {}, width {}, oblique {}, generation {}",
                 lettering.Value, justified(lettering), at.X, at.Y, lettering.Height, degrees(lettering.Rotation),
                 style.Record.Name, lettering.Width, degrees(lettering.Oblique), lettering.Generation);
  }
}
```

```text [Output]
"Plain text": left at (0, 150), height 5, turned 0, style Notes, width 0.8, oblique 15, generation 0
"Centered": middle center at (50, 150), height 5, turned 30, style Notes, width 0.8, oblique 15, generation 0
"Mirror": left at (0, 160), height 5, turned 0, style Standard, width 1, oblique 0, generation 2
```

The style `Notes`, 0.8 wide and slanted 15 degrees, was current for the first two texts, and `Standard` for the third, which the script then turned to generation 2, written backwards. The centered text stands at its alignment point (50, 150); its `Position` is (41.296, 142.088), where its baseline starts.

## Attribute definitions and attributes {#attributes}

An `AttDef` and an `Attrib` hold a `Lettering` and a `Tag`; the definition adds the `Prompt` AutoCAD asks with, and its `Lettering.Value` is the default. `Flags` 1 hides the attribute, 2 makes it constant, 4 has AutoCAD verify it and 8 presets it. A constant definition gets no ATTRIB: its value is the definition's for every insert. `Locked`, from R2007, keeps the attribute where the block puts it.

```cpp
const DwgIO::Tables::BlockRecord *tag = DwgIO::View::Block(drawing, "Tag");
for (const DwgIO::Object *entity : DwgIO::View::Entities(drawing, *tag))
{
  if (const auto *definition = DwgIO::GetIf<DwgIO::Entities::AttDef>(entity))
  {
    std::println("ATTDEF {}: prompt \"{}\", default \"{}\", flags {}", definition->Tag, definition->Prompt,
                 definition->Lettering.Value, definition->Flags);
  }
}
for (const DwgIO::Object &object : drawing.Objects)
{
  if (const auto *attribute = DwgIO::GetIf<DwgIO::Entities::Attrib>(&object))
  {
    std::println("ATTRIB {} = \"{}\" on the insert {}", attribute->Tag, attribute->Lettering.Value,
                 DwgIO::HexOf(attribute->Base.Owner));
  }
}
```

```text [Output]
ATTDEF SIZE: prompt "", default "10", flags 2
ATTDEF NAME: prompt "Name", default "none", flags 0
ATTRIB NAME = "Alice" on the insert 2EF
```

[Blocks and attributes](/tutorials/blocks/#attributes) reads an insert's attributes in order, and adds them. From R2018 `Type` 2 or 4 marks a multiline attribute, which holds an MTEXT; dwgio reads only the single line attribute, `Type` 1, and keeps any other as an `Unknown`.

## MTEXT {#mtext}

An `MText` stands at `Position` and runs along `Direction`, both in world coordinates; `Extrusion` is the normal of its plane. `Height` is the height of its letters and `Width` the width it wraps at, 0 for no wrapping. `Attachment` says which point of the text `Position` is, from 1 at the top left to 9 at the bottom right, row by row. `ExtentWidth` and `ExtentHeight` are the size AutoCAD drew it at, and `LineSpacingStyle` and `LineSpacingFactor` set the line spacing.

```cpp
const auto curves = DwgIO::LoadFile("curves-r2000.dwg");
if (!curves)
{
  return 1;
}
static constexpr std::array<std::string_view, 9> corners{
    "top left", "top center", "top right", "middle left", "middle center", "middle right",
    "bottom left", "bottom center", "bottom right"};
const auto &lines = DwgIO::Get<DwgIO::Entities::MText>(*DwgIO::Find(drawing, DwgIO::ParseHandle("2E4").value()));
std::println("MTEXT 2E4: {} at ({}, {}), height {}, wraps at {}, drawn {:.2f} by {:.2f}",
             corners.at(lines.Attachment - 1), lines.Position.X, lines.Position.Y, lines.Height, lines.Width,
             lines.ExtentWidth, lines.ExtentHeight);
for (const char *handle : {"3D2", "3F9"})
{
  const auto &mtext =
      DwgIO::Get<DwgIO::Entities::MText>(*DwgIO::Find(curves->Drawing, DwgIO::ParseHandle(handle).value()));
  std::println("MTEXT {}: {} at ({:.4f}, {:.4f}), turned {} degrees", handle, corners.at(mtext.Attachment - 1),
               mtext.Position.X, mtext.Position.Y, degrees(std::atan2(mtext.Direction.Y, mtext.Direction.X)));
}
```

```text [Output]
MTEXT 2E4: top left at (0, 200), height 5, wraps at 100, drawn 35.90 by 13.50
MTEXT 3D2: middle center at (46.3445, -13.6555), turned -45 degrees
MTEXT 3F9: middle center at (112.2023, -13.3187), turned 21 degrees
```

A dimension's text is an MTEXT in the dimension's block, attached at its middle center and turned along the dimension line, as 3D2 and 3F9 are.

## MTEXT formatting codes {#codes}

`Value` holds the text with its formatting codes, as DXF's groups 3 and 1 do joined. The common ones:

| Code | Means |
| :--- | :--- |
| `\P` | A new paragraph |
| `\N`, `\X` | A new column; the line break of a dimension text between above and below the line |
| `\~` | A space that does not break |
| `\\`, `\{`, `\}` | A backslash, a brace |
| `{` ... `}` | A group: what is set inside ends at the closing brace |
| `\fArial\|b1\|i0;`, `\FISOCP;` | The font, TrueType or SHX, up to the semicolon |
| `\H2.5;`, `\H0.7x;` | The height, absolute or relative |
| `\W0.8;`, `\Q15;`, `\T1.2;`, `\A1;` | The width factor, the slant, the tracking, the vertical alignment |
| `\C1;`, `\c16711680;` | The color, by index or by RGB |
| `\L` `\l`, `\O` `\o`, `\K` `\k` | Underline, overline, strike through, on and off |
| `\S1/2;`, `\S1^2;`, `\S1#2;` | A stacked fraction, tolerance or diagonal fraction |
| `\pi1,l2;` | Paragraph indents and tabs |
| `\U+00B0` | A character by its code point |

AutoCAD writes `\U+` and four hex digits for a character the drawing's code page lacks, and keeps the form in some texts it makes itself: the angular dimension above reads `90\U+00B0`. dwgio turns the escape into the character when the code page lacks it, and leaves the escape otherwise, so that the text saves back to the same bytes.

The function below keeps the words, writing each paragraph on its own line, and drops every formatting code:

```cpp
const auto utf8 = [](char32_t point) -> std::string
{
  std::string out;
  if (point < 0x80)
  {
    out += static_cast<char>(point);
  }
  else if (point < 0x800)
  {
    out += static_cast<char>(0xC0 | (point >> 6));
    out += static_cast<char>(0x80 | (point & 0x3F));
  }
  else
  {
    out += static_cast<char>(0xE0 | (point >> 12));
    out += static_cast<char>(0x80 | ((point >> 6) & 0x3F));
    out += static_cast<char>(0x80 | (point & 0x3F));
  }
  return out;
};
const auto plain = [&utf8](std::string_view value) -> std::string
{
  std::string out;
  for (std::size_t at = 0; at < value.size(); ++at)
  {
    const char one = value[at];
    if (one == '{' || one == '}')
    {
      continue;
    }
    if (one != '\\' || at + 1 == value.size())
    {
      out += one;
      continue;
    }
    const char code = value[++at];
    if (code == 'P' || code == 'N' || code == 'X')
    {
      out += '\n';
    }
    else if (code == '~')
    {
      out += ' ';
    }
    else if (code == '\\' || code == '{' || code == '}')
    {
      out += code;
    }
    else if (code == 'U' && value.substr(at + 1, 1) == "+" && at + 5 < value.size())
    {
      unsigned point = 0;
      std::from_chars(value.data() + at + 2, value.data() + at + 6, point, 16);
      out += utf8(point);
      at += 5;
    }
    else if (code == 'S')
    {
      const std::size_t end = std::min(value.find(';', at), value.size());
      std::string stack(value.substr(at + 1, end - at - 1));
      std::ranges::replace(stack, '^', '/');
      std::ranges::replace(stack, '#', '/');
      out += stack;
      at = end;
    }
    else if (std::string_view("LlOoKk").find(code) == std::string_view::npos)
    {
      at = std::min(value.find(';', at), value.size());
    }
  }
  return out;
};
for (const DwgIO::Object &object : curves->Drawing.Objects)
{
  if (const auto *mtext = DwgIO::GetIf<DwgIO::Entities::MText>(&object); mtext && mtext->Value.contains('\\'))
  {
    std::println("{}: {} -> {}", DwgIO::HexOf(mtext->Base.Self), mtext->Value, plain(mtext->Value));
  }
}
std::println("2E4: {} ->\n{}", lines.Value, plain(lines.Value));
```

```text [Output]
3D2: 90\U+00B0 -> 90°
3E0: 90\U+00B0 -> 90°
2E4: First line\PSecond line ->
First line
Second line
```

To check the other codes, make an MTEXT that uses them, save it and read it back:

```cpp
DwgIO::Drawing made = DwgIO::Create();
DwgIO::Entities::MText note;
note.Position = DwgIO::Vector{.X = 0, .Y = 10, .Z = 0};
note.Height = 2.5;
note.Width = 60;
note.Value = R"({\fArial|b1|i0|c0|p34;Bearing}\P\H0.7x;Bore \S1/2; inch, \Lnot\l to scale\P{\C1;Red} \{and\} a\~b)";
if (!DwgIO::Edit::Add(made, note) || !DwgIO::SaveFile(made, "notes.dwg"))
{
  return 1;
}
const auto notes = DwgIO::LoadFile("notes.dwg");
if (!notes)
{
  return 1;
}
for (const DwgIO::Object &object : notes->Drawing.Objects)
{
  if (const auto *mtext = DwgIO::GetIf<DwgIO::Entities::MText>(&object))
  {
    std::println("{}", plain(mtext->Value));
  }
}
```

```text [Output]
Bearing
Bore 1/2 inch, not to scale
Red {and} a b
```

## Codes in TEXT {#percent}

A TEXT, an ATTDEF and an ATTRIB know none of MTEXT's codes. They take `%%` codes instead, which AutoCAD keeps in the value as typed: `%%d` the degree sign, `%%p` plus or minus, `%%c` the diameter sign, `%%%` a percent sign, `%%u` and `%%o` underline and overline on and off, and `%%` followed by three digits the character of that number.

```cpp
const auto shown = [&utf8](std::string_view value) -> std::string
{
  std::string out;
  for (std::size_t at = 0; at < value.size(); ++at)
  {
    if (!value.substr(at).starts_with("%%") || at + 2 == value.size())
    {
      out += value[at];
      continue;
    }
    at += 2;
    const char code = static_cast<char>(std::tolower(static_cast<unsigned char>(value[at])));
    if (code == 'd' || code == 'p' || code == 'c')
    {
      out += utf8(code == 'd' ? 0xB0 : code == 'p' ? 0xB1 : 0x2205);
    }
    else if (code == '%')
    {
      out += '%';
    }
    else if (std::isdigit(static_cast<unsigned char>(code)) != 0 && at + 2 < value.size())
    {
      unsigned number = 0;
      std::from_chars(value.data() + at, value.data() + at + 3, number);
      out += utf8(number);
      at += 2;
    }
    else if (code != 'u' && code != 'o')
    {
      out += value.substr(at - 2, 3);
    }
  }
  return out;
};
DwgIO::Entities::Text label;
label.Lettering.Value = "%%c50 %%p0.1, 45%%d, %%uAll%%u 100%%%, %%065";
label.Lettering.Height = 2.5;
if (!DwgIO::Edit::Add(made, label) || !DwgIO::SaveFile(made, "notes.dwg"))
{
  return 1;
}
const auto labels = DwgIO::LoadFile("notes.dwg");
if (!labels)
{
  return 1;
}
for (const DwgIO::Object &object : labels->Drawing.Objects)
{
  if (const auto *text = DwgIO::GetIf<DwgIO::Entities::Text>(&object))
  {
    std::println("{} -> {}", text->Lettering.Value, shown(text->Lettering.Value));
  }
}
```

```text [Output]
%%c50 %%p0.1, 45%%d, %%uAll%%u 100%%%, %%065 -> ∅50 ±0.1, 45°, All 100%, A
```

## Fields {#fields}

A text, an MTEXT or an attribute can hold a field, such as a date or a drawing property. Its `Value` is the field's last result, the text AutoCAD shows. The field itself is a FIELD object in the text's extension dictionary, under the key `ACAD_FIELD`, with the field code, such as `%<\AcVar Date \f "M/d/yyyy">%`, in its own data. dwgio does not type FIELD objects: they load as `Unknown`, kept and saved back, but their code is not read. To find the texts that hold fields, look for `ACAD_FIELD` in their extension dictionary, `Base::Dictionary`.

## Across releases {#releases}

- Up to R2004 the text is in the drawing's code page, the one `Drawing::Codepage` gives, and dwgio converts it to UTF-8. From R2007 it is UTF-16 in the file, and UTF-8 in dwgio.
- An MTEXT's background, `Background`, is from R2004. Its columns, `Columns`, are fields from R2018; before, AutoCAD keeps them in the MTEXT's `ACAD` extended data, between `ACAD_MTEXT_COLUMN_INFO_BEGIN` and `ACAD_MTEXT_COLUMN_INFO_END`. The two-line MTEXT here has dynamic columns, `Columns::Kind` 2, in its R2018 save, and the same values in extended data in this R2000 one. A save across R2018 moves them from one to the other, as [Compatibility](/concepts/compatibility/#r2013) says.
- An R14 file keeps a placeholder in `Alignment` for a Left text; dwgio gives `Position` there instead.
- `AttDef::Locked` and `Attrib::Locked` are from R2007, `Type` from R2018.
