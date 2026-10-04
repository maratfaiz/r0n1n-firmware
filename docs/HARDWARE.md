# Flipper Zero hardware: capabilities and constraints

This document is the technical foundation for every other R0N1N concept doc.
All UX, architecture, and roadmap decisions must stay compatible with what's
described here.

Some numbers in the original research passes (our own detailed one plus a
supporting pass from another model) didn't match. What follows is a
reconciled version where disagreements are called out explicitly, and facts
checked against the official Flipper documentation
(`docs.flipper.net/zero/development/hardware/tech-specs`) and upstream
repositories as of this writing (September 2026) are marked as confirmed.

## Base platform (confirmed)

- **MCU:** STM32WB55RG — two cores: ARM Cortex-M4 @ 64 MHz (main
  application) + ARM Cortex-M0+ @ 32 MHz (BLE/802.15.4 radio stack, closed
  ST/FUS firmware).
- **Memory:** 1024 KB flash and 256 KB SRAM — **both shared between the
  application and the radio** (the official spec literally says "shared
  between application and radio"). The official specs don't publish the
  exact flash split between the radio stack and the firmware; a
  community estimate (not official) puts it around 300 KB for the radio
  stack + FUS, ~700 KB for the main firmware, with the rest shared by the
  dynamic LittleFS. Treat this as a rough guide, not a guarantee — before
  budgeting memory for the R0N1N layer, measure actual free flash/heap on
  the actual R0N1N build rather than relying on the community estimate
  above.
- **Screen:** monochrome 128×64 LCD, ST7567 controller, SPI, 1.4" —
  confirmed. 5-button D-pad + Back. No touchscreen.
- **Sub-GHz:** TI CC1101, 300–348 / 387–464 / 779–928 MHz bands (stock
  antenna/amp), ASK/OOK/(G)FSK/MSK. Confirmed transmit power of the
  built-in module is **around 10 dBm** (not 100 mW/≈20 dBm as one of the
  source documents claimed — that's an explicit discrepancy, almost an
  order of magnitude above the real figure). External CC1101 modules with
  an amplifier reach roughly 14–15 dBm.
  **CC1101 is not an SDR** — modulation and frequency must be set up front;
  there's no spectrum waterfall without external hardware.
- **NFC:** ST25R3916, 13.56 MHz (ISO-14443A/B, MIFARE Classic/Ultralight/
  DESFire, FeliCa, partial iClass/PicoPass via community apps).
- **125 kHz RFID:** implemented in software on the MCU, no dedicated chip
  (EM4100, HID Prox, Indala, Cyfral/Metakom).
- **IR:** TSOP receiver (38 kHz) + 3 transmitting LEDs.
- **iButton / 1-Wire:** read/write/emulate (Dallas DS1990A, Cyfral).
- **GPIO:** 18 pins, UART/SPI/I2C/ADC/SWD, 3.3V logic (5V-tolerant input),
  5V and 3.3V power pins.
- **Bluetooth:** BLE 5.4 (via the M0+ core). The STM32WB55 radio hardware
  also supports 802.15.4 (Thread/Zigbee), but no popular firmware exposes
  that stack on Flipper — effectively a dormant capability that can't be
  activated without rewriting part of the radio stack/FUS, i.e. out of
  reasonable scope for R0N1N.
- **Other:** USB 2.0 Type-C, piezo buzzer, vibration motor, RGB LED, 2100
  mAh LiPo battery (officially "up to 28 days" standby), microSD up to
  256 GB (FAT12/16/32/exFAT).

## Official expansion module: Video Game Module (confirmed)

- MCU: Raspberry Pi RP2040 (dual-core Cortex-M0+, up to 133 MHz, 264 KB
  on-chip SRAM).
- IMU: TDK ICM-42688-P (6-axis accelerometer/gyroscope).
- Video: DVI-D 640×480 @ 60 Hz.
- USB-C on the module itself (device/host, no USB PD).
- 11 GPIO pins, also works as a standalone Pico board.

## What microSD actually enables

Capture storage, dynamic loading of .fap apps (position-independent code,
copied into RAM at launch), key dictionaries, the IR database, asset packs,
JS scripts, the OTA update package (written to `/ext/update`, applied on
reboot). Practically all of R0N1N's extensibility should lean on SD rather
than growing the monolithic firmware.

## What requires external hardware (natively unreachable)

| Feature | Required hardware |
|---|---|
| Wi-Fi scanning/deauth/Evil Portal, wardriving | ESP32 / ESP32-S2/C5/C6 dev board over UART/GPIO |
| 2.4 GHz proprietary protocols (mousejack, keyboard/mouse sniffing) | external nRF24L01+/Si24R1 over SPI |
| Boosted/long-range Sub-GHz, LoRa | external CC1101/LoRa module with PA/LNA |
| Video-out, IMU-driven game controls, a second MCU for heavy side-tasks | official Video Game Module (RP2040) |
| AI/LLM, heavy compute, a large screen, voice input | companion (phone/PC) or an ESP32-S3 bridge |

## Hard constraints every R0N1N decision must respect

1. **RAM is the primary limit.** A FAP loads into the heap; the community
   reports OOM at just a few KB of free RAM, with ~30 KB considered a
   practical ceiling for a single allocation. Implication: one user
   application active at a time, frugal views, no large static buffers in
   R0N1N services.
2. **Flash is a ceiling, not a reserve for later.** Extend functionality
   through .fap apps on SD, not by growing the monolith.
3. **Internal flash wear.** ST guarantees a **minimum** of 10,000 write
   cycles (a guaranteed floor, not a hard cap) — which is why applications
   run from RAM/SD instead of rewriting internal flash on every action.
4. **128×64 monochrome screen, 6 physical buttons, no touch** — a hard
   ceiling on UI density; any "swipe" metaphor is implemented as D-pad
   directions, not as a gesture.
5. **The M0+/FUS radio stack is closed.** R0N1N has no access below
   `furi_hal` — neither to the BLE stack nor to raw M0+ radio.
6. **CC1101 ≠ SDR.** We can't promise "seeing" an arbitrary signal without
   pre-set modulation/frequency parameters — that's a chip limitation, not
   a firmware one.

## Practical takeaway for R0N1N

"99% of the capabilities" is achievable as a thorough, convenient exposure
of what the hardware + microSD + optional modules already allow — not as
exceeding the platform's physical limits. UX and architecture decisions
(see `UX_DESIGN.md`, `ARCHITECTURE.md`) must:

- honestly separate "native" features from "modular"/companion features in
  the UI;
- keep heavy data (search indexes, the capture timeline, dictionaries) on
  SD rather than in RAM;
- verify the actual memory budget on the current base release before
  designing new system services (Profiles, Global Search, Capture Timeline
  — see `UX_DESIGN.md`), rather than treating the numbers in this document
  as an exact budget.

## Measured, not estimated (September 2026)

Default `f7-firmware-D` (debug) target, built from `firmware/`:

| Section | Stock official 1.4.3 | R0N1N (Stage 1) |
|---|---|---|
| `.text` (code) | 627,256 B (612.6 KB) | 628,496 B (613.8 KB) |
| `.rodata` (constants) | 168,772 B (164.8 KB) | 168,884 B (164.9 KB) |
| `.data` (initialized) | 680 B | 680 B |
| `.bss` (RAM, uninitialized) | 4,748 B (4.6 KB) | 4,748 B (4.6 KB) |
| `.free_flash` | 251,532 B (245.6 KB) | 250,180 B (244.3 KB) |

Stage 1 costs ~1.3 KB of flash and no static RAM. The v2 interface with the
Stage 2 screens (Cyrillic font, icons, all R0N1N scenes) brings the debug
build to `.text` 642,592 B / 224 KB free (~22 KB over stock), `.bss` +144 B;
the release build (`COMPACT=1 DEBUG=0`) leaves ~254 KB free. The desktop
thread's stack went from 2 to 3 KB because the shell scenes scan the SD card
on it. That is plenty of headroom to
budget a Cyrillic font and the R0N1N-layer services against, as long as
they're kept frugal (see `ARCHITECTURE.md`). These numbers will shift with
each upstream release (`git subtree pull`) and should be re-measured
before locking in a flash budget for any specific R0N1N feature.

## Open questions (still need confirmation)

- Free **heap** at runtime (as opposed to flash) — the table above is a
  static link-time report; actual free RAM needs the firmware's own
  `free`/`top` CLI commands on real hardware or in the debug build, not
  inferred from the static numbers above.
- Compatibility of new R0N1N services with the official
  `api_symbols.csv` — see `ECOSYSTEM.md`.
- ~~Cyrillic font feasibility~~ — **resolved, cheap.** See below.

## Cyrillic font: resolved (Stage 0, September 2026)

The original assumption — that a Cyrillic bitmap font would need to be
sourced or hand-drawn — was wrong. `firmware/lib/u8g2` (already vendored,
used for all of Flipper's screen rendering) ships 1,673 pre-generated
fonts, of which ~23 are curated `_t_cyrillic` variants covering ASCII +
Cyrillic in one table. `canvas.c` maps the five `Font` enum values to
specific u8g2 fonts:

| `Font` enum | Current (Latin) | Cyrillic sibling in the bundle | Measured/estimated cost |
|---|---|---|---|
| `FontSecondary` | `u8g2_font_haxrcorp4089_tr` | `u8g2_font_haxrcorp4089_t_cyrillic` (exact same face) | **+1,720 B, build-verified** (see below) |
| `FontPrimary` | `u8g2_font_helvB08_tr` (bold) | no exact match; `u8g2_font_6x13B_t_cyrillic` is a bold alternative in the bundle | not measured — different metrics, needs a Stage 2 visual pick, but same order of magnitude (a few KB) |
| `FontBigNumbers` | `u8g2_font_profont22_tn` | n/a | none needed — digits only |
| `FontBatteryPercent` | `u8g2_font_5x7_tr` | n/a | none needed — digits/% only |
| `FontKeyboard` | `u8g2_font_profont11_mr` | none in the `_t_cyrillic` set | only matters if a Cyrillic on-screen keyboard *layout* is built — a separate, larger Stage 2+ UX task, not a font-availability problem |

The `FontSecondary` number is real, not estimated: swapping it to
`u8g2_font_haxrcorp4089_t_cyrillic` in `canvas.c` and running a full
`./fbt` build on the 1.4.3 base dropped `.free_flash` from 249,980 B to
248,260 B — a 1,720 B cost (higher than the two fonts' raw declared-size difference of
780 B because the old font stayed linked in for an unrelated height-adjust
comparison elsewhere in the same file; a clean swap would cost less). That
change was reverted after measuring it — actually wiring locale-aware font
selection is Stage 2 work (the localization service in `ARCHITECTURE.md`),
not Stage 0.

**Conclusion:** against a ~245 KB free-flash budget, a few KB for Cyrillic
glyph tables is noise. This was the one open item blocking "Localization
as a differentiator" in `VISION.md` from being a confident claim rather
than a hope — it no longer is.
