# Roadmap

## Current status

**Stages 0 and 1 are done, and Stage 2 has a working first version** —
see their sections below for exactly what shipped and what was simplified.
`firmware/` is the official Flipper Zero firmware, release 1.4.3
(`git subtree`, full history), with R0N1N's shell on top: the "v2"
interface (Russian UI, one visual language for every screen), Home,
sections, Control Center, Quick Actions, Recent, the Applications menu,
profiles, Global Search, the Capture Timeline and a first Hub — all built
with `./fbt` and their drawing code checked on a host render of the real
firmware views. Nothing has been tested on a physical device yet.

Stage-duration estimates are rough, carried over from the original
research without independent verification; real planning should
recompute them now that Stages 0–1 give an actual sense of pace on this
codebase.

## Stage 0 — Fork & Foundation — done

Goal: assemble R0N1N as a fork of the official Flipper Zero firmware, set
up `ufbt`/CI, release channels, branding, a reproducible build.
Dependencies: an official release tag of
`flipperdevices/flipperzero-firmware` (see `FIRMWARE_LANDSCAPE.md`), the
toolchain.
Risk: drifting from upstream — mitigated by regular syncing with official
releases.

**Progress:** official release `1.4.3` is merged into `firmware/` via
`git subtree` (full history preserved), all 12 submodules resolve and
check out cleanly, and a stock `f7-firmware-D` build succeeds end to end
(toolchain fetch → compile → link → `.bin`/`.dfu`) — see `HARDWARE.md` for
the measured flash numbers this produced. `FIRMWARE_ORIGIN` is `R0N1N`
(reported in version info) and About opens with an R0N1N screen crediting
the official firmware. CI (`.github/workflows/build-firmware.yml`) builds
on push/PR. Cyrillic font feasibility is resolved — see `HARDWARE.md`,
it's cheap (~1-5 KB against ~245 KB free), using fonts already vendored in
`lib/u8g2`, no sourcing/hand-drawing needed. Still open: release
channels/versioning scheme, and an R0N1N boot-splash asset (the stock
animations are brand-neutral).
Outcome: an R0N1N build == official firmware + R0N1N UX layer, installable
via qFlipper ("Install from file") or an SD-card update package.

## Stage 1 — MVP: Home + navigation + Control Center — done

Goal: the new Desktop dashboard (time/date/battery/indicators),
pseudo-swipes, Control Center, basic favorites/recent.
Dependencies: GUI/`ViewDispatcher`, notification (see `ARCHITECTURE.md`).
Risk: Desktop RAM/performance — mitigated by profiling and lazy `View`s.
Outcome: the device feels like a cohesive shell rather than an app list —
a demonstrable MVP prototype.

**Shipped** (the "v2" interface, `docs/UX_DESIGN.md`): all screens are
desktop-service scenes (`firmware/applications/services/desktop/scenes/`)
drawn with shared R0N1N views in the GUI service — `r0n1n_ui` (header bar,
list rows, tiles, captions, scrollbar) and `R0n1nList` / `R0n1nGrid` /
`R0n1nCarousel` (`gui/modules/`). One readable font everywhere (the stock
FontSecondary switched to the same face with Cyrillic, +1.7 KB), selection is
always a filled rounded shape, icons are in `assets/icons/R0N1N/`.

- **Home**: battery, big clock, Russian date, active profile and a landscape
  picture over the whole screen. gui draws no status bar over the desktop
  (only over app windows), and the dolphin's blocking animations are not
  shown (a pending level-up is applied directly), so Home never falls back
  to the stock dolphin screen.
- **Navigation law**: Left/Right = sections of the active profile
  (carousel), Up = Quick Actions, Down = Control Center, OK = Applications
  menu, hold OK = Recent, hold Back = Search (opened on release, so holding
  on for the stock 5 s power-off menu still works).
- **Control Center**: Bluetooth, sound, vibration, stealth (silent) mode,
  lock, power (off/reboot), profile, settings, plus a brightness slider; the selected
  tile's name and state are in the header, a corner mark means "on".
- **Quick Actions**: six configurable slots (`r0n1n_settings.c`,
  `/int/.r0n1n.settings`), defaults NFC / Sub-GHz / IR / BadUSB / Files /
  Settings; hold OK on a tile to reassign it. The stock four favorites stay
  on hold Left/Right.
- **Recent**: apps that actually ran, newest first, with the time since
  launch ("5 мин"). `LoaderEvent` gained a `name` field for this, set only
  when the Loader isn't already running an app (the official Loader
  publishes the event before its lock check).
- **Archive** is reachable from Quick Actions and the menu: in the official
  firmware it isn't a Loader app and was only on Down-short.

**Deliberately not done** (see `UX_DESIGN.md` for the full model):
- **Recent files / args** — Recent relaunches apps *without* the arguments
  they were first started with: replaying args blindly isn't safe (stale
  file paths, an RPC session marker). Recent files are the Capture Timeline's
  job.
