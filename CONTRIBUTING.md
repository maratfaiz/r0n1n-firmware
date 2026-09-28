# Contributing to R0N1N

## Current stage of the project

R0N1N is **in active development**: `firmware/` is the official Flipper
Zero firmware (brought in via `git subtree`, see
`docs/FIRMWARE_LANDSCAPE.md`) with R0N1N's changes on top, and
Stages 0–1 of `docs/ROADMAP.md` are done — the Home dashboard, Control
Center, Quick Actions, and Recent all exist and build. See
`docs/ROADMAP.md`, "Current status," for exactly what's shipped, what's
simplified, and what's next.

Useful contributions at this point:

- code for the current or next roadmap stage — read the relevant
  `docs/ROADMAP.md` section and the "Shipped"/"Deliberately not done"
  notes under the last completed stage first, so a PR doesn't duplicate or
  contradict a scope decision that was already made deliberately;
- reviewing and refining the documents in `docs/` — factual inaccuracies,
  stale references to upstream releases, unrealistic timeline estimates;
- expanding `docs/HARDWARE.md` and `docs/ECOSYSTEM.md` with verified facts
  (cited to a source) wherever something is flagged as "needs
  confirmation."

Firmware PRs are expected to build (`FBT_NO_SYNC=1 ./fbt` from `firmware/`)
before review — CI (`.github/workflows/build-firmware.yml`) checks this on
every PR regardless, but catching it locally first saves a round trip.

## How to propose documentation changes

1. One PR per document, or a tightly related group of documents. Don't mix
   edits to `docs/HARDWARE.md` with edits to `docs/ROADMAP.md` in the same
   PR unless one directly follows from the other.
2. Any factual claim about Flipper Zero hardware or another firmware
   (versions, specs, licenses) should cite a source in the PR description.
3. Use "this proposes X" rather than "we decided X" phrasing — reserve the
   latter for things that are genuinely already settled in existing
   documents.

## White-hat / legality

Any contribution that extends `docs/SECURITY_TOOLKIT.md` or future
security functionality must stay within the boundaries described in
`docs/VISION.md` ("Project boundaries") — the user's own devices, labs,
CTF, and authorized penetration testing only. Proposals for DoS tooling,
mass targeting, evading detection in genuinely malicious scenarios, or
supply-chain compromise are declined without discussion.

## License

The project is distributed under GPL-3.0 (see `LICENSE`) — the license of
the official Flipper Zero firmware it is built on. Any code added must be
compatible with this license, and must be either R0N1N's own or come from
the official firmware: no code is taken from third-party custom firmwares.
