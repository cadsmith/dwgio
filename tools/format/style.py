#!/usr/bin/env python3
"""Formats dwgio's comments: clang-format for the code, then every doc comment as a block.

    python3 -I tools/format/style.py [--check] [file ...]

CLANG_FORMAT names the clang-format to run, LLVM's own by default.

With no file named, every tracked or unignored .hpp and .cpp is formatted. With --check, nothing is
written: the files that would change, or that open with no banner, are listed and the exit status is 1.

Comments take two shapes, their text two spaces in, wrapped at the 120th column, and their tags
one table, the tag names, the parameter names and the texts each in a column of its own.

  A file's banner, the block holding @file: a frame of stars from the first column to the 120th.
  Its @file is the path the file is included by, or its path in the repository.

      /******************************************************************************************
       *  @file      dwgio/entities/line.hpp
       *  @brief     The LINE entity.
       *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
       *****************************************************************************************/

  Everything else: a JavaDoc block, which clangd shows in a hover with nothing around it.

      /**
       *  Reads to the stream's end.
       *
       *  @return The drawing; a failure for a stream that has already failed.
       */

A /** */ block and a run of whole-line /// comments become such a block. A note on a member or
an enumerator is a trailing /**< */, which a trailing ///< becomes: clang-format aligns where a
run of them opens, and the formatter closes them on one column. A // comment is left as it is,
so are // NOLINT and // clang-format directives. clang-format's ReflowComments is IndentOnly, so
the two tools never undo each other. The code's tokens are compared before and after, and a file
whose code would change is refused.
"""

import os
import pathlib
import re
import shutil
import subprocess
import sys
import textwrap

ROOT = pathlib.Path(__file__).resolve().parents[2]
WIDTH = 120
NAMED = {"@param", "@tparam"}
HEADINGS = {"@def", "@namespace", "@class", "@struct", "@union", "@enum", "@fn", "@var", "@typedef",
            "@defgroup", "@page", "@mainpage"}
CLANG_FORMAT_HINTS = ("/opt/homebrew/opt/llvm/bin/clang-format", "/usr/local/opt/llvm/bin/clang-format")


def clang_format():
    named = os.environ.get("CLANG_FORMAT")
    if named:
        return named
    for hint in CLANG_FORMAT_HINTS:
        if pathlib.Path(hint).exists():
            return hint
    found = shutil.which("clang-format")
    if not found:
        raise SystemExit("clang-format not found")
    return found


def wrap(text, width):
    return textwrap.wrap(text, width=max(width, 20), break_long_words=False, break_on_hyphens=False) or [""]


def paragraphs(body):
    """The body's lines as paragraphs: lists of lines, the empty line between them dropped."""
    out, current = [], []
    for line in body:
        if line.strip():
            current.append(line.strip())
        elif current:
            out.append(current)
            current = []
    if current:
        out.append(current)
    return out


def items(paragraph):
    """A paragraph as items: a tag or a bullet starts one, any other line runs on the one before."""
    out = []
    for line in paragraph:
        if line.startswith(("@", "\\", "- ")) or not out:
            out.append(line)
        else:
            out[-1] += " " + line
    return out


def tag_parts(item):
    words = item.split(None, 2)
    tag = words[0]
    if tag in NAMED and len(words) > 1:
        return tag, words[1], words[2] if len(words) > 2 else ""
    return tag, "", item[len(tag):].strip()


def heading(body):
    """A block's heading command, such as `@def NAME`, which must stay its first line and hold the
    name alone; the text after the name is the block's first paragraph."""
    for index, line in enumerate(body):
        if not line.strip():
            continue
        words = line.split(None, 2)
        if words[0] not in HEADINGS or len(words) < 2:
            return None, body
        rest = [words[2]] if len(words) > 2 else []
        return f"{words[0]} {words[1]}", rest + body[index + 1:]
    return None, body


def render(body, prefix, width):
    """The lines of a block's body, each beginning with the prefix, none past the width."""
    room = width - len(prefix)
    title, body = heading(body)
    prose, tags = [], []
    for paragraph in paragraphs(body):
        if paragraph[0].startswith(("@", "\\")):
            tags.extend(tag_parts(item) for item in items(paragraph))
        else:
            prose.append(items(paragraph))
    lines = []
    for paragraph in prose:
        if lines:
            lines.append("")
        for item in paragraph:
            if item.startswith("- "):
                first, *rest = wrap(item[2:], room - 2)
                lines.append("- " + first)
                lines.extend("  " + more for more in rest)
            else:
                lines.extend(wrap(item, room))
    if tags:
        if lines:
            lines.append("")
        tag_width = max(len(tag) for tag, _, _ in tags)
        name_width = max((len(name) for _, name, _ in tags), default=0)
        column = tag_width + 1 + (name_width + 1 if name_width else 0)
        for tag, name, text in tags:
            head = tag.ljust(tag_width) + " " + (name.ljust(name_width) + " " if name_width else "")
            first, *rest = wrap(text, room - column)
            lines.append((head + first).rstrip())
            lines.extend(" " * column + more for more in rest)
    if title:
        lines.insert(0, title)
    return [(prefix + line).rstrip() for line in lines]


def javadoc(indent, body):
    return [indent + "/**", *render(body, indent + " *  ", WIDTH), indent + " */"]


def banner(indent, body, filed):
    """The banner frame, its @file the one given."""
    body = [f"@file {filed}" if line.startswith("@file") else line for line in body]
    stars = WIDTH - len(indent)
    return [indent + "/" + "*" * (stars - 1), *render(body, indent + " *  ", WIDTH),
            indent + " " + "*" * (stars - 2) + "/"]