- **Recent across reboots** — in-memory only.
- **USB-mode, TX-lock and external-module tiles** in Control Center — the
  official firmware has no such switch to flip (TX restrictions are always
  on), so they'd be stubs.

## Stage 2 — Profiles + Global Search + Capture Timeline — first version

Goal: Profile Manager (Everyday/Pentest/Dev/CTF), global search (index on
SD), a unified capture feed.
Dependencies: storage, hooks into core apps' capture-save paths.
Risk: the search index and memory — mitigated by keeping it on SD, not RAM.
Outcome: R0N1N's key UX differentiator (see `UNIQUE_FEATURES.md`, item 1)
is working.

**Shipped** (`r0n1n_catalog.c`, `r0n1n_shell.c` and the scenes):
- **Profiles**: four profiles, each with its own set and order of sections
  (Radio, Cards, IR, USB, Dev); switched from Control Center or Settings in
  two presses, shown on Home, saved on the SD card.
- **Sections and the Applications menu**: apps grouped by section; the Dev
  section is a tool grid (GPIO, UART, I2C, SPI, SWD, logic analyzer, CLI, JS)
  that launches catalog apps from `/ext/apps` when installed and says what to
  install when not.
- **Global Search**: apps, settings, apps on the SD card and saved captures
  whose names contain the query; results open the app or the file in its app.
- **Capture Timeline**: every saved NFC / Sub-GHz / IR / RFID / iButton file,
  newest first, with its time; OK opens it, hold OK deletes it. File times
  come from `storage_common_mtime()` — a small internal storage call added
  for this (kept out of the SDK: `FileInfo` can't grow without breaking
  existing apps).
- **Hub, first version**: the apps installed on the SD card by category, and
  launching them (see Stage 3 for installing).

**Simplified / not done yet:**
- **Search is a live scan, not an SD index**; queries can be typed in
  Russian (the keyboard has a Russian layout).
- **Captures aren't tagged or exported** (frequency, protocol, export to the
  companion), and there are no hooks in the apps' save paths — the timeline
  reads their folders.
- **Profiles don't yet change** Quick Actions, Control Center content,
  density or hints — only the sections.

## Stage 3 — R0N1N Hub + app compatibility

Goal: the on-device catalog with an API-compatibility filter, an official
R0N1N target in `ufbt` + GitHub Actions, microSD layout and migrations
(see `ECOSYSTEM.md`).
Dependencies: a stabilized `api_symbols.csv`, CI.
Risk: "API mismatch" — mitigated by versioning and a build matrix.
Outcome: apps install and update without friction.

## Stage 4 — Companion v1

Goal: the PC/mobile companion — capture sync, catalog/install, backup,
remote screen — built on RPC (see `COMPANION.md`).
Dependencies: RPC, a stable Capture Timeline format.
Risk: cross-platform reach/BLE stability.
Outcome: an ecosystem, not just firmware.

## Stage 5 — Workflow/JS engine + first-class external modules

Goal: JS "recipes," auto-detection and onboarding for ESP32/nRF24/
CC1101/VGM, Sweep mode (see `UNIQUE_FEATURES.md`, items 4–5).
Dependencies: the official JS engine and its modules, module drivers.
Risk: JS-runner memory footprint, inconsistent pinouts across modules.
Outcome: automation and modularity become first-class citizens.

## Stage 6 — AI bridge + educational/CTF layer + polish

Goal: the optional AI bridge in the companion (approve-per-action, offline
mode), built-in labs/CTF mode, a stability audit, integration tests,
documentation.
Dependencies: the companion, RPC.
Risk: AI-action safety/ethics — mitigated by risk tiers, logging,
off-by-default behavior.
Outcome: R0N1N 1.0.

## MVP vs. the full release

**MVP (Stages 0–2):** a fork of the official firmware + the Home dashboard,
pseudo-swipes, Control Center, profiles, Global Search, Capture Timeline.
Functionally the MVP equals the official firmware (stability and app
compatibility are already there); the added value is the UX layer on top.

**Full release / R0N1N 1.0 (Stages 3–6):** adds the R0N1N Hub with app
compatibility solved, the companion ecosystem, the workflow/JS engine,
first-class external modules, the optional AI bridge, and the
educational/CTF layer.

## Main project risks

1. **Memory/performance** — the R0N1N layer must not make free heap worse
   than the stock official firmware (verified starting at Stage 1, see
   `ARCHITECTURE.md`).
2. **Upstream drift** — disciplined, regular syncing with official releases.
3. **App fragmentation** — API versioning and the CI target (Stage 3).
4. **Staying within white-hat boundaries** — TX-lock as an option,
   confirmation for "sharp" operations, AI actions off-by-default (see
   `SECURITY_TOOLKIT.md`, `VISION.md`).

## How to read this roadmap

Each stage is a direction and a dependency order, not a rigid schedule.
Before starting any stage, re-check `HARDWARE.md` (open questions about the
actual memory budget) and `FIRMWARE_LANDSCAPE.md` (how current upstream is
at the time work starts).
