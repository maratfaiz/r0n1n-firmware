# R0N1N UX/UI concept

Given constraints (details in `HARDWARE.md`): a 128×64 monochrome screen, a
5-way D-pad + Back, no touch, single-digit-to-low-tens of KB of free RAM.
Any "swipe" metaphor is implemented through buttons — D-pad directions need
to become predictable "gestures," not a metaphor for its own sake.

## Home — a "dashboard," not a launcher

The stock approach across every fork is "home menu = app list." R0N1N
changes the role of the home screen itself: it shows the **device's
state**, not a list of things you could launch.

Large **time**, **date** below it, **battery** in the corner, a thin strip
of system indicators (BLE, USB, SD, active radio, current profile). The
mascot (see "Mascot") lives in the idle animation underneath, not on Home.
Minimal by design: the essentials are visible immediately, no menu diving
required.

## Navigation model — "pseudo-swipes"

A single, consistent law of motion from Home, identical across every
section:

| Action | Result |
|---|---|
| **OK** | Open the App Launcher (grid/list with categories) |
| **Left / Right** | Switch "desktops" (Radio ⇄ Cards ⇄ IR ⇄ USB/HID ⇄ Dev/GPIO); the set and order depend on the active profile |
| **Down** | Control Center: quick toggles (BLE, backlight, sound/vibration, silent mode, USB mode, TX-lock, active external module), sliders, profile switcher |
| **Up** | Quick Actions / Favorites: a customizable grid of favorite actions, hold-OK to move slots, hold-Back to pick an action; also shows the "last capture" |
| **Hold Back** | Global search (apps, capture files, actions, settings), typed via the system keyboard, incremental |
| **Long-press OK on Home** | Recent — last-used apps/files |

It feels like flipping through phone home screens, but it's implemented
entirely with buttons — no imitation of gestures that would require touch.

**As implemented** (see `ROADMAP.md`, Stages 1–2): OK opens the R0N1N
Applications menu (categories, then Files, Captures, Search, Hub, all apps,
Settings); Left/Right open the active profile's sections as a carousel
(Right starts at the first, Left at the last); hold Back opens Search on
release, because holding it for 5 s is still the stock power-off menu. The
stock favorites stay on hold Left/Right.

## Visual language ("v2")

Checked as real-resolution mockups before it was built:

- one readable font for all text (FontSecondary, now with Cyrillic);
- an inverted 10 px header bar: title left, an "n/N" position or the state
  on the right; clock and battery only on Home and in Control Center;
- selection is always a filled rounded shape with inverted content;
- tiles carry icons only, the selected tile's full name is in a caption
  (or, in Control Center, in the header together with its state);
- 13 px list rows, four visible, with a scrollbar;
- no hint rows: OK opens, Back returns, arrows move — on every screen;
- no system status bar over R0N1N screens: they own the whole 128x64.

Russian everywhere, stock apps included:

- FontPrimary (bold headings) has no Cyrillic, so a heading that contains
  Cyrillic is drawn in the Cyrillic FontSecondary face, struck twice as
  bold (`canvas.c`); ASCII-only headings keep the stock look;
- text layout (text box, scrolling text, multiline, truncation) steps
  through whole UTF-8 characters (`gui/utf8_i.h`), not bytes;
- apps keep their FAM names for launching; the stock menus show Russian
  names (`loader_display_name()`);
- not translated: protocol dumps and file formats (UID, ATQA, Sub-GHz
  key lines), CLI, logs, debug apps;
- the keyboard (`text_input.c`) has Russian, Latin and 123 layouts behind
  one key and edits whole UTF-8 characters. File names on the SD card
  (FAT, code page 850) can't hold Cyrillic, so it is transliterated to
  Latin on Save unless the caller keeps Unicode
  (`text_input_set_allow_unicode()`, Search does);
- FontSecondary gained the letters its source lacked (э, ё, Ё), rebuilt
  with u8g2's bdfconv; the other glyphs are unchanged.

## Simple mode

For people who want the basics only (R0N1N Settings -> "Простой режим").
Home shows a big clock and date and "OK Меню"; any arrow or OK opens a
menu with one big item per screen (32 px icon, 10x20 font): TV remote
(opens the universal TV remote directly), gates (Sub-GHz), cards (NFC),
intercom key (iButton), key fob (125 kHz RFID), Files, Academy, power off,
and "Обычный вид", which leaves simple mode after a confirmation.

## Wallpaper

The Home picture (top-right, 51×46) is chosen in R0N1N Settings → "Фон":
Горы, Череп, Маска, Город, Радио, Волны, or Нет. Built-in 1-bit art in
`assets/icons/R0N1N` (`r0n1n_wallpapers[]`); the choice is `wallpaper` in
R0n1nSettings v4 (v1–v3 migrated). The clock, date and profile stay on the
left so they remain readable over any picture.

## Feedback and search

