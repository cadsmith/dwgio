import { lazy, memo, Suspense, useEffect, useState, useSyncExternalStore } from 'react'
import { MenuIcon, MoonIcon, SearchIcon, SunIcon } from 'lucide-react'
import { Button, buttonVariants } from '@/components/ui/button'
import { Sheet, SheetContent, SheetHeader, SheetTitle, SheetTrigger } from '@/components/ui/sheet'
import { Kbd, KbdGroup } from '@/components/ui/kbd'
import { GitHubIcon } from './components/GitHub'
import { Mark } from './components/Mark'
import { Missing } from './components/Missing'
import { home } from './head'
import { link, useRoute } from './route'
import { nav, repository } from './sections'
import { version } from './version.json'

// Whether the build loads Vercel's anonymous page statistics, which the footer then says.
export const statistics = import.meta.env.VITE_VERCEL === '1'

const Home = lazy(() => import('./pages/Home'))
const Docs = lazy(() => import('./pages/Docs'))
const Reference = lazy(() => import('./pages/Reference'))
const Search = lazy(() => import('./components/Search'))


type Theme = 'dark' | 'light'

// The server cannot tell, so a prerendered page first says Ctrl.
const still = () => () => undefined
const useMac = () => useSyncExternalStore(still, () => /Mac|iPhone|iPad/.test(navigator.userAgent), () => false)

const useScrolled = () => {
  const [scrolled, setScrolled] = useState(false)
  useEffect(() => {
    const update = () => setScrolled(window.scrollY > 4)
    update()
    window.addEventListener('scroll', update, { passive: true })
    return () => window.removeEventListener('scroll', update)
  }, [])
  return scrolled
}

const stored = (): Theme | null => {
  try {
    const value = window.localStorage.getItem('dwgio-theme')
    return value === 'dark' || value === 'light' ? value : null
  } catch {
    return null
  }
}

// The theme is the root's dark class, which index.html sets before the page is drawn.
const watch = (changed: () => void) => {
  const observer = new MutationObserver(changed)
  observer.observe(document.documentElement, { attributeFilter: ['class'] })
  return () => observer.disconnect()
}

const shown = (): Theme => (document.documentElement.classList.contains('dark') ? 'dark' : 'light')

const useTheme = () => {
  const theme = useSyncExternalStore<Theme>(watch, shown, () => 'light')
  useEffect(() => {
    const media = window.matchMedia('(prefers-color-scheme: dark)')
    const changed = () => {
      if (!stored()) document.documentElement.classList.toggle('dark', media.matches)
    }
    media.addEventListener('change', changed)
    return () => media.removeEventListener('change', changed)
  }, [])
  const flip = () => {
    const next = theme === 'dark' ? 'light' : 'dark'
    document.documentElement.classList.toggle('dark', next === 'dark')
    try {
      window.localStorage.setItem('dwgio-theme', next)
    } catch {
      return
    }
  }
  return { theme, flip }
}

const copy = async (button: HTMLElement) => {
  const code = button.closest('figure')?.querySelector('pre')?.textContent ?? ''
  try {
    await navigator.clipboard.writeText(code)
    button.textContent = 'Copied'
  } catch {
    button.textContent = 'Copy failed'
  }
  window.setTimeout(() => {
    button.textContent = 'Copy'
  }, 1600)
}

const pick = (tab: HTMLElement) => {
  const group = tab.closest('.code-group')
  const index = tab.dataset.tab
  if (!group || index === undefined) return
  for (const one of group.querySelectorAll<HTMLElement>('[data-tab]')) {
    const chosen = one.dataset.tab === index
    one.setAttribute('aria-selected', String(chosen))
    one.tabIndex = chosen ? 0 : -1
  }
  for (const one of group.querySelectorAll<HTMLElement>('[data-block]')) {
    one.hidden = one.dataset.block !== index
  }
}

// The tab an arrow, Home or End key moves to from a tab of a code group.
const step = (tab: HTMLElement, key: string) => {
  const tabs = [...(tab.closest('[role="tablist"]')?.querySelectorAll<HTMLElement>('[role="tab"]') ?? [])]
  const at = tabs.indexOf(tab)
  const moves: Record<string, number> = { ArrowRight: at + 1, ArrowLeft: at - 1, Home: 0, End: tabs.length - 1 }
  const to = moves[key]
  return to === undefined ? undefined : tabs[(to + tabs.length) % tabs.length]
}

const useCode = () => {
  useEffect(() => {
    const click = (event: MouseEvent) => {
      const target = event.target as Element | null
      const button = target?.closest<HTMLElement>('button[data-copy]')
      if (button) void copy(button)
      const tab = target?.closest<HTMLElement>('button[data-tab]')
      if (tab) pick(tab)
    }
    const key = (event: KeyboardEvent) => {
      const tab = (event.target as Element | null)?.closest<HTMLElement>('button[data-tab]')
      const to = tab ? step(tab, event.key) : undefined
      if (!to) return
      event.preventDefault()
      pick(to)
      to.focus()
    }
    document.addEventListener('click', click)
    document.addEventListener('keydown', key)
    return () => {
      document.removeEventListener('click', click)
      document.removeEventListener('keydown', key)
    }
  }, [])
}

