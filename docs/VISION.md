# R0N1N Vision

## TL;DR

R0N1N is not "yet another fork with a pile of apps" — it's a UX-first
operating shell on top of the official Flipper Zero firmware: a single
Home dashboard (time/date/battery), "pseudo-swipe" navigation on the
5-button D-pad, a unified app catalog with
search/favorites/recent, and cross-cutting "mode profiles" (Everyday /
Pentest / Dev / CTF) that reshape the menu around the task at hand.

By 2026 the Flipper Zero firmware market (the official firmware and the
community custom firmwares) has functionally converged — they differ in
stability, polish, and the volume of bundled apps, not in raw capability (see
`FIRMWARE_LANDSCAPE.md`). The main unaddressed gap isn't features — it's
**coherence and usability**. That's where R0N1N aims to compete.

## Three principles

1. **UX-first, not feature-first.** The competitive edge lies in
   convenience: how many presses to the action you need, how legible the
   screen is to a newcomer, how fast an expert reaches their goal. We're
   not competing on the number of bundled apps — that's already solved by
   the community catalog (see `ECOSYSTEM.md`).

2. **Progressive disclosure of complexity.** One device serves both a
   beginner and a pentester. The answer isn't two builds, but
   **modes/profiles** (Everyday / Pentest / Dev / CTF) that change
   interface density: a beginner sees large hints and wizards, an expert
   sees dense lists, keybinds, and a CLI. Details in `UX_DESIGN.md`.

3. **Honesty about the hardware.** R0N1N never promises what the STM32WB55
   can't deliver. Features that require external modules or a companion
   are marked with a distinct badge in the UI and never presented as
   "built-in." Technical grounding in `HARDWARE.md`.

## Localization as a differentiator

None of the existing firmwares, official or custom, ship a
native, first-class localization layer — English (or hardcoded strings) is
the assumption baked into their UI code. R0N1N's default UI language is
**Russian**, driven by a proper string-table localization architecture
(not hardcoded Cyrillic strings), so other locales can be added later
without rework. This is a genuinely unoccupied niche, not a cosmetic
choice — see `ARCHITECTURE.md` for the technical approach. The
Cyrillic-font/flash-budget question this raised turned out cheap to answer
(see `HARDWARE.md`): the glyphs already exist in the vendored `u8g2` font
bundle, at a cost of a few KB.

## The analogy

Not "yet another custom firmware," but what GrapheneOS/LineageOS are to
Android: a carefully assembled shell on top of an open base, with its own UX
philosophy and ecosystem — not a low-level kernel fork.

## Project boundaries (white-hat / legality)

R0N1N is a legal open-source project that customizes the firmware of a
commercially available device, in the spirit of the community custom
firmwares, and under the same GPL-3.0 license as the official firmware it
is built on (see `LICENSE`). All security features
(`SECURITY_TOOLKIT.md`) are intended only for the user's own devices, lab
benches, CTF competitions, and authorized penetration testing, with
explicit UI warnings and confirmation prompts for "sharp" operations. The
project does not develop or distribute tooling for DoS, mass targeting,
evading detection in genuinely malicious scenarios, or supply-chain
compromise — these are explicitly out of scope.

## What R0N1N does not try to do

- It doesn't rewrite Flipper's kernel/HAL — it works on top of the public
  API (`furi_hal`), never directly against the STM32 HAL (see
  `ARCHITECTURE.md`).
- It doesn't promise features that natively require an SDR/Wi-Fi/a large
  screen — those are honestly pushed to the modular/companion layer
  (`HARDWARE.md`, `COMPANION.md`).

## Current status

Right now R0N1N exists only as a set of concept documents in this
repository. Actual development (forking, building, code) begins as a
separate decision after this documentation has been reviewed — see
`ROADMAP.md`, Stage 0.

## How to sanity-check this Vision

Any feature added to the roadmap or architecture should answer three
questions:
1. Does it shorten the user's path to their goal (fewer presses, less
   menu-wandering), or does it just add one more list item?
2. Does it adapt to the active profile (Everyday/Pentest/Dev/CTF), or does
   it clutter the interface the same way for everyone?
3. Does the UI honestly reflect where the capability actually comes from
   (native / needs a module / needs a companion)?

If the answer to any question is no, the feature isn't ready to be
included in R0N1N as currently framed.
