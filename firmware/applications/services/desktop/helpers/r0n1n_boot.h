/**
 * @file r0n1n_boot.h
 * R0N1N boot splash: the wordmark assembles out of glitching scanlines, a
 * blade cuts under it, then the motto types out while a bar fills.
 */
#pragma once

#include <gui/gui.h>

#define R0N1N_BOOT_DURATION_MS 2900

/** Draw the splash as it looks `t` ms after it started. */
void r0n1n_boot_draw(Canvas* canvas, uint32_t t);

/** Play the splash over everything on `gui`; any key skips it. Blocks until done. */
void r0n1n_boot_run(Gui* gui);
