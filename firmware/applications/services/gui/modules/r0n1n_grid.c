#include "r0n1n_grid.h"

#include <gui/r0n1n_ui.h>
#include <assets_icons.h>
#include <furi.h>
#include <m-array.h>

#define R0N1N_GRID_GAP      2
#define R0N1N_GRID_SLIDER_Y 47

typedef struct {
    const Icon* icon;
    FuriString* caption;
    bool marked;
    uint32_t index;
} R0n1nGridItem;

static void R0n1nGridItem_init(R0n1nGridItem* item) {
    item->icon = NULL;
    item->caption = furi_string_alloc();
    item->marked = false;
    item->index = 0;
}

static void R0n1nGridItem_init_set(R0n1nGridItem* item, const R0n1nGridItem* src) {
    item->icon = src->icon;
    item->caption = furi_string_alloc_set(src->caption);
    item->marked = src->marked;
    item->index = src->index;
}

static void R0n1nGridItem_set(R0n1nGridItem* item, const R0n1nGridItem* src) {
    item->icon = src->icon;
    furi_string_set(item->caption, src->caption);
    item->marked = src->marked;
    item->index = src->index;
}

static void R0n1nGridItem_clear(R0n1nGridItem* item) {
    furi_string_free(item->caption);
}

ARRAY_DEF(
    R0n1nGridItemArray,
    R0n1nGridItem,
    (INIT(API_2(R0n1nGridItem_init)),
     SET(API_6(R0n1nGridItem_set)),
     INIT_SET(API_6(R0n1nGridItem_init_set)),
     CLEAR(API_2(R0n1nGridItem_clear))))

typedef struct {
    R0n1nGridItemArray_t items;
    FuriString* title;
    FuriString* empty_text;
    const Icon* title_icon;
    uint8_t columns;
    uint8_t tile_width;
    uint8_t tile_height;
    uint8_t y;
    bool caption_in_header;
    int8_t battery_pct;
    bool has_slider;
    const Icon* slider_icon;
    FuriString* slider_caption;
    uint8_t slider_value;
    size_t position; // == item count while the slider is focused
} R0n1nGridModel;

struct R0n1nGrid {
    View* view;
    R0n1nGridCallback ok_callback;
    R0n1nGridCallback hold_callback;
    R0n1nGridSliderCallback slider_callback;
    void* context;
};

static void r0n1n_grid_draw_callback(Canvas* canvas, void* _model) {
    R0n1nGridModel* model = _model;
    const size_t count = R0n1nGridItemArray_size(model->items);
    const bool on_slider = model->has_slider && model->position >= count;

    canvas_clear(canvas);

    const char* caption = NULL;
    if(on_slider) {
        caption = furi_string_get_cstr(model->slider_caption);
    } else if(count) {
        caption =
            furi_string_get_cstr(R0n1nGridItemArray_cget(model->items, model->position)->caption);
    }

    if(model->caption_in_header) {
        r0n1n_ui_header(canvas, model->title_icon, caption, NULL, model->battery_pct);
    } else {
        char counter[24] = {0};
        if(count)
            snprintf(
                counter,
                sizeof(counter),
                "%u/%u",
                (unsigned)(model->position + 1),
                (unsigned)count);
        r0n1n_ui_header(
            canvas,
            model->title_icon,
            furi_string_get_cstr(model->title),
            count ? counter : NULL,
            -1);
    }

    if(!count && !model->has_slider && furi_string_size(model->empty_text)) {
        canvas_set_font(canvas, FontSecondary);
        r0n1n_ui_multiline_centered(canvas, 33, furi_string_get_cstr(model->empty_text));
    }

    // Rows scroll: as many as fit above the caption (or slider), the one
    // with the selection kept in view
    const uint8_t cols = MAX(model->columns, 1);
    const int32_t x0 = (128 - (cols * model->tile_width + (cols - 1) * R0N1N_GRID_GAP)) / 2;
    const int32_t bottom = model->has_slider        ? R0N1N_GRID_SLIDER_Y :
                           model->caption_in_header ? 64 :
                                                      R0N1N_UI_CAPTION_SEP_Y;
    const int32_t row_h = model->tile_height + R0N1N_GRID_GAP;
    const size_t visible_rows = MAX((bottom - model->y + R0N1N_GRID_GAP) / row_h, 1);
    const size_t rows = (count + cols - 1) / cols;
    const size_t selected_row = on_slider ? 0 : model->position / cols;
    const size_t first_row = selected_row >= visible_rows ? selected_row - visible_rows + 1 : 0;
    for(size_t i = first_row * cols; i < count && i < (first_row + visible_rows) * cols; i++) {
        const R0n1nGridItem* item = R0n1nGridItemArray_cget(model->items, i);
        r0n1n_ui_tile(
            canvas,
            x0 + (i % cols) * (model->tile_width + R0N1N_GRID_GAP),
            model->y + (int32_t)(i / cols - first_row) * row_h,
            model->tile_width,
            model->tile_height,
            item->icon,
            i == model->position,
            item->marked);
    }
    if(rows > visible_rows) {
        r0n1n_ui_scrollbar(
            canvas, model->y, visible_rows * row_h - R0N1N_GRID_GAP, selected_row, rows);
    }

    if(model->has_slider) {
        const int32_t y = R0N1N_GRID_SLIDER_Y;
        if(on_slider) {
            canvas_draw_rbox(canvas, 0, y, 128, 13, 2);
            canvas_set_color(canvas, ColorWhite);
        }
        if(model->slider_icon) canvas_draw_icon(canvas, 2, y + 2, model->slider_icon);
        canvas_draw_rframe(canvas, 14, y + 3, 86, 7, 1);
        canvas_draw_box(canvas, 16, y + 5, (82 * model->slider_value) / 100, 3);
        char value[6];
        snprintf(value, sizeof(value), "%u%%", model->slider_value);
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 126, y + 10, AlignRight, AlignBottom, value);
        canvas_set_color(canvas, ColorBlack);
    } else if(!model->caption_in_header) {
        r0n1n_ui_caption(canvas, R0N1N_UI_CAPTION_SEP_Y, caption);
    }
}

