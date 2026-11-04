---
colors:
  background: "#0b1120"
  foreground: "#f8fafc"
  surface:
    DEFAULT: "#151e32"
    foreground: "#f8fafc"
    card: "#1a243b"
    pitch: "#0d3822"
  primary:
    DEFAULT: "#2ecc71"
    foreground: "#0b1120"
    hover: "#27ae60"
  secondary:
    DEFAULT: "#1e293b"
    foreground: "#e2e8f0"
  muted:
    DEFAULT: "#1e293d"
    foreground: "#8a99ad"
  accent:
    DEFAULT: "#2ecc71"
    foreground: "#0b1120"
  border:
    DEFAULT: "#222f47"
    subtle: "#1f293d"
  destructive:
    DEFAULT: "#e74c3c"
    foreground: "#ffffff"
  warning:
    DEFAULT: "#f39c12"
    foreground: "#ffffff"
  info:
    DEFAULT: "#3498db"
    foreground: "#ffffff"
  success:
    DEFAULT: "#2ecc71"
    foreground: "#0b1120"
typography:
  fontFamily:
    sans: "'Inter', system-ui, -apple-system, sans-serif"
    heading: "'Outfit', system-ui, -apple-system, sans-serif"
    mono: "ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, monospace"
  fontSize:
    xs: "0.625rem"
    sm: "0.75rem"
    base: "0.875rem"
    lg: "1rem"
    xl: "1.25rem"
    "2xl": "1.5rem"
    "3xl": "2rem"
    "4xl": "2.5rem"
    timer: "5.5rem"
  fontWeight:
    normal: "400"
    medium: "500"
    semibold: "600"
    bold: "700"
    extrabold: "800"
  lineHeight:
    none: "1"
    tight: "1.2"
    snug: "1.375"
    normal: "1.5"
    relaxed: "1.625"
spacing:
  0: "0px"
  1: "0.25rem"
  2: "0.5rem"
  3: "0.75rem"
  4: "1rem"
  5: "1.25rem"
  6: "1.5rem"
  8: "2rem"
  10: "2.5rem"
  12: "3rem"
  16: "4rem"
  20: "5rem"
  24: "6rem"
rounded:
  none: "0px"
  sm: "0.375rem"
  DEFAULT: "0.5rem"
  md: "0.75rem"
  lg: "1rem"
  xl: "1.5rem"
  "2xl": "1.5rem"
  "24px": "1.5rem"
  "32px": "2rem"
  full: "9999px"
shadows:
  sm: "0 1px 2px 0 rgba(0, 0, 0, 0.2)"
  DEFAULT: "0 4px 6px -1px rgba(0, 0, 0, 0.3)"
  md: "0 10px 15px -3px rgba(0, 0, 0, 0.4)"
  lg: "0 20px 25px -5px rgba(0, 0, 0, 0.5)"
  glow: "0 0 30px rgba(46, 204, 113, 0.35)"
borderWidth:
  DEFAULT: "1px"
  0: "0px"
  2: "2px"
  4: "4px"
---

# FootCoach Design System

FootCoach Design System is a high-performance, athletic coaching management framework engineered for multi-device pitch-side and desktop workflows. The design language pairs a Deep Navy midnight atmosphere (`#0B1120`) with electric Pitch Green highlights (`#2ECC71`), providing ultra-high contrast for bright outdoor stadiums, low-light locker rooms, and desktop telemetry centers.

## Principles

- **Pitch-First Visibility**: High-contrast, vibrant status indicators and large target areas ensure instant readability and touch precision when used on field-side tablets and smartphones under daylight or motion.
- **Precision Typography Split**: Brand identity and major session metrics use bold, expressive `Outfit` typography, while structured tables, attendance rosters, and tactical timing rely on highly legible `Inter`.
- **Adaptable Responsive Ergonomics**: Navigation structures scale seamlessly across devices—from a comprehensive 256px Desktop Sidebar, to a 80px Tablet Mini-Sidebar, down to an ergonomic 64px Mobile Bottom Tab Bar.
- **Glassmorphic Tactile Depth**: Rich surface elevation using deep navy cards (`#151E32`), subtle border strokes (`#222F47`), soft pitch green ambient glows, and rounded corners (`24px` to `32px`) establish modern athletic polish.

