# Security policy

dwgio parses DWG files, which often come from people and programs you do not control. A crafted or damaged file must
make a load fail or report issues; it must never crash the program, read or write out of bounds, hang, or make it
allocate memory out of proportion to the file.

## Supported versions

| Version | Supported |
| :--- | :--- |
| 0.1.x | Yes |

## What dwgio does to stay safe

Every size, count and page number read from a file is checked before anything is allocated for it. The reader and
the writer run under eight libFuzzer targets with AddressSanitizer and UndefinedBehaviorSanitizer, and every input a
target has found is kept with a test ([tests/fuzz/README.md](tests/fuzz/README.md)). The tests also run under the
sanitizers.

## Reporting a vulnerability

Report a vulnerability privately through GitHub's
[private vulnerability reporting](https://github.com/cadsmith/dwgio/security/advisories/new) for this repository; do
not open a public issue. Include the release of dwgio, the platform and compiler, and the file that triggers the
problem, or a description of how to make one.

These count as vulnerabilities: a crash, an out-of-bounds read or write, undefined behaviour, a hang or unbounded
memory growth caused by the content of a file passed to `Load` or `LoadFile`, or by a drawing so loaded and then
passed to `Save`, `Edit`, `View` or `Audit`.

The maintainers aim to acknowledge a report within a week, and to publish a fix and an advisory once a fix is
released.
