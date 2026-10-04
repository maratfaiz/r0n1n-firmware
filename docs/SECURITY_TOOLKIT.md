# Security / white-hat toolkit

## Scope of use

Every tool in this document is intended **only** for: the user's own
devices, lab/educational benches, CTF competitions, and authorized
penetration testing with the system owner's explicit permission. This is
a direct extension of the project boundaries in `VISION.md`.

Implementation requirements (mandatory for every tool covered here,
checked in code review once development begins):

- "Sharp" operations (writing/emulating onto real hardware, brute force,
  running BadUSB payloads) require confirmation in the UI.
- Regional TX-lock for Sub-GHz is a configurable option — neither hardcoded
  nor absent entirely (balancing legal-by-default behavior against the
  device owner's freedom).
- Rolling-code features are strictly a risk demonstration, with an explicit
  warning about lock desynchronization; they're never positioned as "crack
  your neighbor's gate."
- Every tool carries a one-line "what this is and when it's legal" note on
  its card (see `UX_DESIGN.md`, contextual actions/cards).

## Organized by domain

Every domain offers two entry points — a wizard (step-by-step, for
beginners) and an advanced mode (all parameters, for experts) — a direct
application of the "progressive disclosure of complexity" principle from
`VISION.md`.

### RF security (Sub-GHz)

Spectrum/frequency analysis within CC1101's limits (not an SDR — see
`HARDWARE.md`), capture/replay, fixed-code brute force (lab use), decoders
for known protocols (POCSAG, TPMS, weather stations — via decoders from
the official App Catalog rather than new code).

### NFC/RFID

Dictionary/mfkey32/nested/card-only attacks on MIFARE Classic, MIFARE Plus
SL3, magic tags, RFID Fuzzer, card/reader type identification.

### Hardware/embedded

SWD/JTAG via DAP Link — dumping/debugging firmware, a UART/SPI/I2C bridge,
SPI flash dumps, an I2C scanner, a basic logic analyzer, GPIO signal
fuzzing.

### USB security

BadUSB/BadKB (USB+BLE), a DuckyScript/JS editor with templates and a
line-by-line explanation of each step, a demonstration of exfiltration
into a virtual disk image (no real data leaves the device without explicit
consent).

### IoT / 2.4 GHz (modular, honestly labeled in the UI)

nRF24 mousejack/sniffing, ESP32 tools (Wi-Fi scan/deauth/Evil Portal/PCAP),
BLE reconnaissance — all behind a "requires external hardware" badge (see
`HARDWARE.md`).

### Auth/defensive — the "protect yourself" section

U2F, eventually FIDO2/passkey, TOTP/HOTP, a FindMy-style device finder —
tools for everyday users, not just pentesters; an important part of the
project's mixed audience.

### CTF/debugging

Notes, a timer, a hex viewer, converters, exporting findings to the
companion (see `UNIQUE_FEATURES.md`, educational/CTF layer).

## Relationship to profiles

The full list of security tools is visible in the **Pentest** profile; in
**Everyday** only the "safe" (auth/defensive) tools are visible; **CTF**
shows the same set as Pentest plus the CTF/debugging layer. Profile
mechanics are detailed in `UX_DESIGN.md`.