## Color & Theming

The FootCoach palette is rooted in the Deep Pitch Refined theme.

- `background`: Deep Navy `#0b1120` serves as the global canvas background across desktop and mobile layouts.
- `surface`: Elevated panel container `#151e32` for cards, headers, modal drawers, and status strips.
- `surface.card`: Slightly elevated tile surface `#1a243b` for player cards and list rows.
- `surface.pitch`: Specialized deep emerald green container `#0d3822` used for active session timers and drill cards.
- `primary` / `accent`: Electric Pitch Green `#2ecc71`. Used for active CTA buttons, active tab indicators, present status badges, and timer text glows. On `primary` fills, text is dark `background` (`#0b1120`).
- `border`: `#222f47` structural border color for panels, inputs, and tab dividers.
- `border.subtle`: `#1f293d` subtle divider line for table rows and card interiors.
- `muted`: `#1e293d` base with `#8a99ad` foreground for secondary labels, metadata, and inactive nav icons.
- `destructive`: Crimson `#e74c3c` for absent player badges, cancellation actions, and critical alerts.
- `warning`: Warm Amber `#f39c12` for excused absences, pending sync statuses, and drill warnings.
- `info`: Pitch Cyan `#3498db` for general informational notices, tactic tags, and secondary metrics.
- `success`: Vibrant Green `#2ecc71` matching `primary` for present attendance state and saved sync confirmations.

## Typography

FootCoach strictly separates headings and brand accents from functional body text and tabular metrics.

- `fontFamily.heading` (`Outfit`): Used for product titles, main headings (`h1`, `h2`, `h3`), hero session cards, and prominent numeric timers.
- `fontFamily.sans` (`Inter`): Used for functional body text, table cells, form labels, search inputs, status badges, and navigation links.
- `fontSize.timer` (`5.5rem` / `88px`): Massive timer display for live session countdowns.
- `fontSize.4xl` (`2.5rem` / `40px`): Section hero numbers and live total counts.
- `fontSize.3xl` (`2rem` / `32px`): Top page titles on desktop.
- `fontSize.2xl` (`1.5rem` / `24px`): Card titles and section headers.
- `fontSize.xl` (`1.25rem` / `20px`): Subtitle headers and drawer titles.
- `fontSize.lg` (`1rem` / `16px`): Primary button labels and prominent list item titles.
- `fontSize.base` (`0.875rem` / `14px`): Standard body text, table row content, and input fields.
- `fontSize.sm` (`0.75rem` / `12px`): Badge labels, metadata tags, and secondary action text.
- `fontSize.xs` (`0.625rem` / `10px`): Uppercase section trackers, table column headers, and tab labels.

## Spacing, Sizing & Density

Built on a strict 4px/8px spatial scale (`spacing.1` = 4px, `spacing.2` = 8px, `spacing.4` = 16px, `spacing.6` = 24px, `spacing.8` = 32px).

- **Touch Target Rules**: Primary interactive elements (buttons, nav items, status toggle buttons) enforce a minimum hit target of `48px` (height/width) on mobile and tablet touch viewports.
- **Nav Widths**: Desktop Sidebar (`256px` / `w-64`), Tablet Mini-Sidebar (`80px` / `w-20` or `96px` / `w-24`), Mobile Tab Bar (`64px` height).
- **Page Gutters**: Desktop (`px-10 py-8`), Tablet (`px-8 py-6`), Mobile (`px-5 py-4`).

## Shape & Elevation

- **Radii Scale**:
  - `rounded-24px` (`1.5rem` / `24px`): Primary card containers, hero session blocks, and action buttons.
  - `rounded-32px` (`2rem` / `32px`): Large pitch cards, player tile containers, and modal sheets.
  - `rounded-lg` (`0.5rem` / `8px`): Inputs, search bars, and compact badges.
  - `rounded-full` (`9999px`): Player avatars, status dot indicators, and icon action circles.