def strip_block(lines):
    """The text of a /** */ block's lines, its frame and leading stars taken off."""
    body = []
    for index, line in enumerate(lines):
        text = line.strip()
        if index == 0:
            text = re.sub(r"^/\*+", "", text)
        if index == len(lines) - 1:
            text = re.sub(r"\**/$", "", text)
        text = re.sub(r"^\*+(?=\s|$)", "", text) if index else text
        body.append(text.strip())
    return body


BLOCK_START = re.compile(r"^(\s*)/\*\*")
TRAILING_NOTE = re.compile(r"^(.*\S)\s*///<\s?(.*?)\s*$")
DOC_LINE = re.compile(r"^(\s*)///(?!<)(?!/)\s?(.*)$")


def comments(text, filed):
    lines = text.split("\n")
    out, index = [], 0
    while index < len(lines):
        line = lines[index]
        start = BLOCK_START.match(line)
        if start and not line.strip().startswith("/**<"):
            end = index
            while "*/" not in lines[end] or (end == index and line.strip().endswith("/**")):
                end += 1
            block = lines[index:end + 1]
            indent = start.group(1)
            body = strip_block(block)
            framed = any(each.startswith("@file") for each in body)
            out.extend(banner(indent, body, filed) if framed else javadoc(indent, body))
            index = end + 1
            if not indent and index < len(lines) and lines[index].strip().startswith("#"):
                out.append("")
            continue
        doc = DOC_LINE.match(line)
        if doc:
            indent = doc.group(1)
            body = []
            while index < len(lines):
                each = DOC_LINE.match(lines[index])
                if not each or each.group(1) != indent:
                    break
                body.append(each.group(2))
                index += 1
            out.extend(javadoc(indent, body))
            continue
        note = TRAILING_NOTE.match(line)
        if note:
            line = f"{note.group(1)} /**< {note.group(2)} */"
        out.append(line)
        index += 1
    return "\n".join(out)


NOTE = re.compile(r"^(.*\S)(\s+)/\*\*<\s(.*?)\s*\*/\s*$")


def notes(text):
    """Trailing /**< */ notes on consecutive lines, closed on one column: each note's text padded
    to the longest of its run, unless that would take a line past the width. clang-format aligns
    where they open, and leaves the spaces inside a comment as they are."""
    lines = text.split("\n")
    index = 0
    while index < len(lines):
        run = []
        while index + len(run) < len(lines) and NOTE.match(lines[index + len(run)]):
            run.append(NOTE.match(lines[index + len(run)]))
        width = max((len(note.group(3)) for note in run), default=0)
        padded = [f"{code}{gap}/**< {words.ljust(width)} */" for code, gap, words in (note.groups() for note in run)]
        if all(len(line) <= WIDTH for line in padded):
            lines[index:index + len(run)] = padded
        index += len(run) or 1
    return "\n".join(lines)


def tokens(text):
    """The code with every comment and all whitespace taken out, string and character literals
    kept whole: what formatting must never change."""
    out, index, size = [], 0, len(text)
    while index < size:
        char = text[index]
        if text.startswith("//", index):
            index = text.find("\n", index)
            index = size if index < 0 else index
            continue
        if text.startswith("/*", index):
            index = text.index("*/", index + 2) + 2
            out.append(" ")
            continue
        raw = re.match(r'R"([^()\\ ]{0,16})\(', text[index:index + 20])
        if raw:
            close = text.index(")" + raw.group(1) + '"', index) + len(raw.group(1)) + 2
            out.append(text[index:close])
            index = close
            continue
        if char == '"' or (char == "'" and not (index and (text[index - 1].isalnum() or text[index - 1] == "_"))):
            end = index + 1
            while text[end] != char:
                end += 2 if text[end] == "\\" else 1
            out.append(text[index:end + 1])
            index = end + 1
            continue
        if char.isspace():
            out.append(" ")
        else:
            out.append(char)
        index += 1
    return re.sub(r"\s+", " ", "".join(out)).strip()


def sources(named):
    if named:
        return [pathlib.Path(name).resolve() for name in named]
    listed = subprocess.run(["git", "ls-files", "--cached", "--others", "--exclude-standard", "*.hpp", "*.cpp"],
                            cwd=ROOT, check=True, capture_output=True, text=True).stdout.split()
    return [ROOT / name for name in listed]


def filed(path):
    """The path a banner names a file by: as included for a public header, else from the root."""
    relative = path.relative_to(ROOT).as_posix()
    return relative.removeprefix("include/")


def formatted(path, tool):
    def clang(text):
        return subprocess.run([tool, f"--assume-filename={path}"], input=text, check=True, capture_output=True,
                              text=True, cwd=ROOT).stdout

    return notes(clang(comments(clang(path.read_text()), filed(path))))


def bannered(text):
    return text.startswith("/" + "*" * (WIDTH - 1) + "\n") and "@file" in text.split("*/", 1)[0]


def main():
    arguments = sys.argv[1:]
    check = "--check" in arguments
    named = [argument for argument in arguments if argument != "--check"]
    tool = clang_format()
    changed, bare = [], []
    for path in sources(named):
        before = path.read_text()
        after = formatted(path, tool)
        shown = path.relative_to(ROOT)
        if tokens(after) != tokens(before):
            raise SystemExit(f"{shown}: formatting would change the code; nothing written")
        if not bannered(after):
            bare.append(shown)
        if after != before:
            changed.append(shown)
            if not check:
                path.write_text(after)
    for path in changed:
        print(("would change: " if check else "formatted: ") + str(path))
    for path in bare:
        print(f"no banner: {path}")
    sys.exit(1 if check and (changed or bare) else 0)


if __name__ == "__main__":
    main()
