import { readFileSync, readdirSync, statSync, writeFileSync } from 'node:fs'
import { join, relative } from 'node:path'
import { paint } from './highlight.ts'

type Tag = { tag: string; name?: string; text: string }
type Documented = { doc: string; tags?: Tag[] }
type Field = Documented & { type: string; name: string; value?: string }
type Method = Documented & { signature: string; html?: string }
type Struct = Documented & {
  kind: 'struct' | 'class'
  name: string
  template?: string
  templateHtml?: string
  fields: Field[]
  methods: Method[]
}
type Value = Documented & { name: string }
type Enum = Documented & { name: string; values: Value[] }
type Alias = Documented & { name: string; target: string; html?: string }
type Constant = Documented & { type: string; name: string; value: string }
type Fn = Documented & { name: string; signature: string; html?: string }
type Header = {
  path: string
  brief?: string
  about?: Documented
  include?: string
  namespace: string
  group: string
  structs: Struct[]
  enums: Enum[]
  aliases: Alias[]
  constants: Constant[]
  functions: Fn[]
}

const root = join(import.meta.dirname, '..', '..')
const include = join(root, 'include', 'dwgio')
const source = join(root, 'source')

const walk = (dir: string): string[] =>
  readdirSync(dir).flatMap((name) => {
    const path = join(dir, name)
    return statSync(path).isDirectory() ? walk(path) : [path]
  })

const squeeze = (text: string) => text.replace(/\s+/g, ' ').trim()

const groups: [string, (path: string) => boolean][] = [
  ['Start', (p) => ['dwgio.hpp', 'load.hpp', 'save.hpp', 'view.hpp', 'create.hpp', 'edit.hpp', 'audit.hpp', 'version.hpp'].includes(p) || p.startsWith('edit/')],
  ['Drawing', (p) => ['drawing.hpp', 'header.hpp', 'class.hpp', 'preview.hpp', 'section.hpp', 'issue.hpp', 'failure.hpp'].includes(p)],
  ['Objects', (p) => ['object.hpp', 'unknown.hpp', 'base.hpp', 'common.hpp', 'corners.hpp', 'xdata.hpp', 'tag.hpp'].includes(p) || p.startsWith('objects/')],
  ['Tables', (p) => p.startsWith('tables/')],
  ['Entities', (p) => p.startsWith('entities/')],
  ['Values', () => true],
]

// A comment's text, its delimiters and leading stars taken off, one entry a line.
const commentLines = (comment: string): string[] =>
  comment
    .replace(/^\/\*\*+<?/, '')
    .replace(/\*+\/$/, '')
    .split('\n')
    .map((line) => line.trim().replace(/^\*+(?=\s|$)/, '').trim())

// A doc comment's prose, paragraphs apart by an empty line, and its tags: `@param name text`, `@return text`.
const parse = (lines: string[]): Documented => {
  const prose: string[][] = [[]]
  const tags: Tag[] = []
  for (const line of lines) {
    if (line.startsWith('@')) {
      const [tag, ...rest] = line.slice(1).split(/\s+/)
      const named = tag === 'param' || tag === 'tparam'
      tags.push({ tag, name: named ? rest.shift() : undefined, text: rest.join(' ') })
    } else if (!line) {
      if (prose.at(-1)?.length) prose.push([])
    } else if (tags.length > 0) {
      const last = tags.at(-1)
      if (last) last.text = squeeze(`${last.text} ${line}`)
    } else {
      prose.at(-1)?.push(line)
    }
  }
  const doc = prose
    .filter((one) => one.length > 0)
    .map((one) => one.join(' '))
    .join('\n\n')
  return tags.length > 0 ? { doc, tags } : { doc }
}

// The text with every comment blanked out, its length and lines kept, so that a search of the code never matches
// words of a comment.
const masked = (text: string): string =>
  text.replace(/\/\*[^]*?\*\/|\/\/[^\n]*/g, (comment) => comment.replace(/[^\n]/g, ' '))

