#include "desktop_view_simple.h"

#include <furi.h>
#include <gui/elements.h>
#include <assets_icons.h>
#include <u8g2.h>

typedef struct {
    const Icon* icon;
    const char* label;
} DesktopSimpleMenuItem;

typedef struct {
    DesktopSimpleMenuItem items[DESKTOP_SIMPLE_MENU_MAX_ITEMS];
    uint32_t count;
    uint32_t selected;
} DesktopSimpleMenuModel;

struct DesktopSimpleMenu {
    View* view;
    DesktopSimpleMenuCallback callback;
    void* context;
};

static void desktop_simple_menu_draw(Canvas* canvas, void* model) {
    DesktopSimpleMenuModel* m = model;
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    if(!m->count) return;
    const DesktopSimpleMenuItem* item = &m->items[m->selected];

    // Arrows hint that there is more to the left and right
    canvas_draw_icon(canvas, 4, 12, &I_ButtonLeft_4x7);
    canvas_draw_icon(canvas, 120, 12, &I_ButtonRight_4x7);

    canvas_draw_icon(canvas, 48, 0, item->icon);

    canvas_set_custom_u8g2_font(canvas, u8g2_font_10x20_t_cyrillic);
    canvas_draw_str_aligned(canvas, 64, 50, AlignCenter, AlignBottom, item->label);

    // Page dots
    const int32_t dots_w = (int32_t)m->count * 6 - 2;
    int32_t x = 64 - dots_w / 2;
    for(uint32_t i = 0; i < m->count; i++, x += 6) {
        if(i == m->selected) {
            canvas_draw_box(canvas, x, 58, 4, 4);
        } else {
            canvas_draw_frame(canvas, x, 58, 4, 4);
        }
    }
    canvas_set_font(canvas, FontSecondary);
}

static bool desktop_simple_menu_input(InputEvent* event, void* context) {
    DesktopSimpleMenu* menu = context;
    if(event->type != InputTypeShort && event->type != InputTypeRepeat) return false;

    bool consumed = true;
    uint32_t selected = 0;
    bool open = false;
    with_view_model(
        menu->view,
        DesktopSimpleMenuModel * m,
        {
            if(!m->count) {
                consumed = false;
            } else if(event->key == InputKeyRight || event->key == InputKeyDown) {
                m->selected = (m->selected + 1) % m->count;
            } else if(event->key == InputKeyLeft || event->key == InputKeyUp) {
                m->selected = (m->selected + m->count - 1) % m->count;
            } else if(event->key == InputKeyOk && event->type == InputTypeShort) {
                open = true;
            } else {
                consumed = false; // Back: the scene manager goes back
            }
            selected = m->selected;
        },
        consumed);

    if(open && menu->callback) menu->callback(selected, menu->context);
    return consumed;
}

DesktopSimpleMenu* desktop_simple_menu_alloc(void) {
    DesktopSimpleMenu* menu = malloc(sizeof(DesktopSimpleMenu));
    menu->view = view_alloc();
    view_allocate_model(menu->view, ViewModelTypeLocking, sizeof(DesktopSimpleMenuModel));
    view_set_context(menu->view, menu);
    view_set_draw_callback(menu->view, desktop_simple_menu_draw);
    view_set_input_callback(menu->view, desktop_simple_menu_input);
    return menu;
}

void desktop_simple_menu_free(DesktopSimpleMenu* menu) {
    furi_assert(menu);
    view_free(menu->view);
    free(menu);
}

View* desktop_simple_menu_get_view(DesktopSimpleMenu* menu) {
    furi_assert(menu);
    return menu->view;
}

void desktop_simple_menu_reset(DesktopSimpleMenu* menu) {
    with_view_model(
        menu->view,
        DesktopSimpleMenuModel * m,
        {
            m->count = 0;
            m->selected = 0;
        },
        true);
}

void desktop_simple_menu_add_item(DesktopSimpleMenu* menu, const Icon* icon, const char* label) {
    with_view_model(
        menu->view,
        DesktopSimpleMenuModel * m,
        {
            furi_check(m->count < DESKTOP_SIMPLE_MENU_MAX_ITEMS);
            m->items[m->count].icon = icon;
            m->items[m->count].label = label;
            m->count++;
        },
        true);
}

void desktop_simple_menu_set_selected(DesktopSimpleMenu* menu, uint32_t index) {
    with_view_model(
        menu->view,
        DesktopSimpleMenuModel * m,
        { m->selected = index < m->count ? index : 0; },
        true);
}

uint32_t desktop_simple_menu_get_selected(DesktopSimpleMenu* menu) {
    uint32_t selected = 0;
    with_view_model(menu->view, DesktopSimpleMenuModel * m, { selected = m->selected; }, false);
    return selected;
}

void desktop_simple_menu_set_callback(
    DesktopSimpleMenu* menu,
    DesktopSimpleMenuCallback callback,
    void* context) {
    menu->callback = callback;
    menu->context = context;
}
