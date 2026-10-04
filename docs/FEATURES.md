# Core feature set

Inherited from the official Flipper Zero firmware (release 1.4.3, see
`FIRMWARE_LANDSCAPE.md`), with R0N1N's UX layer on top. This describes
what works "out of the box" — excluding R0N1N's unique differentiators
(see `UNIQUE_FEATURES.md`) and the security toolkit (see
`SECURITY_TOOLKIT.md`).

Each section lists what the built-in apps do (checked against
`firmware/applications/`), then what is left to apps from the official
App Catalog or to later roadmap stages. R0N1N keeps the official API, so
catalog apps install without an API mismatch (see `ECOSYSTEM.md`).

## Sub-GHz

**Built in:** read decoded signals and RAW captures, save and replay them,
add a remote manually, a frequency analyzer (within what CC1101 allows —
see the "not an SDR" constraint in `HARDWARE.md`), and an external CC1101
module over GPIO as an alternative radio. Transmission follows the
official regional restrictions.

**Catalog / later:** brute force (lab use only, see
`SECURITY_TOOLKIT.md`), Subdriving (tagging a signal with GPS
coordinates), signal playlists.

## NFC (13.56 MHz)

**Built in:** read/save/emulate across ISO 14443-3A/3B/4A/4B, ISO 15693,
FeliCa, ST25TB, MIFARE Classic/Ultralight/DESFire/Plus and SLIX; MIFARE
Classic dictionary attack and reader nonce collection for mfkey32; MIFARE
Ultralight C dictionary attack; parsers for a set of supported
transit/access cards.

**Catalog / later:** the mfkey32 key recovery itself, NFC Magic, NFC Maker
(NDEF/vcard).

## 125 kHz RFID

**Built in:** read/write/emulate (EM4100, HID Prox, Indala and the other
official protocols).

**Catalog / later:** RFID Fuzzer (lab use).

## Infrared

**Built in:** universal remotes built on the IR database, learning new
commands.

## iButton / 1-Wire

**Built in:** read/write/emulate.

## BadUSB

**Built in:** DuckyScript over USB and over BLE HID, plus the `badusb`
module for JS scripts.

## U2F

**Built in:** native U2F over USB. Extending this to FIDO2/passkey and
TOTP/HOTP is a separate evaluation once its turn comes up in the roadmap.

## GPIO / Dev

**Built in:** a USB-UART bridge, GPIO control, the JS engine (mJS) with
GPIO, serial, storage, GUI, notification and BadUSB modules.

**Catalog / later:** SPI/I2C bridges, SWD/JTAG debugging (DAP Link), an
I2C scanner, sensors, a logic analyzer, GPS NMEA.

## System capabilities (UX layer)

**Built in (R0N1N):** the Home dashboard, sections (Left/Right), Control
Center (Down), Quick Actions (Up), Recent (hold OK), the Applications menu
(OK), profiles, Global Search (hold Back), the Capture Timeline and a first
Hub — in Russian, like the stock apps (menus, dialogs, settings,
card parsers) — see `ROADMAP.md`, `UX_DESIGN.md`.

**Later:** an SD search index, capture tags and export, keybinds, an
advanced file manager (see `UX_DESIGN.md`, `UNIQUE_FEATURES.md`).

## Explicitly outside the core feature set (see `HARDWARE.md`)

Wi-Fi attacks, wardriving, nRF24 tooling, video-out, AI — none of these are
part of the core, since they require external hardware or a companion.
They're described separately as modular/companion features so the base
firmware never sets false expectations.
