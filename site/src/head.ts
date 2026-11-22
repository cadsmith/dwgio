import type { Header } from './api'
import type { Entry } from './docs'
import { link, reference } from './route'
import { start, titleOf } from './sections'

type Crumb = { name: string; path: string }

// What a page tells a search engine: its title, a summary, its one address and where it sits.
export type Head = { title: string; description: string; path: string; trail: Crumb[] }

const plain = (html: string) =>
  html
    .replace(/<[^>]+>/g, '')
    .replace(/\s+/g, ' ')
    .replace(/&lt;/g, '<')
    .replace(/&gt;/g, '>')
    .replace(/&quot;/g, '"')
    .replace(/&amp;/g, '&')
    .trim()

export const home: Head = {
  title: 'dwgio: read and write DWG in C++',
  description:
    'dwgio is a C++23 library that reads and writes AutoCAD DWG files from R14 to R2018, every object kept.',
  path: link(),
  trail: [],
}

export const missing: Head = { title: 'Page not found · dwgio', description: '', path: '', trail: [] }

export const docHead = ({ section, slug, page }: Entry): Head => {
  const own = { name: page.title, path: link(section, slug) }
  return {
    title: `${page.title} · dwgio`,
    description: plain(page.lead),
    path: own.path,
    trail: start(section) === own.path ? [own] : [{ name: titleOf(section), path: start(section) }, own],
  }
}

export const referenceHead = (one: Header): Head => {
  const top = { name: 'Reference', path: link('reference') }
  if (one.path === 'dwgio.hpp') {
    return {
      title: 'Reference · dwgio',
      description: 'Every type and function of dwgio, from its headers: the drawing, its objects, and the items of the header variables.',
      path: top.path,
      trail: [top],
    }
  }
  const declared = [...one.structs, ...one.enums, ...one.aliases, ...one.functions, ...one.constants]
  const summary = one.brief ?? declared.find((item) => item.doc)?.doc.split('\n\n')[0]
  return {
    title: `${one.path} · dwgio reference`,
    description: summary ?? `What dwgio/${one.path} declares: ${[...new Set(declared.map((item) => item.name))].join(', ')}.`,
    path: reference(one.path),
    trail: [top, { name: one.path, path: reference(one.path) }],
  }
}
