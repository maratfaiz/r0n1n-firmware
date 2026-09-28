/**
 * @file r0n1n_ui.h
 * R0N1N interface building blocks (docs/UX_DESIGN.md, "v2" layout).
 *
 * One visual language for every R0N1N screen:
 * - an inverted 10 px header bar: title on the left, a counter or state on the right;
 * - one readable font (FontSecondary) for all text;
 * - selection is always a filled rounded shape with inverted content;
 * - 13 px list rows, icon-only tiles whose name is shown in a caption.
 */
#pragma once

#include <gui/canvas.h>
#include <gui/icon.h>
#include <furi.h>

#ifdef __cplusplus
extern "C" {
#endif

#define R0N1N_UI_HEADER_HEIGHT 10
#define R0N1N_UI_ROW_HEIGHT    13
#define R0N1N_UI_CAPTION_SEP_Y 54

/** Inverted header bar. `icon`, `right` may be NULL; battery_pct < 0 hides the battery. */
void r0n1n_ui_header(
    Canvas* canvas,
    const Icon* icon,
    const char* title,
    const char* right,
    int8_t battery_pct);

/** Battery gauge with a percentage, right-aligned at `x_right`, in the current color. */
void r0n1n_ui_battery(Canvas* canvas, int32_t x_right, int32_t y, uint8_t pct);

/** One list row. `icon`, `right`, `right_icon` may be NULL. Label is shortened to fit. */
void r0n1n_ui_list_row(
    Canvas* canvas,
    int32_t y,
    uint8_t height,
    uint8_t width,
    const Icon* icon,
    const char* label,
    const char* right,
    const Icon* right_icon,
    bool selected);

/** Dotted track with a solid thumb along the right edge. */
void r0n1n_ui_scrollbar(Canvas* canvas, int32_t y, uint8_t height, size_t pos, size_t total);

/** Icon-only tile. `marked` draws the small "on" badge in the top-right corner. */
void r0n1n_ui_tile(
    Canvas* canvas,
    int32_t x,
    int32_t y,
    uint8_t width,
    uint8_t height,
    const Icon* icon,
    bool selected,
    bool marked);

/** Separator line at `sep_y` and centered text in the strip below it. */
void r0n1n_ui_caption(Canvas* canvas, int32_t sep_y, const char* text);

/** Shorten `text` with "..." to fit `width` px in the current font, UTF-8 safe. */
void r0n1n_ui_fit_width(Canvas* canvas, FuriString* text, size_t width);

/** Centered lines (split on '\n') around `y_center`, FontSecondary. */
void r0n1n_ui_multiline_centered(Canvas* canvas, int32_t y_center, const char* text);

/** Draw `icon` centered in the given box. */
void r0n1n_ui_icon_centered(
    Canvas* canvas,
    int32_t x,
    int32_t y,
    uint8_t width,
    uint8_t height,
    const Icon* icon);

#define R0N1N_UI_LOGO_WIDTH  77
#define R0N1N_UI_LOGO_HEIGHT 20

/** The R0N1N wordmark. `row_shift` (may be NULL) moves each of its 20 rows sideways, for glitch effects. */
void r0n1n_ui_logo(Canvas* canvas, int32_t x, int32_t y, const int8_t* row_shift);

/** Text with `spacing` extra px between characters, centered on `x` at baseline `y`.
 * Only the first `chars` characters are drawn; the position is that of the whole text. */
void r0n1n_ui_spaced_text(
    Canvas* canvas,
    int32_t x,
    int32_t y,
    const char* text,
    uint8_t spacing,
    size_t chars);

/** Rounded frame with a hatched fill, `percent` of 100. */
void r0n1n_ui_progress(
    Canvas* canvas,
    int32_t x,
    int32_t y,
    uint8_t width,
    uint8_t height,
    uint8_t percent);

#ifdef __cplusplus
}
#endif