static bool r0n1n_grid_input_callback(InputEvent* event, void* context) {
    R0n1nGrid* grid = context;
    if(event->type != InputTypeShort && event->type != InputTypeRepeat &&
       event->type != InputTypeLong) {
        return event->key != InputKeyBack;
    }

    bool consumed = true;
    R0n1nGridCallback item_callback = NULL;
    uint32_t item_index = 0;
    bool slider_changed = false;
    uint8_t slider_value = 0;

    with_view_model(
        grid->view,
        R0n1nGridModel * model,
        {
            const size_t count = R0n1nGridItemArray_size(model->items);
            const uint8_t cols = MAX(model->columns, 1);
            const bool on_slider = model->has_slider && model->position >= count;
            const bool is_long = event->type == InputTypeLong;

            if(!count && !model->has_slider) {
                consumed = event->key != InputKeyBack;
            } else if(event->key == InputKeyLeft || event->key == InputKeyRight) {
                const int32_t delta = event->key == InputKeyLeft ? -1 : 1;
                if(on_slider) {
                    const int32_t value = model->slider_value + delta * 10;
                    model->slider_value = CLAMP(value, 100, 0);
                    slider_changed = true;
                    slider_value = model->slider_value;
                } else if(!is_long) {
                    model->position = (model->position + count + delta) % count;
                }
            } else if(event->key == InputKeyUp && !is_long) {
                if(on_slider) {
                    model->position = count - 1;
                } else if(model->position >= cols) {
                    model->position -= cols;
                }
            } else if(event->key == InputKeyDown && !is_long) {
                if(!on_slider && model->position + cols < count) {
                    model->position += cols;
                } else if(!on_slider && model->has_slider) {
                    model->position = count;
                }
            } else if(event->key == InputKeyOk && !on_slider && count) {
                item_callback = event->type == InputTypeShort ? grid->ok_callback :
                                is_long                       ? grid->hold_callback :
                                                                NULL;
                item_index = R0n1nGridItemArray_cget(model->items, model->position)->index;
            } else {
                consumed = event->key != InputKeyBack;
            }
        },
        true);

    if(slider_changed && grid->slider_callback) {
        grid->slider_callback(grid->context, slider_value);
    }
    if(item_callback) item_callback(grid->context, item_index);
    return consumed;
}

R0n1nGrid* r0n1n_grid_alloc(void) {
    R0n1nGrid* grid = malloc(sizeof(R0n1nGrid));
    grid->view = view_alloc();
    view_set_context(grid->view, grid);
    view_allocate_model(grid->view, ViewModelTypeLocking, sizeof(R0n1nGridModel));
    view_set_draw_callback(grid->view, r0n1n_grid_draw_callback);
    view_set_input_callback(grid->view, r0n1n_grid_input_callback);

    with_view_model(
        grid->view,
        R0n1nGridModel * model,
        {
            R0n1nGridItemArray_init(model->items);
            model->title = furi_string_alloc();
            model->empty_text = furi_string_alloc();
            model->slider_caption = furi_string_alloc();
            model->title_icon = NULL;
            model->columns = 3;
            model->tile_width = 41;
            model->tile_height = 19;
            model->y = 12;
            model->caption_in_header = false;
            model->battery_pct = -1;
            model->has_slider = false;
            model->slider_icon = NULL;
            model->slider_value = 0;
            model->position = 0;
        },
        true);

    return grid;
}

