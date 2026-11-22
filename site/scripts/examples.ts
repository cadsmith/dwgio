// Builds and runs the C++ of every page in docs/guide, docs/tutorials and docs/entities, and the
// home page's example, then checks what each wrote.
//
//   npm run examples                 all of them
//   npm run examples -- styles text  some of them
//   npm run examples -- --acad       and AutoCAD's own AUDIT on every file they wrote
//
// A page's code is the body of a main, or a whole program when it has its own. It passes
// when it compiles with warnings as errors, runs without failing, prints exactly the text
// of its "Output" blocks, and every DWG it writes loads back with no issue (and passes
// AutoCAD's audit, with --acad).
import { execFileSync, spawnSync } from 'node:child_process'
import { copyFileSync, existsSync, mkdtempSync, readFileSync, readdirSync, writeFileSync } from 'node:fs'
import { tmpdir } from 'node:os'
import { join } from 'node:path'

const site = join(import.meta.dirname, '..')
const root = join(site, '..')
const library = join(root, 'build', 'release', 'lib', 'libdwgio.a')
const compiler = process.env.CXX ?? 'c++'
const samples = [
  'foundation-r2000.dwg',
  'foundation-r14.dwg',
  'foundation-r2004.dwg',
  'curves-r2000.dwg',
  'curves-r14.dwg',
  'curves-r2004.dwg',
  'curves-r2018.dwg',
  'entities-r2000.dwg',
  'entities-r14.dwg',
  'kinds-r2000.dwg',
  'dimensions-r2000.dwg',
  'mleaders-r2018.dwg',
  'rasters-r2000.dwg',
  'solids-r2000.dwg',
  'solids-r2018.dwg',
  'tables-r2004.dwg',
  'cells-r2004.dwg',
  'cells-r2018.dwg',
].map((name) => join(root, 'tests', 'fixtures', 'acad', name))
const flags = [
  '-std=c++23',
  '-Wall',
  '-Wextra',
  '-Werror',
  `-I${join(root, 'include')}`,
  `-I${join(root, 'build', 'release', 'include')}`,
]

if (!existsSync(library)) {
  console.error(`${library} is missing: cmake --preset release && cmake --build --preset release --target dwgio`)
  process.exit(1)
}

const args = process.argv.slice(2)
const acad = args.includes('--acad')
const wanted = args.filter((one) => !one.startsWith('--'))
const content = join(root, 'docs')
const pages = ['guide', 'tutorials', 'entities']
  .flatMap((folder) =>
    readdirSync(join(content, folder))
      .filter((name) => name.endsWith('.md'))
      .map((name) => `${folder}/${name.replace(/\.md$/, '')}`),
  )
  .concat('home')
  .filter((page) => wanted.length === 0 || wanted.some((one) => page.endsWith(one)))

// The home page's install tab repeats the example's first line; only the example runs.
const source = (page: string) => {
  const text = readFileSync(join(content, `${page}.md`), 'utf8')
  return page === 'home' ? text.slice(text.indexOf('{#example}')) : text
}

const work = mkdtempSync(join(tmpdir(), 'dwgio-examples-'))

const checker = `#include "dwgio/dwgio.hpp"
#include <print>
auto main(int count, char **paths) -> int
{
  int failed = 0;
  for (int at = 1; at < count; ++at)
  {
    const auto loaded = DwgIO::LoadFile(paths[at]);
    if (!loaded)
    {
      std::println("{}: does not load: byte {}: {}", paths[at], loaded.error().Offset.value_or(0), loaded.error().What);
      ++failed;
      continue;
    }
    for (const DwgIO::Issue &issue : loaded->Issues)
    {
      std::println("{}: issue at byte {}: {}", paths[at], issue.Offset.value_or(0), issue.What);
      ++failed;
    }
  }
  return failed == 0 ? 0 : 1;
}
`
writeFileSync(join(work, 'check.cpp'), checker)
execFileSync(compiler, [...flags, join(work, 'check.cpp'), library, '-o', join(work, 'check')], { stdio: 'inherit' })

const program = (body: string) =>
  [
    '#include "dwgio/dwgio.hpp"',
    '',
    '#include <algorithm>',
    '#include <array>',
    '#include <cctype>',
    '#include <charconv>',
    '#include <cmath>',
    '#include <cstdint>',
    '#include <format>',
    '#include <map>',
    '#include <optional>',
    '#include <print>',
    '#include <string>',
    '#include <string_view>',
    '#include <utility>',
    '#include <variant>',
    '#include <vector>',
    '',
    'auto main() -> int',
    '{',
    body.replaceAll('#include "dwgio/dwgio.hpp"\n', ''),
    'return 0;',
    '}',
  ].join('\n')

let failures = 0
for (const slug of pages) {
  const text = source(slug)
  const code = [...text.matchAll(/```cpp[^\n]*\n([^]*?)```/g)].map((one) => one[1]).join('\n')
  const expected = [...text.matchAll(/```text \[Output\]\n([^]*?)```/g)].map((one) => one[1]).join('')
  const place = join(work, slug)
  execFileSync('mkdir', ['-p', place])
  for (const sample of samples) copyFileSync(sample, join(place, sample.slice(sample.lastIndexOf('/') + 1)))
  copyFileSync(samples[0], join(place, 'plan.dwg'))
  const built = join(place, 'example')
  writeFileSync(`${built}.cpp`, code.includes('auto main(') ? code : program(code))
  const build = spawnSync(compiler, [...flags, `${built}.cpp`, library, '-o', built], { encoding: 'utf8' })
  if (build.status !== 0) {
    console.log(`✗ ${slug}: does not compile\n${build.stderr}`)
    failures += 1
    continue
  }
  const ran = spawnSync(built, { cwd: place, encoding: 'utf8', timeout: 30000 })
  if (ran.status !== 0) {
    console.log(`✗ ${slug}: exits with ${ran.status}\n${ran.stdout}${ran.stderr}`)
    failures += 1
    continue
  }
  if (expected && ran.stdout !== expected) {
    console.log(`✗ ${slug}: prints something other than its Output block\n--- the page\n${expected}--- the program\n${ran.stdout}`)
    failures += 1
    continue
  }
  const copied = new Set([...samples.map((sample) => sample.slice(sample.lastIndexOf('/') + 1)), 'plan.dwg'])
  const written = readdirSync(place).filter((name) => name.endsWith('.dwg') && !copied.has(name))
  const checked = spawnSync(join(work, 'check'), written, { cwd: place, encoding: 'utf8' })
  if (checked.status !== 0) {
    console.log(`✗ ${slug}: a file it wrote fails\n${checked.stdout}`)
    failures += 1
    continue
  }
  let autocad = ''
  if (acad) {
    const refused = written.filter((name) => {
      const audit = spawnSync('python3', [join(root, 'tools', 'acad', 'verify.py'), join(place, name)], { encoding: 'utf8', timeout: 120000 })
      if (audit.status !== 0) console.log(`✗ ${slug}: AutoCAD's audit fails ${name}\n${audit.stdout}${audit.stderr}`)
      return audit.status !== 0
    })
    if (refused.length) {
      failures += 1
      continue
    }
    autocad = ', AutoCAD audit clean'
  }
  console.log(`✓ ${slug}: ${written.length ? written.join(', ') : 'no file'} written, read back clean${autocad}`)
}
process.exitCode = failures === 0 ? 0 : 1
