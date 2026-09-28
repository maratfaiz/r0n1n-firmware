#include "r0n1n_list.h"

#include <gui/r0n1n_ui.h>
#include <assets_icons.h>
#include <furi.h>
#include <m-array.h>

typedef struct {
    const Icon* icon;
    FuriString* label;
    FuriString* right;
    const Icon* right_icon;
    FuriString* caption;
    uint32_t index;
} R0n1nListItem;

static void R0n1nListItem_init(R0n1nListItem* item) {
    item->icon = NULL;
    item->label = furi_string_alloc();
    item->right = furi_string_alloc();
    item->right_icon = NULL;
    item->caption = furi_string_alloc();
    item->index = 0;
}

static void R0n1nListItem_init_set(R0n1nListItem* item, const R0n1nListItem* src) {
    item->icon = src->icon;
    item->label = furi_string_alloc_set(src->label);
    item->right = furi_string_alloc_set(src->right);
    item->right_icon = src->right_icon;
    item->caption = furi_string_alloc_set(src->caption);
    item->index = src->index;
}

static void R0n1nListItem_set(R0n1nListItem* item, const R0n1nListItem* src) {
    item->icon = src->icon;
    furi_string_set(item->label, src->label);
    furi_string_set(item->right, src->right);
    item->right_icon = src->right_icon;
    furi_string_set(item->caption, src->caption);
    item->index = src->index;
}

static void R0n1nListItem_clear(R0n1nListItem* item) {
    furi_string_free(item->label);
    furi_string_free(item->right);
    furi_string_free(item->caption);
}

ARRAY_DEF(
    R0n1nListItemArray,
    R0n1nListItem,
    (INIT(API_2(R0n1nListItem_init)),
     SET(API_6(R0n1nListItem_set)),
     INIT_SET(API_6(R0n1nListItem_init_set)),
     CLEAR(API_2(R0n1nListItem_clear))))

typedef struct {
    R0n1nListItemArray_t items;
    FuriString* title;
    const Icon* title_icon;
    FuriString* query;
    bool has_query;
    FuriString* empty_text;
    const char* const* tabs;
    uint8_t tab_count;
    uint8_t tab;
    bool has_captions;
    size_t position;
    size_t window;
} R0n1nListModel;

struct R0n1nList {
    View* view;
    R0n1nListCallback ok_callback;
    R0n1nListCallback hold_callback;
    R0n1nListCallback tab_callback;
    void* context;
};

typedef struct {
    int32_t top;
    uint8_t row_height;
    uint8_t visible;
} R0n1nListLayout;

static R0n1nListLayout r0n1n_list_layout(const R0n1nListModel* model) {
    R0n1nListLayout layout = {.top = 11, .row_height = R0N1N_UI_ROW_HEIGHT};
    int32_t bottom = 64;
    if(model->has_query) {
        layout.top = 27;
        layout.row_height = 12;
    } else if(model->tab_count) {
        layout.top = 25;
    } else if(model->has_captions) {
        layout.row_height = 11;
        bottom = 55;
    }
    layout.visible = (bottom - layout.top) / layout.row_height;
    return layout;
}

