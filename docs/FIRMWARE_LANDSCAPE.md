# Landscape of existing firmwares and R0N1N's base strategy

## Method

Below is a reconciliation of two independent research passes (our detailed
breakdown plus a supporting pass from another model), cross-checked against
GitHub repositories (September 2026). Version numbers and release dates
didn't match between the two sources and in places resembled hallucinated
specifics — so only verified facts are listed here (repositories, active
development status, general architectural differences), not invented build
numbers. Before syncing `firmware/` with a new upstream release, re-check
the current state of each repository — links below.

## Official Firmware (OFW) — R0N1N's base

- Repository: `flipperdevices/flipperzero-firmware`.
- FreeRTOS + Furi, layered architecture (HAL → services → applications).
- Maximum stability, signed OTA updates, official App Catalog — apps from
  the catalog install on R0N1N without an API mismatch, because R0N1N
  keeps the official API.
- Regional TX restrictions on by default, doesn't store captured rolling
  codes, minimal UI customization. R0N1N keeps these defaults: its value is
  the UX layer, not unlocking the radio.
- As of 2026, Flipper Devices has resumed active support for external
  contributions with stricter review (including specific attention to
  AI-generated code touching low-level libraries) — relevant if R0N1N ever
  upstreams fixes.

**Conclusion:** the official firmware is the fork base: the most stable
code, the official app ecosystem, and a UI layer that is barely customized
— meaning it won't fight the R0N1N UX layer.

## Momentum (Next-Flip) — source of UX ideas

- Repository: `Next-Flip/Momentum-Firmware` (plus the `Momentum-Apps` and
  `Asset-Packs` satellite repos); a direct continuation of Xtreme
  Firmware, built by the same team (Xtreme officially stopped development
  in late 2024, and its developers moved to Momentum).
- The most polished UX among the forks: multiple home-menu styles, a
  Control Center with quick toggles, an advanced file manager, Asset
  Packs, a keybind system (button remapping, press/hold), JS modules
  (Storage, GUI, BLE, SubGHz, USB Disk), Bad-KB (USB+BLE), FindMy, BLE
  Spam, GPS Subdriving.
- **For R0N1N, Momentum is a reference for UX ideas only** — not a fork
  base and not a source of code: it's built on a different codebase than
  the official firmware, so components like a Control Center, file manager
  or keybinds are designed and written for R0N1N on top of the official
  firmware instead.

## RogueMaster (RogueMaster/The-Flipper-Files) — catalog map, not a base

- Repository: `RogueMaster/flipperzero-firmware-wPlugins`.
- "Kitchen sink": maximum apps, games, plugins, animations; releases ship
  frequently (weekly / with each OFW update).
- Upside — breadth of community app coverage; downside — the least
  predictable stability and the "heaviest" build; the project's own notes
  about needing to clear `/ext/apps` before updating are a sign of fragile
  migrations.
- **Role for R0N1N:** a source of catalog candidates (see `ECOSYSTEM.md`),
  filtered hard for stability and memory footprint — not forked directly.

## Xtreme Firmware — historical note

- Was once the stability/feature-completeness flagship among the forks;
  officially discontinued (Flipper-XFW) in late 2024.
- The team and its work moved into Momentum — meaning every current Xtreme
  idea worth having is already available through Momentum; there's no need
  to treat Xtreme as a separate source for R0N1N.

## Synthesis: R0N1N's base strategy

1. **Fork the official firmware** — the foundation: stability, the official
   App Catalog and API, active upstream maintenance. The fork lives inside
   this same repository, under `firmware/`, brought in via `git subtree`
   (tag `1.4.3`) so the upstream commit history (and GPL attribution) is
   preserved rather than squashed — this also keeps `git subtree pull`
   available for staying in sync with new official releases.
2. **Write R0N1N's own UX layer** on top — Home dashboard, pseudo-swipes,
   profiles (see `UX_DESIGN.md`). Other firmwares are a source of ideas,
   not code: nothing is copied from third-party custom firmwares.
3. **Community forks are a source of catalog candidates** (`ECOSYSTEM.md`),
   installed as regular apps where they're compatible with the official
   API, not code to fork.
4. **Disciplined, regular syncing with official releases** — the main risk
   of this strategy is drifting from upstream (see `ROADMAP.md`, risks
   section).

## Sources to re-verify before syncing upstream

- `https://github.com/Next-Flip/Momentum-Firmware`
- `https://github.com/RogueMaster/flipperzero-firmware-wPlugins`
- `https://github.com/flipperdevices/flipperzero-firmware`
- `https://docs.flipper.net/zero/development/hardware/tech-specs`