- **Elevation & Shadows**:
  - `shadow-sm`: Subtle inset depth on text inputs and secondary button fills.
  - `shadow-lg`: Elevated cards, desktop sidebar headers, and sticky mobile top bars.
  - `shadow-glow`: Ambient emerald aura (`0 0 30px rgba(46, 204, 113, 0.35)`) used on active live timers, primary green CTAs, and active navigation badges.

## States & Interaction

- **Hover**: Background items brighten by `10%` (`hover:bg-white/5` or `hover:bg-primary/90`).
- **Pressed / Active**: Scale down animation (`active:scale-95` or `active:scale-[0.98]`) for touch tactile feedback.
- **Focus Visible**: Focused inputs and buttons display an electric Pitch Green outer ring (`focus-visible:ring-2 focus-visible:ring-primary focus-visible:outline-none`).
- **Disabled**: Reduced opacity (`opacity-50 pointer-events-none`) with muted border `#1f293d` and grey background `#1e293b`.
- **Selected**: In status toggles, selected buttons take active accent fill (e.g. Present = Pitch Green `#2ecc71`, Absent = Red `#e74c3c`, Excused = Amber `#f39c12`) with white or dark contrast typography.

## Motion

- **Durations**: Quick micro-interactions (`150ms` transition for hover/active scale), smooth tab panels (`200ms ease-out`).
- **Live Animations**: Continuous pulse glow on `LIVE` status pill indicators (`animate-pulse`) and slow rotate (`animate-spin-slow`, 8s) on active drill stopwatch icons.

## Responsive & Accessibility

- **Breakpoints**:
  - Mobile Smartphone (`375px` to `767px`): Uses `MobileTabBar` bottom navigation, single-column player attendance stack, full-width session timer card.
  - Tablet (`768px` to `1279px`): Uses `TabletSidebar` mini icon bar (80px), multi-column player grid (2 to 3 cols), header quick counters.
  - Desktop (`1280px`+): Uses `DesktopSidebar` expanded navigation (256px), full `DataTable` view with detailed attendance status strips and export actions.
- **Accessibility**: Minimum color contrast ratio >= 4.5:1 for body text against dark navy (`#0b1120`/`#151e32`). High-contrast text `#ffffff` or `#f8fafc` on dark surfaces; `#0b1120` dark navy text on bright Pitch Green `#2ecc71` buttons. Touch targets at least `48px`.

## Component Conventions

- **Variant Names**: `default` (or `primary`), `secondary`, `outline`, `ghost`, `destructive`, `success`, `warning`.
- **Size Names**: `sm`, `default` (or `md`), `lg`.
- **Composition Style**: Slot-friendly and child-driven React components built with Tailwind utility classes.
- **Event Callbacks**: Standard handlers (`onClick`, `onValueChange`, `onSearch`).

## Content Guidance

- **Label Casing**: Title Case for main navigation links and page titles ("Attendance Tracking", "Live Tracking"); ALL CAPS with tracked spacing for technical status headers ("SÉANCE DU JOUR", "PRESENT", "ABSENT", "EXCUSED").
- **Tone**: Professional, high-performance, direct, and athletic.
- **Empty & Loading States**: Skeleton placeholders with shimmer pulse animation; explicit messaging when no players or sessions are found.

## Do's and Don't's