static void r0n1n_list_draw_callback(Canvas* canvas, void* _model) {
    R0n1nListModel* model = _model;
    const size_t count = R0n1nListItemArray_size(model->items);
    const R0n1nListLayout layout = r0n1n_list_layout(model);

    canvas_clear(canvas);

    char counter[24] = {0};
    if(count)
        snprintf(
            counter, sizeof(counter), "%u/%u", (unsigned)(model->position + 1), (unsigned)count);
    r0n1n_ui_header(
        canvas, model->title_icon, furi_string_get_cstr(model->title), count ? counter : NULL, -1);

    canvas_set_font(canvas, FontSecondary);
    if(model->has_query) {
        canvas_draw_rframe(canvas, 0, 12, 128, 13, 2);
        canvas_draw_icon(canvas, 3, 15, &I_R_Search_8x8);
        canvas_draw_str(canvas, 15, 22, furi_string_get_cstr(model->query));
    } else if(model->tab_count) {
        // Scroll the tab strip so the selected tab is always fully visible.
        int32_t x = 0;
        int32_t selected_end = 0;
        for(uint8_t i = 0; i <= model->tab; i++) {
            selected_end += canvas_string_width(canvas, model->tabs[i]) + 8 + (i ? 2 : 0);
        }
        if(selected_end > 128) x = 128 - selected_end;
        for(uint8_t i = 0; i < model->tab_count; i++) {
            const int32_t w = canvas_string_width(canvas, model->tabs[i]) + 8;
            if(i == model->tab) {
                canvas_draw_rbox(canvas, x, 12, w, 10, 2);
                canvas_set_color(canvas, ColorWhite);
            }
            canvas_draw_str(canvas, x + 4, 20, model->tabs[i]);
            canvas_set_color(canvas, ColorBlack);
            x += w + 2;
        }
        canvas_draw_line(canvas, 0, 23, 127, 23);
    }

    if(!count) {
        if(furi_string_size(model->empty_text)) {
            r0n1n_ui_multiline_centered(
                canvas,
                layout.top + (64 - layout.top) / 2,
                furi_string_get_cstr(model->empty_text));
        }
        return;
    }

    const bool scroll = count > layout.visible;
    const uint8_t width = scroll ? 122 : 128;
    for(size_t i = 0; i < layout.visible && model->window + i < count; i++) {
        const size_t pos = model->window + i;
        const R0n1nListItem* item = R0n1nListItemArray_cget(model->items, pos);
        r0n1n_ui_list_row(
            canvas,
            layout.top + i * layout.row_height,
            layout.row_height,
            width,
            item->icon,
            furi_string_get_cstr(item->label),
            furi_string_size(item->right) ? furi_string_get_cstr(item->right) : NULL,
            item->right_icon,
            pos == model->position);
    }
    if(scroll) {
        r0n1n_ui_scrollbar(
            canvas, layout.top, layout.visible * layout.row_height - 1, model->position, count);
    }
    if(model->has_captions) {
        const R0n1nListItem* item = R0n1nListItemArray_cget(model->items, model->position);
        r0n1n_ui_caption(canvas, 55, furi_string_get_cstr(item->caption));
    }
}

static void r0n1n_list_move(R0n1nList* list, int32_t delta) {
    with_view_model(
        list->view,
        R0n1nListModel * model,
        {
            const size_t count = R0n1nListItemArray_size(model->items);
            if(count) {
                const R0n1nListLayout layout = r0n1n_list_layout(model);
                model->position = (model->position + count + delta) % count;
                if(model->position < model->window) {
                    model->window = model->position;
                } else if(model->position >= model->window + layout.visible) {
                    model->window = model->position - layout.visible + 1;
                }
            }
        },
        true);
}

static bool r0n1n_list_input_callback(InputEvent* event, void* context) {
    R0n1nList* list = context;
    bool consumed = false;

    if(event->key == InputKeyUp || event->key == InputKeyDown) {
        if(event->type == InputTypeShort || event->type == InputTypeRepeat) {
            r0n1n_list_move(list, event->key == InputKeyUp ? -1 : 1);
            consumed = true;
        }
    } else if(event->key == InputKeyOk) {
        consumed = true;
        R0n1nListCallback callback = event->type == InputTypeShort ? list->ok_callback :
                                     event->type == InputTypeLong  ? list->hold_callback :
                                                                     NULL;
        const uint32_t index = r0n1n_list_get_selected_item(list);
        if(callback && index != UINT32_MAX) callback(list->context, index);
    } else if(event->key == InputKeyLeft || event->key == InputKeyRight) {
        if(event->type == InputTypeShort) {
            uint8_t tab = 0;
            bool changed = false;
            with_view_model(
                list->view,
                R0n1nListModel * model,
                {
                    if(model->tab_count) {
                        const int8_t delta = event->key == InputKeyLeft ? -1 : 1;
                        model->tab = (model->tab + model->tab_count + delta) % model->tab_count;
                        tab = model->tab;
                        changed = true;
                    }
                },
                true);
            if(changed && list->tab_callback) list->tab_callback(list->context, tab);
            consumed = changed;
        }
    }

    return consumed;
}

R0n1nList* r0n1n_list_alloc(void) {
    R0n1nList* list = malloc(sizeof(R0n1nList));
    list->view = view_alloc();
    view_set_context(list->view, list);
    view_allocate_model(list->view, ViewModelTypeLocking, sizeof(R0n1nListModel));
    view_set_draw_callback(list->view, r0n1n_list_draw_callback);
    view_set_input_callback(list->view, r0n1n_list_input_callback);

    with_view_model(
        list->view,
        R0n1nListModel * model,
        {
            R0n1nListItemArray_init(model->items);
            model->title = furi_string_alloc();
            model->query = furi_string_alloc();
            model->empty_text = furi_string_alloc();
            model->title_icon = NULL;
            model->has_query = false;
            model->tabs = NULL;
            model->tab_count = 0;
            model->tab = 0;
            model->has_captions = false;
            model->position = 0;
            model->window = 0;
        },
        true);

    return list;
}

