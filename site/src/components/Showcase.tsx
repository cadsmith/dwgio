import { useEffect, useMemo, useRef, useState, type KeyboardEvent } from 'react'
import { gsap } from '../motion'

const steps = [
  {
    title: 'Load',
    text: 'LoadFile reads a file of any release from R14 to R2018: the header variables, the classes, every object and the preview.',
  },
  {
    title: 'Walk the objects',
    text: 'TypeOf names each object; BaseOf gives its handle, its owner, its reactors and its extended data.',
  },
  {
    title: 'Save',
    text: 'SaveFile writes the drawing to its own release or any other, and AutoCAD’s audit finds nothing to fix.',
  },
]

// The home example's code group, its own tab bar dropped: the steps beside it are its tabs.
const panels = (html: string) =>
  html
    .replace(/<div class="code-tabs" role="tablist">.*?<\/div>/, '')
    .replace(/id="group-\d+-panel-(\d+)" aria-labelledby="group-\d+-tab-\d+"/g, 'id="showcase-panel-$1" aria-labelledby="showcase-tab-$1"')

export function Showcase({ html }: { html: string }) {
  const code = useRef<HTMLDivElement>(null)
  const tabs = useRef<(HTMLButtonElement | null)[]>([])
  const [active, setActive] = useState(0)
  const content = useMemo(() => panels(html), [html])
  useEffect(() => {
    const blocks = code.current?.querySelectorAll<HTMLElement>('[data-block]') ?? []
    for (const one of blocks) one.hidden = one.dataset.block !== String(active)
    const shown = code.current?.querySelector(`[data-block="${active}"]`)
    if (shown && window.matchMedia('(prefers-reduced-motion: no-preference)').matches) {
      gsap.fromTo(shown, { opacity: 0, y: 6 }, { opacity: 1, y: 0, duration: 0.35, ease: 'power2.out' })
    }
  }, [active])
  const key = (event: KeyboardEvent) => {
    const moves: Record<string, number> = { ArrowDown: active + 1, ArrowRight: active + 1, ArrowUp: active - 1, ArrowLeft: active - 1, Home: 0, End: steps.length - 1 }
    const to = moves[event.key]
    if (to === undefined) return
    event.preventDefault()
    const next = (to + steps.length) % steps.length
    setActive(next)
    tabs.current[next]?.focus()
  }
  return (
    <div className="showcase-body">
      <ol className="steps" role="tablist" aria-label="Steps of the example" aria-orientation="vertical">
        {steps.map((one, index) => (
          <li key={one.title} role="presentation">
            <button
              type="button"
              role="tab"
              id={`showcase-tab-${index}`}
              aria-controls={`showcase-panel-${index}`}
              aria-selected={active === index}
              tabIndex={active === index ? 0 : -1}
              ref={(element) => {
                tabs.current[index] = element
              }}
              onClick={() => setActive(index)}
              onKeyDown={key}
            >
              <span className="step-index">{String(index + 1).padStart(2, '0')}</span>
              <span className="step-text">
                <strong>{one.title}</strong>
                <span>{one.text}</span>
              </span>
            </button>
          </li>
        ))}
      </ol>
      <div className="showcase-code" ref={code} dangerouslySetInnerHTML={{ __html: content }} />
    </div>
  )
}