void r0n1n_grid_free(R0n1nGrid* grid) {
    furi_check(grid);
    with_view_model(
        grid->view,
        R0n1nGridModel * model,
        {
            R0n1nGridItemArray_clear(model->items);
            furi_string_free(model->title);
            furi_string_free(model->empty_text);
            furi_string_free(model->slider_caption);
        },
        false);
    view_free(grid->view);
    free(grid);
}

View* r0n1n_grid_get_view(R0n1nGrid* grid) {
    furi_check(grid);
    return grid->view;
}

void r0n1n_grid_reset(R0n1nGrid* grid) {
    furi_check(grid);
    with_view_model(
        grid->view,
        R0n1nGridModel * model,
        {
            R0n1nGridItemArray_reset(model->items);
            furi_string_reset(model->title);
            furi_string_reset(model->empty_text);
            furi_string_reset(model->slider_caption);
            model->title_icon = NULL;
            model->caption_in_header = false;
            model->battery_pct = -1;
            model->has_slider = false;
            model->position = 0;
        },
        true);
}

void r0n1n_grid_set_title(R0n1nGrid* grid, const Icon* icon, const char* title) {
    furi_check(grid);
    with_view_model(
        grid->view,
        R0n1nGridModel * model,
        {
            model->title_icon = icon;
            furi_string_set(model->title, title ? title : "");
        },
        true);
}

void r0n1n_grid_set_layout(
    R0n1nGrid* grid,
    uint8_t columns,
    uint8_t tile_width,
    uint8_t tile_height,
    uint8_t y) {
    furi_check(grid);
    with_view_model(
        grid->view,
        R0n1nGridModel * model,
        {
            model->columns = columns;
            model->tile_width = tile_width;
            model->tile_height = tile_height;
            model->y = y;
        },
        true);
}

void r0n1n_grid_set_caption_in_header(R0n1nGrid* grid, bool enabled, int8_t battery_pct) {
    furi_check(grid);
    with_view_model(
        grid->view,
        R0n1nGridModel * model,
        {
            model->caption_in_header = enabled;
            model->battery_pct = battery_pct;
        },
        true);
}

void r0n1n_grid_add_item(
    R0n1nGrid* grid,
    const Icon* icon,
    const char* caption,
    bool marked,
    uint32_t index) {
    furi_check(grid);
    with_view_model(
        grid->view,
        R0n1nGridModel * model,
        {
            R0n1nGridItem* item = R0n1nGridItemArray_push_new(model->items);
            item->icon = icon;
            furi_string_set(item->caption, caption ? caption : "");
            item->marked = marked;
            item->index = index;
        },
        true);
}

void r0n1n_grid_update_item(R0n1nGrid* grid, uint32_t index, const char* caption, bool marked) {
    furi_check(grid);
    with_view_model(
        grid->view,
        R0n1nGridModel * model,
        {
            for(size_t i = 0; i < R0n1nGridItemArray_size(model->items); i++) {
                R0n1nGridItem* item = R0n1nGridItemArray_get(model->items, i);
                if(item->index == index) {
                    if(caption) furi_string_set(item->caption, caption);
                    item->marked = marked;
                    break;
                }
            }
        },
        true);
}

void r0n1n_grid_set_slider(R0n1nGrid* grid, const Icon* icon, const char* caption, uint8_t value) {
    furi_check(grid);
    with_view_model(
        grid->view,
        R0n1nGridModel * model,
        {
            model->has_slider = true;
            model->slider_icon = icon;
            furi_string_set(model->slider_caption, caption ? caption : "");
            model->slider_value = MIN(value, 100);
        },
        true);
}

void r0n1n_grid_set_callbacks(
    R0n1nGrid* grid,
    R0n1nGridCallback ok,
    R0n1nGridCallback hold_ok,
    R0n1nGridSliderCallback slider,
    void* context) {
    furi_check(grid);
    grid->ok_callback = ok;
    grid->hold_callback = hold_ok;
    grid->slider_callback = slider;
    grid->context = context;
}

void r0n1n_grid_set_selected_item(R0n1nGrid* grid, uint32_t index) {
    furi_check(grid);
    with_view_model(
        grid->view,
        R0n1nGridModel * model,
        {
            for(size_t i = 0; i < R0n1nGridItemArray_size(model->items); i++) {
                if(R0n1nGridItemArray_cget(model->items, i)->index == index) {
                    model->position = i;
                    break;
                }
            }
        },
        true);
}

void r0n1n_grid_set_empty_text(R0n1nGrid* grid, const char* text) {
    furi_check(grid);
    with_view_model(
        grid->view, R0n1nGridModel * model, { furi_string_set(model->empty_text, text); }, true);
}