// The doc block ending on the line before the one holding `at`; a file's banner is no declaration's.
const docBefore = (text: string, at: number): Documented => {
  const before = text.slice(0, text.lastIndexOf('\n', at - 1) + 1).trimEnd()
  if (!before.endsWith('*/')) return { doc: '' }
  const start = before.lastIndexOf('/**')
  const comment = before.slice(start)
  if (start < 0 || comment.startsWith('/**<') || comment.includes('@file')) return { doc: '' }
  return parse(commentLines(comment))
}

const closing = (text: string, open: number): number => {
  let depth = 0
  for (let at = open; at < text.length; at += 1) {
    if (text[at] === '/' && text[at + 1] === '/') {
      at = text.indexOf('\n', at)
      if (at < 0) return text.length
      continue
    }
    if (text[at] === '/' && text[at + 1] === '*') {
      at = text.indexOf('*/', at)
      if (at < 0) return text.length
      continue
    }
    if (text[at] === '{') depth += 1
    if (text[at] === '}') {
      depth -= 1
      if (depth === 0) return at
    }
  }
  return text.length
}

type Statement = Documented & { code: string }

// The statements of a body, each with the doc block before it or the trailing note after it.
const statements = (body: string): Statement[] => {
  const out: Statement[] = []
  let depth = 0
  let code = ''
  let doc: string[] = []
  let block: string[] | undefined
  const flush = () => {
    if (squeeze(code)) out.push({ code: squeeze(code), ...parse(doc) })
    code = ''
    doc = []
  }
  for (const whole of body.split('\n')) {
    const line = whole.trim()
    if (block) {
      block.push(line)
      if (line.endsWith('*/')) {
        doc = commentLines(block.join('\n'))
        block = undefined
      }
      continue
    }
    if (depth === 0 && line.startsWith('/**') && !line.startsWith('/**<')) {
      if (line.endsWith('*/')) doc = commentLines(line)
      else block = [line]
      continue
    }
    if (line.startsWith('//')) continue
    const note = /\/\*\*<([^]*?)\*\//.exec(whole)
    const raw = whole.replace(/\s*\/\*\*<[^]*?\*\//, '')
    const before = out.length
    if (depth === 0 && /^(public|private|protected):$/.test(line)) {
      code = line
      flush()
      continue
    }
    const opened = depth
    for (const char of raw) {
      if (char === '{' || char === '(') depth += 1
      if (char === '}' || char === ')') depth -= 1
      code += char
      if (char === ';' && depth === 0) flush()
    }
    code += '\n'
    if (depth === 0 && opened === 0 && raw.trim().endsWith('}')) flush()
    if (depth === 0 && opened > 0 && raw.trim() === '}') flush()
    if (note) {
      const noted = parse(commentLines(`/**<${note[1]}*/`))
      const last = out.at(-1)
      if (out.length > before && last) Object.assign(last, noted)
      else doc = commentLines(`/**<${note[1]}*/`)
    }
  }
  return out
}

const membersOf = (body: string, kind: 'struct' | 'class') => {
  const fields: Field[] = []
  const methods: Method[] = []
  let visible = kind === 'struct'
  for (const one of statements(body)) {
    if (one.code === 'public:') visible = true
    if (one.code === 'private:' || one.code === 'protected:') visible = false
    if (!visible || one.code.includes('operator') || /^(using|static|friend|template)\b/.test(one.code)) continue
    if (/^[\w:<>, ]*\(|\w\s*\(/.test(one.code.replace(/\{[^]*\}\s*;?$/, '')) && !/^[^(]*=/.test(one.code)) {
      const signature = one.code
        .replace(/\s*\{[^]*\}\s*;?$/, '')
        .replace(/;$/, '')
        .replace(/^\[\[nodiscard\]\]\s*/, '')
        .replace(/\)\s*:\s*(?!:)[^]*$/, ')')
      methods.push({ signature: squeeze(signature), doc: one.doc, tags: one.tags })
      continue
    }
    const match = /^(.+?)\s+(\w+)\s*(\{.*\}|=\s*.+)?;$/.exec(one.code)
    if (!match) continue
    fields.push({
      type: match[1],
      name: match[2],
      value: match[3] ? match[3].replace(/^=\s*/, '').replace(/^\{(.*)\}$/, '$1') : undefined,
      doc: one.doc,
      tags: one.tags,
    })
  }
  return { fields, methods }
}

const names = new Map<string, string>()
for (const path of walk(source).filter((p) => p.endsWith('.hpp'))) {
  const text = readFileSync(path, 'utf8')
  for (const match of text.matchAll(/Type<([\w:]+)>\s*=\s*"([^"]+)"/g)) {
    names.set(match[1], match[2])
  }
  for (const match of text.matchAll(/struct Shape<([\w:]+)>[\s\S]*?Name = "([^"]+)"/g)) {
    names.set(match[1], match[2])
  }
}

const headers: Header[] = walk(include)
  .filter((path) => path.endsWith('.hpp'))
  .sort()
  .map((path) => {
    const text = readFileSync(path, 'utf8')
    const code = masked(text)
    const rel = relative(include, path)
    const opened = /namespace\s+(DwgIO(?:::\w+)*)\s*\{/.exec(code)
    const namespace = opened?.[1] ?? 'DwgIO'
    const banner = /^\/\*+\n([^]*?)\*+\//.exec(text)
    const brief = banner ? parse(commentLines(`/**${banner[1]}*/`)).tags?.find((one) => one.tag === 'brief')?.text : undefined
    const about = opened ? docBefore(text, opened.index) : { doc: '' }
    const structs: Struct[] = []
    const spans: [number, number][] = []
    const pattern = /(template\s*<[^>]*>\s*)?(?<!enum )\b(struct|class)\s+(?:DWGIO_API\s+)?(\w+)\s*(?::[^{;]*)?\{/g
    for (const match of code.matchAll(pattern)) {
      const at = match.index ?? 0
      const open = at + match[0].length - 1
      const end = closing(text, open)
      spans.push([at, end])
      if (['std', 'hash', 'formatter'].includes(match[3])) continue
      const kind = match[2] as 'struct' | 'class'
      structs.push({
        kind,
        name: match[3],
        template: match[1] ? squeeze(match[1]) : undefined,
        ...docBefore(text, at),
        ...membersOf(text.slice(open + 1, end), kind),
      })
    }
    const outside = (at: number) => !spans.some(([from, to]) => at > from && at < to)
    const enums = [...code.matchAll(/enum class (\w+)\s*:\s*[\w:]+\s*\{([^}]*)\}/g)].map((one) => ({
      name: one[1],
      ...docBefore(text, one.index ?? 0),
      values: statements(
        text.slice((one.index ?? 0) + one[0].indexOf('{') + 1, (one.index ?? 0) + one[0].length - 1).replace(/,(\s*(?:\/\*\*<[^\n]*?\*\/)?\s*)$/gm, ';$1'),
      )
        .map((value) => ({ name: value.code.replace(/;$/, '').replace(/\s*=.*$/, ''), doc: value.doc, tags: value.tags }))
        .filter((value) => /^\w+$/.test(value.name)),
    }))
    const aliases = [...code.matchAll(/^[ \t]*using (\w+)\s*=\s*([^;]+);/gm)]
      .filter((one) => outside(one.index ?? 0))
      .map((one) => ({ name: one[1], target: squeeze(one[2]), ...docBefore(text, (one.index ?? 0) + one[0].indexOf('using')) }))
    const constants = [...code.matchAll(/^[ \t]*inline constexpr ([\w:<>, ]+?) (\w+)\s*(?:=\s*([^;]+)|\{([^;]*)\});/gm)]
      .filter((one) => outside(one.index ?? 0))
      .map((one) => ({
        type: squeeze(one[1]),
        name: one[2],
        value: squeeze(one[3] ?? one[4] ?? ''),
        ...docBefore(text, (one.index ?? 0) + one[0].indexOf('inline')),
      }))
    const declared = /^[ \t]*(?:\[\[nodiscard\]\]\s*)?DWGIO_API\s+(auto|void)\s+(\w+)\(([^;]*?)\)\s*(->\s*[^;{]+)?;/gm
    const templated = /^(template\s*<[^>]*>\s*(?:requires[^\n]*\n)?)\s*\[\[nodiscard\]\]\s*auto\s+(\w+)\(([^)]*)\)\s*(->\s*[^{;]+)\{/gm
    const functions = [
      ...[...code.matchAll(declared)].map((one) => ({ one, head: '' })),
      ...[...code.matchAll(templated)].map((one) => ({ one, head: squeeze(one[1]) + ' ' })),
    ]
      .filter(({ one }) => outside(one.index ?? 0))
      .sort((a, b) => (a.one.index ?? 0) - (b.one.index ?? 0))
      .map(({ one, head }) => ({
        name: one[2],
        signature: squeeze(`${head}${head ? 'auto' : one[1]} ${one[2]}(${one[3]})${one[4] ? ' ' + one[4] : ''}`),
        ...docBefore(text, (one.index ?? 0) + one[0].search(/\S/)),
      }))
    const group = groups.find(([, test]) => test(rel))?.[0] ?? 'Values'
    return { path: rel, brief, about: about.doc ? about : undefined, namespace, group, structs, enums, aliases, constants, functions }
  })

for (const one of headers) {
  one.include = paint(`#include "dwgio/${one.path}"`)
  for (const fn of one.functions) fn.html = paint(fn.signature)
  for (const alias of one.aliases) alias.html = paint(`using ${alias.name} = ${alias.target};`)
  for (const item of one.structs) {
    if (item.template) item.templateHtml = paint(item.template)
    for (const method of item.methods) method.html = paint(method.signature)
  }
}

const kinds = [...names.entries()]
  .map(([type, dxf]) => ({
    type,
    dxf,
    family: type.startsWith('Entities::') ? 'entity' : type.startsWith('Objects::') ? 'object' : 'table',
  }))
  .sort((one, other) => one.dxf.localeCompare(other.dxf))

const homes = (type: string) => {
  const name = type.split('::').pop() ?? type
  return headers.find((one) => one.structs.some((item) => item.name === name) || one.aliases.some((item) => item.name === name))?.path
}
writeFileSync(join(import.meta.dirname, '..', 'src', 'headers.json'), JSON.stringify(headers))
const version = /project\(dwgio VERSION ([\d.]+)/.exec(readFileSync(join(root, 'CMakeLists.txt'), 'utf8'))?.[1] ?? ''
writeFileSync(join(import.meta.dirname, '..', 'src', 'version.json'), JSON.stringify({ version }))
writeFileSync(
  join(import.meta.dirname, '..', 'src', 'kinds.json'),
  JSON.stringify(kinds.map((one) => ({ ...one, header: homes(one.type) ?? 'dwgio.hpp' })), null, 1),
)
const declarations = headers.flatMap((one) => [
  ...one.structs.map((item) => ({ at: `${one.path} ${item.name}`, doc: item.doc || item.tags?.length })),
  ...one.structs.flatMap((item) =>
    [...item.fields.map((field) => ({ at: `${one.path} ${item.name}::${field.name}`, doc: field.doc || field.tags?.length })),
      ...item.methods.map((method) => ({ at: `${one.path} ${item.name}::${method.signature}`, doc: method.doc || method.tags?.length }))],
  ),
  ...one.enums.flatMap((item) => [{ at: `${one.path} ${item.name}`, doc: item.doc }, ...item.values.map((value) => ({ at: `${one.path} ${item.name}::${value.name}`, doc: value.doc || value.tags?.length }))]),
  ...one.aliases.map((item) => ({ at: `${one.path} ${item.name}`, doc: item.doc || item.tags?.length })),
  ...one.constants.map((item) => ({ at: `${one.path} ${item.name}`, doc: item.doc || item.tags?.length })),
  ...one.functions.map((item) => ({ at: `${one.path} ${item.name}`, doc: item.doc || item.tags?.length })),
])
const missing = declarations.filter((one) => !one.doc)
console.log(`${headers.length} headers, ${kinds.length} kinds, ${declarations.length - missing.length} of ${declarations.length} declarations documented`)
if (process.argv.includes('--missing')) {
  for (const one of missing) console.log(`  ${one.at}`)
}
