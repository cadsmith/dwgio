import { link } from './route'

// Each group is a heading of the sidebar; a section may span several.
export const sections = [
  { id: 'guide', title: 'Guide', slugs: ['start', 'install', 'objects', 'errors'] },
  { id: 'tutorials', title: 'Tutorials', slugs: ['reading', 'spaces', 'blocks', 'coordinates', 'styles', 'creating', 'editing', 'auditing', 'saving'] },
  { id: 'entities', title: 'Entities', slugs: ['curves', 'polylines', 'text', 'hatches', 'dimensions', 'others'] },
  { id: 'concepts', title: 'Concepts', slugs: ['drawing', 'releases', 'compatibility', 'quality', 'limits'] },
]

export const titleOf = (section: string) =>
  section === 'reference' ? 'Reference' : (sections.find((one) => one.id === section)?.title ?? section)

// A section's link is its first page's, so each page has one address.
export const start = (section: string) => link(section, sections.find((one) => one.id === section)?.slugs[0] ?? '')

// The sections the bar, the footer and the phone menu link to.
export const nav = [
  ...sections.map((one) => ({ id: one.id, label: one.title, href: start(one.id) })),
  { id: 'reference', label: 'Reference', href: link('reference') },
]

export const repository = 'https://github.com/cadsmith/dwgio'