const useSearch = () => {
  const [open, setOpen] = useState(false)
  const [loaded, setLoaded] = useState(false)
  const show = (now: boolean) => {
    setOpen(now)
    if (now) setLoaded(true)
  }
  useEffect(() => {
    const key = (event: KeyboardEvent) => {
      if (event.key.toLowerCase() === 'k' && (event.metaKey || event.ctrlKey)) {
        event.preventDefault()
        setOpen((now) => !now)
        setLoaded(true)
      }
    }
    document.addEventListener('keydown', key)
    return () => document.removeEventListener('keydown', key)
  }, [])
  return { open, loaded, show }
}

export function App({ at }: { at: string[] }) {
  const { route, pending } = useRoute(at)
  const page = route[0] ?? ''
  const docs = page === 'guide' || page === 'tutorials' || page === 'entities' || page === 'concepts'
  const known = docs || page === 'reference'
  const mac = useMac()
  const { theme, flip } = useTheme()
  const search = useSearch()
  const scrolled = useScrolled()
  const [menu, setMenu] = useState(false)
  useCode()
  const next = theme === 'dark' ? 'light' : 'dark'
  return (
    <>
      <a className="skip" href="#main">
        Skip to content
      </a>
      <header className={['bar', known ? 'bar-docs' : 'bar-home', scrolled ? 'bar-scrolled' : ''].join(' ')}>
        <span className="progress" data-pending={pending || undefined} aria-hidden="true" />
        <div className="bar-inner">
          <div className="bar-start">
            <a className="brand" href={link()} aria-label="dwgio, home">
              <Mark size={26} />
              <span>dwgio</span>
            </a>
            <span className="bar-version" title="The library's version">
              v{version}
            </span>
          </div>
          <nav className="bar-nav" aria-label="Sections">
            {nav.map((one) => (
              <a key={one.id} href={one.href} aria-current={page === one.id ? 'page' : undefined}>
                {one.label}
              </a>
            ))}
          </nav>
          <div className="bar-end">
            <Button variant="secondary" className="search-trigger" onClick={() => search.show(true)} aria-label="Search docs">
              <SearchIcon data-icon="inline-start" />
              <span className="search-word">Search docs</span>
              <KbdGroup className="search-keys hidden sm:inline-flex">
                <Kbd>{mac ? '⌘' : 'Ctrl'}</Kbd>
                <Kbd>K</Kbd>
              </KbdGroup>
            </Button>
            <a className={buttonVariants({ variant: 'ghost', size: 'icon' })} href={repository} aria-label="dwgio on GitHub">
              <GitHubIcon />
            </a>
            <Button variant="ghost" size="icon" className="theme-toggle" onClick={flip} aria-label={`Use the ${next} theme`}>
              <SunIcon className="theme-sun" />
              <MoonIcon className="theme-moon" />
            </Button>
            {!known && (
              <Sheet open={menu} onOpenChange={setMenu}>
                <SheetTrigger render={<Button variant="ghost" size="icon" className="sm:hidden" aria-label="Open menu" />}>
                  <MenuIcon />
                </SheetTrigger>
                <SheetContent side="right" className="bar-sheet">
                  <SheetHeader>
                    <SheetTitle>Menu</SheetTitle>
                  </SheetHeader>
                  <nav className="bar-sheet-nav" aria-label="Sections">
                    {nav.map((one) => (
                      <a key={one.id} href={one.href} aria-current={page === one.id ? 'page' : undefined} onClick={() => setMenu(false)}>
                        {one.label}
                      </a>
                    ))}
                  </nav>
                </SheetContent>
              </Sheet>
            )}
          </div>
        </div>
      </header>
      {search.loaded && (
        <Suspense>
          <Search open={search.open} onOpenChange={search.show} />
        </Suspense>
      )}
      <Pages route={route} />
    </>
  )
}

// The page under the bar. It redraws only when the address changes: a redraw for the theme or the
// search, while a prerendered page still loads its code, would make React throw that page away.
const Pages = memo(function Pages({ route }: { route: string[] }) {
  const page = route[0] ?? ''
  const docs = page === 'guide' || page === 'tutorials' || page === 'entities' || page === 'concepts'
  const known = docs || page === 'reference'
  return (
    <>
      <div id="main">
        <Suspense fallback={<p className="loading">Loading…</p>}>
          {docs && <Docs route={route} />}
          {page === 'reference' && <Reference route={route.slice(1)} />}
          {!known && (route.length === 0 ? <Home /> : <Missing />)}
        </Suspense>
      </div>
      {!known && (
        <footer className="foot">
          <div className="foot-inner">
            <div className="foot-brand">
              <a className="brand" href={link()}>
                <Mark size={24} />
                <span>dwgio</span>
              </a>
              <p>{home.description}</p>
            </div>
            <nav className="foot-links" aria-label="Footer">
              <div>
                <p>Documentation</p>
                {nav.map((one) => (
                  <a key={one.id} href={one.href}>
                    {one.label}
                  </a>
                ))}
              </div>
              <div>
                <p>Library</p>
                <span>Version {version}</span>
                <span>C++23, static or shared</span>
                <span>No dependencies</span>
              </div>
            </nav>
          </div>
          {statistics && <p className="foot-note">Anonymous page statistics, without cookies, by Vercel Web Analytics.</p>}
        </footer>
      )}
    </>
  )
})
