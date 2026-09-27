/**
 * @file r0n1n_grid.h
 * R0N1N tile grid: icon-only tiles, the selected tile's caption shown in full
 * (at the bottom, or in the header for Control Center), an optional "on"
 * mark per tile, and an optional slider row below the tiles.
 */
#pragma once

#include <gui/view.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct R0n1nGrid R0n1nGrid;

typedef void (*R0n1nGridCallback)(void* context, uint32_t index);
typedef void (*R0n1nGridSliderCallback)(void* context, uint8_t value);

R0n1nGrid* r0n1n_grid_alloc(void);
void r0n1n_grid_free(R0n1nGrid* grid);
View* r0n1n_grid_get_view(R0n1nGrid* grid);

/** Remove items and the slider; keeps callbacks. */
void r0n1n_grid_reset(R0n1nGrid* grid);

void r0n1n_grid_set_title(R0n1nGrid* grid, const Icon* icon, const char* title);

/** Tiles are centered horizontally with 2 px gaps, starting at `y`. */
void r0n1n_grid_set_layout(
    R0n1nGrid* grid,
    uint8_t columns,
    uint8_t tile_width,
    uint8_t tile_height,
    uint8_t y);

/** Show the selected caption in the header (with a battery gauge) instead of at the bottom. */
void r0n1n_grid_set_caption_in_header(R0n1nGrid* grid, bool enabled, int8_t battery_pct);

void r0n1n_grid_add_item(
    R0n1nGrid* grid,
    const Icon* icon,
    const char* caption,
    bool marked,
    uint32_t index);

/** Update the caption and mark of the item with the given `index`. */
void r0n1n_grid_update_item(R0n1nGrid* grid, uint32_t index, const char* caption, bool marked);

/** Slider row under the tiles (Down from the last row); value is 0..100. */
void r0n1n_grid_set_slider(R0n1nGrid* grid, const Icon* icon, const char* caption, uint8_t value);

void r0n1n_grid_set_callbacks(
    R0n1nGrid* grid,
    R0n1nGridCallback ok,
    R0n1nGridCallback hold_ok,
    R0n1nGridSliderCallback slider,
    void* context);

void r0n1n_grid_set_selected_item(R0n1nGrid* grid, uint32_t index);

/** Text shown centered while the grid has no items ('\\n' splits lines). */
void r0n1n_grid_set_empty_text(R0n1nGrid* grid, const char* text);

#ifdef __cplusplus
}
#endif
