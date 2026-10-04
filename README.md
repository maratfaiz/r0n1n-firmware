# R0N1N

**R0N1N** is a custom Flipper Zero firmware built as a UX-first operating
shell — not yet another "fork with a pile of apps."

> **Status: in development.** `firmware/` is the official Flipper Zero
> firmware (1.4.3) with R0N1N's shell on top: a Russian UI, Home, sections,
> Control Center, Quick Actions, Recent, profiles, search, the Capture
> Timeline and a first Hub, verified against actual builds (Stages 0–2 of
> the roadmap; not yet tested on a device). See [`docs/ROADMAP.md`](docs/ROADMAP.md),
> "Current status," for exactly what's shipped, what's simplified, and
> what's next.

## The idea in short

By 2026, Flipper Zero firmwares, official and custom alike, have
functionally converged — they differ in stability and polish, not in
capability. R0N1N starts from the premise that the market's real gap isn't
features, it's **coherence and usability**: a single Home dashboard instead
of an app list, predictable "pseudo-swipe" navigation on the D-pad,
cross-cutting mode profiles (Everyday / Pentest / Dev / CTF), a unified
capture feed, and global search. Details in
[`docs/VISION.md`](docs/VISION.md).

Technically, the project is built on the official Flipper Zero firmware,
with R0N1N's own UX layer on top — not a from-scratch kernel rewrite, and
no code taken from third-party custom firmwares. Anything that physically
doesn't fit on the STM32WB55 (Wi-Fi attacks, AI, video-out, SDR) is
honestly pushed to the level of optional external modules or a companion
app, rather than promised as "built-in."

## Documentation

| Document | What it covers |
|---|---|
| [`docs/VISION.md`](docs/VISION.md) | Project vision, three principles, boundaries (white-hat/legality) |
| [`docs/HARDWARE.md`](docs/HARDWARE.md) | Flipper Zero hardware capabilities and hard constraints |
| [`docs/FIRMWARE_LANDSCAPE.md`](docs/FIRMWARE_LANDSCAPE.md) | Analysis of existing firmwares and the base-fork strategy |
| [`docs/UX_DESIGN.md`](docs/UX_DESIGN.md) | Home dashboard, pseudo-swipe navigation, profiles, cross-cutting services |
| [`docs/FEATURES.md`](docs/FEATURES.md) | Core feature set inherited from the official firmware |
| [`docs/UNIQUE_FEATURES.md`](docs/UNIQUE_FEATURES.md) | What sets R0N1N apart from other forks |
| [`docs/SECURITY_TOOLKIT.md`](docs/SECURITY_TOOLKIT.md) | The white-hat security toolkit and its scope of use |
| [`docs/ECOSYSTEM.md`](docs/ECOSYSTEM.md) | The app ecosystem and solving the "API mismatch" problem |
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | The firmware's layered architecture, memory management |
| [`docs/COMPANION.md`](docs/COMPANION.md) | The PC/mobile companion and the optional AI bridge |
| [`docs/ROADMAP.md`](docs/ROADMAP.md) | Development stages, MVP vs. the full release, risks |

## Legality

R0N1N is a legal open-source project that customizes the firmware of a
commercially available device, in the spirit of the community custom
firmwares. All security features are intended only for the user's own
devices, lab benches, CTF competitions, and authorized penetration
testing. Details in [`docs/VISION.md`](docs/VISION.md) ("Project
boundaries") and [`docs/SECURITY_TOOLKIT.md`](docs/SECURITY_TOOLKIT.md).

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md) — at this stage, reviewing and
refining the documentation is more useful than code.

## License

[GPL-3.0](LICENSE) — same as the official Flipper Zero firmware R0N1N is
built on.
