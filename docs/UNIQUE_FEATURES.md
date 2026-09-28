# R0N1N's unique features

Features whose integration — or even framing — doesn't exist as a cohesive
whole in any existing fork. Some of these are thin clients running on a
companion rather than code on the STM32 (see `HARDWARE.md` for the
platform's limits, `COMPANION.md` for the companion-layer architecture).
That's a deliberate consequence of the hardware constraints, not a
workaround.

## 1. Profiles + Global Search + Capture Timeline

Similar ideas show up individually across different forks, but as a single
OS-level paradigm (see `UX_DESIGN.md`) they don't exist anywhere. This is
R0N1N's primary, lowest-risk source of differentiation (fully native,
requires no external hardware) — top priority in the roadmap, see
`ROADMAP.md`, Stage 2.

## 2. R0N1N Companion + AI bridge (agentic layer)

An optional companion that can:
- explain a captured signal/card in plain language;
- generate BadUSB/JS scripts from a description of the task;
- control the Flipper over USB/BLE.

The community already has working prototypes of this class (LLM bridges
controlling a Flipper via RPC over USB/BLE, MCP servers for agentic
control) — this confirms the approach is technically feasible, but any
specific third-party projects need to be re-checked for current status and
licensing before we rely on them in implementation (see `COMPANION.md`).

The R0N1N version needs to be:
- **secure by default**: approve-per-action, risk tiers for actions, a
  full log;
- **capable offline**: a local model (e.g. via Ollama) as an option,
  without mandatory data sent to an external service;
- **off by default** — enabled only through an explicit user action.

AI lives on the companion/an external bridge, not on the STM32 — the
Flipper itself stays a thin RPC client.

## 3. R0N1N Sync — companion ↔ catalog/GitHub

Auto-pulling the IR database, Sub-GHz protocols, dictionaries, and apps
from curated repositories through the companion, with versioning and
API-compatibility checks (solving "API mismatch" — the community's main
pain point, see `ECOSYSTEM.md`).

## 4. JS workflow engine ("recipes")

Chains of actions ("load profile X when an SD card is inserted," "on
button press, emulate card Y and log it to Capture Timeline"), edited on
the companion, executed natively on the mJS engine (confirmed: on 32-bit
ARM, mJS takes about 50 KB of flash and under 1 KB of RAM — i.e. cheap on
resources — see `HARDWARE.md`). Implemented technically as a JS module on
top of R0N1N's system services (Capture Timeline, Profile Manager), not a
separate runtime.

## 5. Sweep mode (counter-surveillance)

RSSI "warmer/colder" tracking of a single chosen target on the built-in
radio; BLE/Wi-Fi presence detection only through an external module
(ESP32), honestly labeled as a modular feature; a silent, LED-only
indication mode.

## 6. Educational / CTF layer

Built-in interactive lab scenarios plus a CTF mode (notes, a timer,
exporting findings to the companion) — a direct answer to the project's
mixed audience (see `VISION.md`).

## 7. Native Russian-language UI

A proper localization layer (string tables, not hardcoded strings) with
Russian as the default locale — something no existing fork treats as a
first-class feature. Fully native, no companion or external hardware
required, so it belongs in the same early-priority bucket as items 1 and 8
below. The font risk this raised is resolved and cheap (see
`HARDWARE.md`): the vendored `u8g2` library already ships Cyrillic
variants of Flipper's own fonts, at a cost of a few KB against a ~245 KB
free-flash budget — confirmed with a real build, not estimated. What's
left is a software/UX problem — the string-table service itself
(`ARCHITECTURE.md`) and picking Cyrillic font substitutes for the two
`Font` enum values that don't have an exact-face match in the bundle.

## 8. "Explain this capture" — offline heuristics, no AI

No network, no companion: from metadata (frequency/modulation/protocol,
already determined by existing decoders) produce a human-readable hint like
"looks like: a fixed-code gate remote / a TPMS sensor / a weather station."
A fully native feature that requires no external hardware — a good
candidate for early implementation, not tied to the companion.

## Prioritization (for the roadmap)

Of the items above, **1, 7, and 8** are fully native and don't depend on a
companion/external hardware — they should come earlier than the rest.
Items 2–4 and part of 5 require a companion or an external module — it
makes sense to plan them once the companion transport (RPC over USB/BLE)
has stabilized (see `ROADMAP.md`, `COMPANION.md`).
