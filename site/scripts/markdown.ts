import { fromHighlighter } from '@shikijs/markdown-it/core'
import MarkdownIt from 'markdown-it'
import alerts from 'markdown-it-github-alerts'
import type { Plugin } from 'vite'
import { figure, highlighter, labelOf, themes } from './highlight.ts'

type Part = { id: string; title: string; html: string }

const md = MarkdownIt({ html: true, typographer: true })
md.use(alerts)
md.use(fromHighlighter(highlighter, { themes, defaultColor: false, transformers: [figure] }))
md.renderer.rules.table_open = () => '<div class="table-scroll"><table>\n'
md.renderer.rules.table_close = () => '</table></div>\n'

// A page links another as /tutorials/styles/, below wherever the site is served.
let base = '/'
md.renderer.rules.link_open = (tokens, index, options, _env, self) => {
  const href = tokens[index].attrGet('href')
  if (typeof href === 'string' && href.startsWith('/') && !href.startsWith('//')) tokens[index].attrSet('href', base + href.slice(1))
  return self.renderToken(tokens, index, options)
}

const slug = (title: string) =>
  title
    .toLowerCase()
    .replace(/[^a-z0-9]+/g, '-')
    .replace(/^-|-$/g, '')

const escape = (text: string) => text.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;')

let groups = 0

// Tabs over the fences of a code group. Only the selected tab is in the tab order; the app moves the
// selection on a click and with the arrow, Home and End keys.
const group = (body: string): string => {
  const fences = [...body.matchAll(/```(\w+)([^\n]*)\n([^]*?)```/g)]
  const id = `group-${++groups}`
  const tabs = fences
    .map(
      ([, lang, meta], index) =>
        `<button type="button" role="tab" id="${id}-tab-${index}" aria-controls="${id}-panel-${index}" data-tab="${index}" aria-selected="${index === 0}" tabindex="${index === 0 ? 0 : -1}">${escape(labelOf(lang, meta))}</button>`,
    )
    .join('')
  const blocks = fences
    .map(
      ([whole], index) =>
        `<div class="code-block" role="tabpanel" id="${id}-panel-${index}" aria-labelledby="${id}-tab-${index}" data-block="${index}"${index === 0 ? '' : ' hidden'}>${md.render(whole)}</div>`,
    )
    .join('')
  return `<div class="code-group" id="${id}"><div class="code-tabs" role="tablist">${tabs}</div>${blocks}</div>`
}

const render = (source: string): string => {
  const held: string[] = []
  const marked = source.replace(/^::: code-group\n([^]*?)\n:::\s*$/gm, (_, body: string) => {
    held.push(group(body))
    return `<!--group ${held.length - 1}-->`
  })
  return md.render(marked).replace(/<!--group (\d+)-->/g, (_, index: string) => held[Number(index)])
}

// A part's prose as plain words for the search: no code, markup or link targets.
const words = (markdown: string) =>
  markdown
    .replace(/```[^]*?```/g, ' ')
    .replace(/^:::.*$/gm, ' ')
    .replace(/\[([^\]]*)\]\([^)]*\)/g, '$1')
    .replace(/[`*_|>#{}]/g, ' ')
    .replace(/\s+/g, ' ')
    .trim()

const sectioned = (source: string) => {
  const parts: { id: string; title: string; text: string }[] = []
  for (const block of source.split(/^## /m).slice(1)) {
    const [heading, ...body] = block.split('\n')
    const named = /^(.+?)(?:\s+\{#([\w-]+)\})?\s*$/.exec(heading)
    if (named) parts.push({ id: named[2] ?? slug(named[1]), title: named[1], text: words(body.join('\n')) })
  }
  return parts
}

export const page = (source: string) => {
  const lines = source.split('\n')
  const title = lines.find((line) => line.startsWith('# '))?.slice(2).trim() ?? ''
  const parts: Part[] = []
  const lead: string[] = []
  let current: { id: string; title: string; lines: string[] } | undefined
  const close = () => {
    if (current) parts.push({ id: current.id, title: current.title, html: render(current.lines.join('\n')) })
  }
  for (const line of lines) {
    if (line.startsWith('# ')) continue
    const heading = /^## (.+?)(?:\s+\{#([\w-]+)\})?\s*$/.exec(line)
    if (heading) {
      close()
      current = { id: heading[2] ?? slug(heading[1]), title: heading[1], lines: [] }
      continue
    }
    if (current) current.lines.push(line)
    else lead.push(line)
  }
  close()
  return { title, lead: render(lead.join('\n')), parts }
}

export const markdown = (): Plugin => ({
  name: 'dwgio-markdown',
  configResolved(config) {
    base = config.base
  },
  // A page imported with ?outline keeps its sections' names and drops their text; with ?search, each
  // section's words, without code or markup.
  transform(source, id) {
    const [file, query] = id.split('?')
    if (!file.endsWith('.md')) return null
    if (query === 'search') return { code: `export default ${JSON.stringify(sectioned(source))}`, map: null }
    const whole = page(source)
    const kept = query === 'outline' ? { ...whole, parts: whole.parts.map(({ id: part, title }) => ({ id: part, title })) } : whole
    return { code: `export default ${JSON.stringify(kept)}`, map: null }
  },
})