- **Feedback** (R0N1N Settings → "Отклик"): a short click (a beep plus a
  brief vibro) when something is selected in the shell, an error buzz when a
  popup reports a problem, success cues from the apps themselves. Sound and
  vibro still obey the Control Center toggles and stealth mode. Off disables
  the shell click; default on. `desktop_r0n1n_feedback()`.
- **Search index**: captures are listed from `/ext/.r0n1n/captures.idx`
  instead of walking every capture directory and stat-ing every file on each
  search. The index is rebuilt only when the SD card's change counter differs
  from the one recorded (in RAM) at the last build, so a saved or deleted
  capture refreshes it and nothing else costs a rescan.

## Academy

`applications/system/academy`, on the SD card in Tools and in the R0N1N
menu: short lessons (buttons, Home, simple mode, TV remote, files,
charging and SD card, updating, responsible use), each ending with one
question; passed lessons are remembered.

## Mascot

R0N1N's mascot is a ninja-cat (кот-ниндзя): a sitting cat with a ninja
mask band across the eyes, a headband whose tails stream to the side, and a
curled tail. It replaces the stock Flipper dolphin everywhere the dolphin
appeared as the device's character, in 1-bit:

- **lock screen** — the "how to unlock" hint (`R_Mascot_45x42`, in
  `desktop/views/desktop_view_locked.c`);
- **idle animation** — the built-in idle (`assets/dolphin/internal/
  L1_Tv_128x47`) is a sitting cat that blinks and dozes; it runs under the
  Home dashboard as before, so it shows through anywhere the dashboard does
  not cover it. The English speech-bubble easter eggs are dropped;
- **level-up** — the celebration one-shots (`assets/icons/Animations/
  Levelup1_128x64`, `Levelup2_128x64`) show the cat with rising twinkles
  and up-chevrons;
- **boot splash** — a compact mascot head (`R_MascotHead_24x22`) settles in
  beside the wordmark once the blade lands.

The stock `WarningDolphin` icon stays as the generic "something went wrong"
symbol in error dialogs (SD, NFC, Sub-GHz), which is a separate role from
the mascot.

Brand moments carry the 77×20 R0N1N wordmark (`r0n1n_ui_logo`):

- boot splash (`desktop/helpers/r0n1n_boot.c`, ~3 s, any key skips): the
  wordmark settles out of glitching scanlines, a blade cuts under it, then
  the motto types out while a bar fills;
- the updater's progress screen (wordmark, "ОБНОВЛЕНИЕ", hatched bar,
  percentage, stage in Russian) and its failure screen;
- the post-update slideshow (`assets/slideshow/update_default`, 3 frames):
  "firmware installed", the d-pad map of Home, and the hold gestures.

## Profiles/modes — the core of the UX concept

The same build serves a mixed audience through a profile, not through a
forked firmware:

- **Everyday** — large elements, step-by-step wizards, "safe" tools (IR
  remote, U2F, TOTP, reading your own cards).
- **Pentest** — dense lists, the full RF/NFC/USB toolset, keybinds,
  Capture Timeline front and center.
- **Dev** — GPIO/UART/SPI/I2C/SWD, a logic analyzer, a console/CLI, the JS
  runner.
- **CTF** — notes, a timer, "cheat sheets," exporting findings to the
  companion.

A profile changes: the set and order of "desktops," interface density, the
content of Quick Actions/Control Center, and the level of hints. Switching
profiles takes 2 presses from the Control Center.

## Cross-cutting system services

- **Capture Timeline** — a single chronological feed of every artifact
  (.sub/.nfc/.rfid/.ir), tagged by type/frequency/time; replay, export, and
  send-to-companion from one place instead of scattered across each app's
  own folder.
- **Global Search** — search across apps, files, actions, and settings;
  the index lives on SD (see the RAM constraints in `HARDWARE.md`), not in
  memory.
- **Favorites/Recent** — quick access to frequently used tools.
- **Contextual actions** — a long OK-press on any object opens a single
  menu (emulate/save/rename/export/delete) — the same behavior everywhere,
  instead of each app reinventing it.
- **Progressive hints** — a hint line is visible in Everyday, hidden in
  Pentest/Dev.
- **Notifications via RGB/vibration/sound** — a configurable layer,
  including a "silent mode" (LED only).
- **Consistency** — one system keyboard, one file picker, one confirmation
  dialog for "sharp" operations, shared across the whole firmware.

## Where each piece comes from technically

Everything here is written for R0N1N on top of the official Flipper Zero
firmware (see `FIRMWARE_LANDSCAPE.md`), reusing its existing pieces where
they fit (Control Center is the stock lock menu today, JS modules come
from the official JS engine); ideas from other firmwares are
reimplemented, not copied. The Home dashboard, the
pseudo-swipe navigation model, profiles, Global Search, and Capture
Timeline are R0N1N's own contribution, implemented as new system services
on top of Furi (see `ARCHITECTURE.md`, "R0N1N layer" section).

## Readiness criterion for a UX decision

Every new screen or interaction is checked against the list in
`VISION.md` (path to the goal, adapts to profile, honest about the
capability's source) before it's greenlit for implementation.