void r0n1n_list_free(R0n1nList* list) {
    furi_check(list);
    with_view_model(
        list->view,
        R0n1nListModel * model,
        {
            R0n1nListItemArray_clear(model->items);
            furi_string_free(model->title);
            furi_string_free(model->query);
            furi_string_free(model->empty_text);
        },
        false);
    view_free(list->view);
    free(list);
}

View* r0n1n_list_get_view(R0n1nList* list) {
    furi_check(list);
    return list->view;
}

void r0n1n_list_reset(R0n1nList* list) {
    furi_check(list);
    with_view_model(
        list->view,
        R0n1nListModel * model,
        {
            R0n1nListItemArray_reset(model->items);
            furi_string_reset(model->title);
            furi_string_reset(model->query);
            furi_string_reset(model->empty_text);
            model->title_icon = NULL;
            model->has_query = false;
            model->tabs = NULL;
            model->tab_count = 0;
            model->tab = 0;
            model->has_captions = false;
            model->position = 0;
            model->window = 0;
        },
        true);
}

void r0n1n_list_set_title(R0n1nList* list, const Icon* icon, const char* title) {
    furi_check(list);
    with_view_model(
        list->view,
        R0n1nListModel * model,
        {
            model->title_icon = icon;
            furi_string_set(model->title, title ? title : "");
        },
        true);
}

void r0n1n_list_add_item(
    R0n1nList* list,
    const Icon* icon,
    const char* label,
    const char* right,
    const Icon* right_icon,
    const char* caption,
    uint32_t index) {
    furi_check(list);
    furi_check(label);
    with_view_model(
        list->view,
        R0n1nListModel * model,
        {
            R0n1nListItem* item = R0n1nListItemArray_push_new(model->items);
            item->icon = icon;
            furi_string_set(item->label, label);
            if(right) furi_string_set(item->right, right);
            item->right_icon = right_icon;
            if(caption) {
                furi_string_set(item->caption, caption);
                model->has_captions = true;
            }
            item->index = index;
        },
        true);
}

void r0n1n_list_set_query(R0n1nList* list, const char* query) {
    furi_check(list);
    with_view_model(
        list->view,
        R0n1nListModel * model,
        {
            model->has_query = true;
            furi_string_set(model->query, query ? query : "");
        },
        true);
}

void r0n1n_list_set_tabs(R0n1nList* list, const char* const* tabs, uint8_t count, uint8_t selected) {
    furi_check(list);
    with_view_model(
        list->view,
        R0n1nListModel * model,
        {
            model->tabs = tabs;
            model->tab_count = count;
            model->tab = count ? MIN(selected, count - 1) : 0;
        },
        true);
}

void r0n1n_list_set_empty_text(R0n1nList* list, const char* text) {
    furi_check(list);
    with_view_model(
        list->view, R0n1nListModel * model, { furi_string_set(model->empty_text, text); }, true);
}

void r0n1n_list_set_callbacks(
    R0n1nList* list,
    R0n1nListCallback ok,
    R0n1nListCallback hold_ok,
    R0n1nListCallback tab_changed,
    void* context) {
    furi_check(list);
    list->ok_callback = ok;
    list->hold_callback = hold_ok;
    list->tab_callback = tab_changed;
    list->context = context;
}

void r0n1n_list_set_selected_item(R0n1nList* list, uint32_t index) {
    furi_check(list);
    with_view_model(
        list->view,
        R0n1nListModel * model,
        {
            const size_t count = R0n1nListItemArray_size(model->items);
            for(size_t i = 0; i < count; i++) {
                if(R0n1nListItemArray_cget(model->items, i)->index == index) {
                    const R0n1nListLayout layout = r0n1n_list_layout(model);
                    model->position = i;
                    model->window = i >= layout.visible ? i - layout.visible + 1 : 0;
                    break;
                }
            }
        },
        true);
}

uint32_t r0n1n_list_get_selected_item(R0n1nList* list) {
    furi_check(list);
    uint32_t index = UINT32_MAX;
    with_view_model(
        list->view,
        R0n1nListModel * model,
        {
            if(R0n1nListItemArray_size(model->items)) {
                index = R0n1nListItemArray_cget(model->items, model->position)->index;
            }
        },
        false);
    return index;
}