- **Do** pair `primary` (#2ecc71) fills with dark `background` (#0b1120) text for crisp readability.
- **Do** use `Outfit` font for bold headlines, big timers, and brand headers, and `Inter` for data tables and list items.
- **Do** enforce minimum 48px hit targets for field-side buttons and status toggles on mobile and tablet components.
- **Do** use `rounded-24px` and `rounded-32px` on surface containers to preserve the signature athletic aesthetic.
- **Don't** use standard white card backgrounds; always build on deep navy `#151e32` or pitch emerald `#0d3822` surfaces.
- **Don't** mix unrefined random border colors; stick to `#222f47` structural border and `#1f293d` subtle dividers.
- **Don't** drop navigation labels on mobile bottom tab bar; pair clean icons with uppercase 10px subtext.
- **Don't** hardcode raw pixel sizes outside the 4px/8px grid ladder.

## Composition

FootCoach layouts follow an adaptive, multi-tier application shell structure built around three primary layout architectures depending on screen size.

On **Desktop (1280px+)**, the main shell consists of a fixed `DesktopSidebar` (256px width) on the left paired with a flexible content area. The content area features a top `Header` (80px height) containing page titles, session drop-downs, and primary actions, followed by a status counter strip (`QuickCounter`) and main content panels (`DataTable` or card grids).

On **Tablet (768px - 1279px)**, the shell compresses into a compact pitch-side layout using `TabletSidebar` (80px width) with touch-optimized icon triggers. The main content zone rearranges data into responsive 2-3 column grids (`PlayerAttendanceCard` grid) for rapid visual scanning during field drills.

On **Mobile (375px - 767px)**, the shell displays a sticky top `Header` bar with quick actions, a scrollable main workspace containing stacked hero cards (`HeroTimerCard`) and player lists, and a fixed bottom `MobileTabBar` (64px height) anchored to the viewport bottom for thumb-friendly navigation.

## Qt 6.9 / QML Mapping & Implementation Guidelines

FootCoach is designed for deployment across web/React and Qt 6.9 / QML desktop/mobile platforms. The design tokens and component visual patterns map directly to Qt Quick Controls 2 and Qt QML constructs:

- **Token Mapping to QML**:
  - `colors.background`: `color: "#0b1120"` in `Window` / `ApplicationWindow` background.
  - `colors.surface`: `Rectangle { color: "#151e32"; radius: 24; border.color: "#222f47"; border.width: 1 }`.
  - `colors.primary`: `color: "#2ecc71"`.
  - `typography.heading`: `FontLoader { id: outfitFont; source: "qrc:/fonts/Outfit-Bold.ttf" }`.
  - `typography.sans`: `FontLoader { id: interFont; source: "qrc:/fonts/Inter-Regular.ttf" }`.
- **Qt Quick Control Wrappers**:
  - `Button` -> Custom `Button` component using QML `Rectangle` with `MouseArea` hover/pressed property bindings or Qt Quick Controls 2 `Button` styling template.
  - `DesktopSidebar` / `TabletSidebar` -> `Drawer` or fixed `Item` / `ColumnLayout` bound to window width state handlers (`State` / `PropertyChanges`).
  - `MobileTabBar` -> `TabBar` + `TabButton` with custom QML background and icon/text delegates.
  - `DataTable` -> Qt Quick `TableView` or `ListView` with custom delegate `Rectangle` items and `HeaderView`.
- **Qt Native Charts Alternatives (Replacing Plotly)**:
  - For cross-platform Qt 6.9 deployments without heavy web view dependencies, replace Plotly web components with **Qt Charts** (`import QtCharts 6.9` / `ChartView` with `SplineSeries`, `LineSeries`, `BarSeries`).
  - Customize `ChartView` background color to `"#151e32"`, grid lines to `"#222f47"`, and series colors to Pitch Green `"#2ecc71"` and Cyan `"#3498db"`.
  - For lightweight tactical heatmaps or pitch diagrams, use QML `Canvas` or `Shape` paths with antialiased QSG (Qt Scene Graph) rendering for 60fps pitch-side telemetry.

## Gaps & Decisions

- **Color Palette Baseline**: Extracted directly from the application's verified canvas designs, using `#0B1120` (Deep Navy) and `#2ECC71` (Pitch Green) as foundational ground truth.
- **Plotly Chart Integration**: Represented in React via custom SVG and Canvas performance analytics views, with explicit QML Qt Charts mapping rules for C++/QML targets.
- **Font Availability**: Web targets use Google Fonts (`Outfit` & `Inter`), while QML targets bundle TTF/WOFF font assets in Qt Resource files (`qrc:/fonts/`).