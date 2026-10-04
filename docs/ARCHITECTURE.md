# Firmware architecture

## Base decision

A fork of the **official Flipper Zero firmware**, currently release 1.4.3
(see `FIRMWARE_LANDSCAPE.md` for why this base was chosen); R0N1N's UX
components are written for it, not copied from other custom firmwares.
We're not writing an OS from scratch — that would break compatibility with the app
catalog and throw away years of community work on radio-stack stability,
drivers, and the HAL.

## Layers (inherited from the official firmware's model)

1. **Hardware / `furi_hal`** — nothing below the HAL is touched; we work
   only through the public `furi_hal`, never directly against the STM32
   HAL — a precondition for staying compatible with future official
   releases.
2. **FreeRTOS + Furi (Furi OS)** — the scheduler, threads, records
   (`record_open`/`record_close`), services. Not modified.
3. **Services** — GUI/`ViewDispatcher`, storage (LittleFS + SD), input,
   notification, RPC (control over USB/BLE — the companion transport, see
   `COMPANION.md`), BLE.
4. **Core apps** — Sub-GHz, NFC, RFID, IR, GPIO, iButton, BadUSB, U2F —
   inherited from the official firmware with almost no logic changes, only
   integration points with the new UX layer (see below).
5. **R0N1N layer** — the project's main technical contribution,
   implemented mostly as system apps/services on top of Furi so the
   monolith doesn't grow:
   - the new **Desktop/Home** (dashboard + pseudo-swipes),
   - the **navigation manager** (desktops),
   - **Profile Manager**,
   - **Global Search** (index on SD),
   - the **Capture Timeline** service (hooks into core apps' save paths),
   - **R0N1N Hub** (catalog with a compatibility filter),
   - the **Workflow/JS runner** (built on mJS),
   - a **Localization service**: UI strings resolved through a string
     table keyed by locale, Russian as the default (see `VISION.md`,
     "Localization as a differentiator"). Strings live in core apps and
     the R0N1N layer alike are pulled through this service rather than
     hardcoded, so adding a locale later doesn't require touching call
     sites. The open risk isn't the string-table design — it's whether a
     legible Cyrillic bitmap font fits the flash budget (see
     `HARDWARE.md`).
6. **FAP apps on SD** — all further extensibility, without growing the
   monolith (see `ECOSYSTEM.md`).

## Memory management (critical — see `HARDWARE.md`)

Real limits need to be respected, not designed around "future hardware
that might exist":
- One user app active at a time.
- Frugal `View`s, no heavy static buffers in new services.
- Profile with the firmware's own `top`/`free` CLI on every significant
  change to the R0N1N layer, not only "at the end."
- Minimal `stack_size` for new threads, so they don't waste heap.
- R0N1N services must be "lazy": data (the search index, Capture Timeline)
  lives on SD, with only the current screen's working set in RAM.
- **Design goal:** the R0N1N layer must not make free heap worse than
  the stock official firmware on the same API version — measurable, verified during
  the prototype stage (`ROADMAP.md`, Stage 1).

## Firmware updates

The official update mechanism is kept as-is: the update package is written to
SD at `/ext/update/`, applied offline on reboot by a small bootloader,
without touching user data; DFU via qFlipper serves as emergency recovery.
Signed releases, a public changelog.

## Stability and compatibility

- Mandatory integration tests before every release (mirroring current
  upstream OFW practice).
- Regional TX-lock as a configurable option (see `SECURITY_TOOLKIT.md`).
- "Sharp" security features gated behind explicit UI confirmation.
- Separate dev/release build channels.
- Compatibility with the Flipper ecosystem through the public Furi API and
  `api_symbols.csv` versioning (see `ECOSYSTEM.md`).

## Companion channel

RPC over USB/BLE (already present in the firmware as a service) — the only
transport for the companion and the AI bridge. The device stays a thin
client; heavy computation stays on the companion (see `COMPANION.md`).
This is an architectural principle, not an implementation detail: no
feature critical to the device's base, standalone operation may depend on
the companion.

## Stage 1 finding: Home/Control Center/Quick Actions/Recent are core-service patches, not FAPs

Stage 1 (see `ROADMAP.md`) settled this for the pieces it touched, by
building them rather than by deciding up front. Home, Control Center,
Quick Actions, and Recent are all new scenes/views inside the official
firmware's existing `desktop` service (`firmware/applications/services/desktop/`),
not separate FAP apps — because they need to *replace and extend* that
service's own Home screen, input law, and view stack, which isn't
something a FAP sitting on top of the public SDK can reach into. The one
change outside `desktop/` was additive and minimal: a `name` field on
`LoaderEvent` (`loader.h`), needed for Recent to know what app was about
to launch, set only when the Loader isn't already running an app, every
other subscriber unaffected. One consequence specific to the official base:
Archive isn't a Loader app there and was only reachable from Down on Home,
so with Down given to Control Center, Quick Actions carries a fixed Archive
entry (`desktop_run_archive` in `desktop.c`).

The v2 interface kept the same pattern and added to it: every R0N1N screen
(sections, menu, profiles, search, captures, Hub, Dev tools) is a desktop
scene, and they share one instance each of three new GUI modules —
`R0n1nList`, `R0n1nGrid`, `R0n1nCarousel` (`gui/modules/`), drawn with
`gui/r0n1n_ui.c` — reset on scene entry, so extra screens cost almost no
RAM. Screens report actions as custom events tagged in their high bits, so
a list index can never collide with the desktop's global events. Two small
additions outside `desktop/`: FontSecondary uses the Cyrillic variant of the
same face (`canvas.c`), and `storage_common_mtime()` (a new storage command,
`storage/storage_mtime.h`) gives a file's own modification time for the
Capture Timeline — deliberately not an SDK function, since `FileInfo` can't
grow without breaking apps that allocate it.

## Open question (Stage 2 and later)

Whether *later* R0N1N services — Global Search, Capture Timeline, Profile
Manager — follow the same core-patch pattern, or turn out simpler and
safer for upstream compatibility as privileged FAPs, is still open. Their
needs differ from Stage 1's: they're less about intercepting an existing
service's input/draw and more about indexing and cross-app data, which a
FAP has a more plausible path to without patching core. That decision
still follows from prototyping each one, not from deciding it here.
