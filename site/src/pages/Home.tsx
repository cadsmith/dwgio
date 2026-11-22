import type { CSSProperties } from 'react'
import { BracesIcon, FileCheckIcon, GaugeIcon, ShieldCheckIcon } from 'lucide-react'
import home from '../../../docs/home.md'
import { cn } from 'cn'
import { buttonVariants } from '@/components/ui/button'
import { Stage } from '../components/Hero'
import { Showcase } from '../components/Showcase'
import { Timeline } from '../components/Timeline'
import { home as head } from '../head'
import { link, useTitle } from '../route'
import { start } from '../sections'

const part = (id: string) => home.parts.find((one) => one.id === id)?.html ?? ''

const features = [
  {
    icon: FileCheckIcon,
    title: 'Every object kept',
    text: 'Every object comes back with its handle, its owner and its data. Saved to the release it came from, the drawing comes back unchanged.',
  },
  {
    icon: ShieldCheckIcon,
    title: 'Checked against AutoCAD',
    text: 'What dwgio reads matches the DXF AutoCAD writes of the same drawing, and what it writes passes AutoCAD’s audit.',
  },
  {
    icon: GaugeIcon,
    title: 'Fast and small',
    text: 'A drawing of 100,000 entities loads in 0.04 s and saves to any release in under 0.4 s.',
  },
  {
    icon: BracesIcon,
    title: 'Plain C++23',
    text: 'Plain structs named after the DXF kinds, one std::variant over all of them, and std::expected for every failure.',
  },
]

// The title's lines, each word in a mask it slides up through as the page first paints.
const title = [
  ['Read', 'and', 'write'],
  ['DWG', 'in', 'C++'],
]

const order = (at: number): CSSProperties => ({ '--at': at }) as CSSProperties

function Corners() {
  return (
    <>
      <span className="tick tick-tl" aria-hidden="true" />
      <span className="tick tick-tr" aria-hidden="true" />
      <span className="tick tick-bl" aria-hidden="true" />
      <span className="tick tick-br" aria-hidden="true" />
    </>
  )
}

export default function Home() {
  useTitle(head.title)
  return (
    <main className="home">
      <section className="frame hero">
        <Corners />
        <div className="hero-text">
          <p className="hero-eyebrow">C++23 · R14 to R2018 read and written</p>
          <h1 className="hero-title">
            {title.map((words, line) => (
              <span key={line} className={line > 0 ? 'hero-line hero-accent' : 'hero-line'}>
                {words.map((word, index) => (
                  <span key={word}>
                    {index > 0 && ' '}
                    <span className="hero-mask">
                      <span className="hero-word" style={order(title.slice(0, line).flat().length + index)}>
                        {word}
                      </span>
                    </span>
                  </span>
                ))}
              </span>
            ))}
          </h1>
          <p className="hero-lead">
            dwgio reads an AutoCAD drawing into plain C++ values, every object kept, and writes it back unchanged or to another release.
          </p>
          <div className="hero-actions">
            <span className="shine">
              <a className={cn(buttonVariants({ size: 'lg', className: 'h-11 rounded-full px-6 text-[15px]' }))} href={start('guide')}>
                Get started
              </a>
            </span>
            <a
              className={cn(buttonVariants({ size: 'lg', variant: 'outline', className: 'h-11 rounded-full px-6 text-[15px]' }))}
              href={link('reference')}
            >
              Reference
            </a>
          </div>
          <div className="hero-install" dangerouslySetInnerHTML={{ __html: part('install') }} />
        </div>
        <Stage />
      </section>

      <section className="frame features" aria-label="Features">
        {features.map((one, index) => (
          <article key={one.title} className="feature" style={order(index)}>
            <span className="feature-icon" aria-hidden="true">
              <one.icon />
            </span>
            <h2>{one.title}</h2>
            <p>{one.text}</p>
          </article>
        ))}
      </section>

      <section className="showcase" aria-labelledby="showcase-title">
        <header className="section-head">
          <p className="eyebrow">Example</p>
          <h2 id="showcase-title">Load, read and save a drawing</h2>
          <p>LoadFile reads a drawing, its objects are plain values, and SaveFile writes a file AutoCAD opens.</p>
        </header>
        <Showcase html={part('example')} />
      </section>

      <section className="releases" aria-labelledby="releases-title">
        <header className="section-head">
          <p className="eyebrow">Compatibility</p>
          <h2 id="releases-title">Seven releases, every one read and written</h2>
          <p>
            Every release AutoCAD has written since 1997 names itself in its first six bytes; dwgio reads and writes
            each of them, and saves any as another. <a href={link('concepts', 'releases')}>What each release changed</a>
          </p>
        </header>
        <Timeline />
      </section>
    </main>
  )
}
