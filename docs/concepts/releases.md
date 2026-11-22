# Releases

AutoCAD changed the DWG format with most of its releases. The first six bytes of a file name the release: `ReleaseOf` turns them into a `Version`, `AcadverOf` turns a `Version` back into them, and `NameOf` gives the short name.

| Version | Code | Reads | Writes | What changed |
| :--- | :--- | :--- | :--- | :--- |
| `R14` | AC1014 | yes | yes | The locator file: a header of records, bit-packed variables, objects, a map, a second header |
| `R2000` | AC1015 | yes | yes | A bit size before each object's handles, entity colors and plot styles as flags, lineweights |
| `R2004` | AC1018 | yes | yes | Sections paged and compressed; owned objects listed, true colors, backgrounds, gradients and shade plots |
| `R2007` | AC1021 | yes | yes | Pages coded against damage and compressed another way; text in UTF-16, in a stream of its own inside each object |
| `R2010` | AC1024 | yes | yes | Back to R2004's paged container, keeping R2007's text stream; object types as a bit pair and a byte |
| `R2013` | AC1027 | yes | yes | A data store section, and a bit on each object for it |
| `R2018` | AC1032 | yes | yes | A few entities' fields and three shorts in the auxiliary header |

## Across releases {#across}

A drawing keeps what its file holds, in the file's terms, and saves back to its own release: byte for byte in R14 and R2000, and section for section, once decompressed, from R2004. Saved to another release, `Save` translates what the two store differently as it writes: an R14 drawing saved as R2000 holds every typed entity as AutoCAD's own R2000 save does, except for the plot styles, and the same is true the other way. AutoCAD's audit finds nothing in a drawing of any release saved as any other.

R2007 holds its objects as R2010 does except for the type code, so a save to or from it translates what R2004 to R2018 translate among themselves, and its coded container is made again.

[Compatibility](/concepts/compatibility/) lists, step by step between releases, what a save converts, what it drops with an issue and what it drops without one.
