import data from './headers.json'
import { kinds } from './kinds'

export type Tag = { tag: string; name?: string; text: string }
export type Documented = { doc: string; tags?: Tag[] }
export type Field = Documented & { type: string; name: string; value?: string }
export type Method = Documented & { signature: string; html?: string }
export type Struct = Documented & {
  kind: 'struct' | 'class'
  name: string
  template?: string
  templateHtml?: string
  fields: Field[]
  methods: Method[]
}
export type Value = Documented & { name: string }
export type Enum = Documented & { name: string; values: Value[] }
export type Alias = Documented & { name: string; target: string; html?: string }
export type Constant = Documented & { type: string; name: string; value: string }
export type Fn = Documented & { name: string; signature: string; html?: string }
export type Header = {
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

export const headers = data as Header[]
export { kinds }

export const order = ['Start', 'Edit', 'Document', 'Entities', 'Objects', 'Tables', 'Blocks', 'Audit', 'Values']

export const names = (one: Header): string[] => [
  ...one.structs.map((item) => item.name),
  ...one.enums.map((item) => item.name),
  ...one.aliases.map((item) => item.name),
  ...one.functions.map((item) => item.name),
  ...one.constants.map((item) => item.name),
]
