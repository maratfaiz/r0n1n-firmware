#include <gui/gui_i.h>
#include <gui/view.h>
#include <gui/elements.h>
#include <gui/canvas.h>
#include <furi.h>
#include <input/input.h>
#include <dolphin/dolphin.h>
#include <locale/locale.h>
#include <assets_icons.h>
#include <gui/r0n1n_ui.h>
#include <u8g2.h>

#include "desktop_view_main.h"

// R0N1N Home dashboard (docs/UX_DESIGN.md): the whole screen, opaque over
// the idle dolphin animation (gui draws no status bar over the desktop):
// battery, a big clock, the date, the active profile and a landscape picture.
#define DASHBOARD_CLOCK_Y   31
#define DASHBOARD_DATE_Y    43
#define DASHBOARD_PROFILE_Y 57

typedef struct {
    DateTime datetime;
    char profile_name[32];
    uint8_t battery_pct;
    const Icon* wallpaper; // Home picture, NULL for none
    bool has_datetime;
    bool simple_mode;
} DesktopMainViewModel;

struct DesktopMainView {
    View* view;
    // R0N1N Home dashboard: a *separate* View stacked above the dolphin
    // animation in desktop.c (main_view_stack) so it draws on top. It's kept
    // separate from `view` rather than reordering `view` in the stack because
    // ViewStack's input dispatch walks the same array in reverse and stops at
    // the first consumer (view_stack.c): `view`'s input callback always
    // returns true, so moving it later would swallow input before the dolphin
    // view ever saw it. The dashboard's own input callback only takes
    // Right-short (sections) and lets everything else through.
    View* dashboard_view;
    DesktopMainViewCallback callback;
    void* context;
    FuriTimer* poweroff_timer;
    bool dummy_mode;
    bool simple_mode;
    bool back_held; // Back held past InputTypeLong: open Search on release
};

#define DESKTOP_MAIN_VIEW_POWEROFF_TIMEOUT 5000

static void desktop_main_poweroff_timer_callback(void* context) {
    DesktopMainView* main_view = context;
    main_view->back_held = false; // the power menu wins over Search
    main_view->callback(DesktopMainEventOpenPowerOff, main_view->context);
}

void desktop_main_set_callback(
    DesktopMainView* main_view,
    DesktopMainViewCallback callback,
    void* context) {
    furi_assert(main_view);
    furi_assert(callback);
    main_view->callback = callback;
    main_view->context = context;
}

View* desktop_main_get_view(DesktopMainView* main_view) {
    furi_assert(main_view);
    return main_view->view;
}

View* desktop_main_get_dashboard_view(DesktopMainView* main_view) {
    furi_assert(main_view);
    return main_view->dashboard_view;
}

void desktop_main_set_dummy_mode_state(DesktopMainView* main_view, bool dummy_mode) {
    furi_assert(main_view);
    main_view->dummy_mode = dummy_mode;
}

void desktop_main_set_simple_mode(DesktopMainView* main_view, bool simple_mode) {
    furi_assert(main_view);
    main_view->simple_mode = simple_mode;
    with_view_model(
        main_view->dashboard_view,
        DesktopMainViewModel * model,
        { model->simple_mode = simple_mode; },
        true);
}

void desktop_main_update_dashboard(
    DesktopMainView* main_view,
    const DateTime* datetime,
    const char* profile_name,
    uint8_t battery_pct,
    const Icon* wallpaper) {
    furi_assert(main_view);
    furi_assert(datetime);
    furi_assert(profile_name);
    with_view_model(
        main_view->dashboard_view,
        DesktopMainViewModel * model,
        {
            model->datetime = *datetime;
            model->battery_pct = battery_pct;
            model->wallpaper = wallpaper;
            model->has_datetime = true;
            strlcpy(model->profile_name, profile_name, sizeof(model->profile_name));
        },
        true);
}

// Simple mode Home: only what matters, as large as the screen allows
static void desktop_main_draw_simple(Canvas* canvas, DesktopMainViewModel* m) {
    static const char* const weekdays[] = {"Пн", "Вт", "Ср", "Чт", "Пт", "Сб", "Вс"};
    static const char* const months[] = {
        "янв", "фев", "мар", "апр", "мая", "июн", "июл", "авг", "сен", "окт", "ноя", "дек"};
    const DateTime* dt = &m->datetime;

    r0n1n_ui_battery(canvas, 127, 1, m->battery_pct);

    FuriString* str = furi_string_alloc();
    locale_format_time(str, dt, locale_get_time_format(), false);
    canvas_set_font(canvas, FontBigNumbers);
    canvas_draw_str_aligned(canvas, 64, 24, AlignCenter, AlignBottom, furi_string_get_cstr(str));

    furi_string_printf(
        str,
        "%s, %u %s",
        weekdays[(dt->weekday >= 1 && dt->weekday <= 7) ? dt->weekday - 1 : 0],
        dt->day,
        months[(dt->month >= 1 && dt->month <= 12) ? dt->month - 1 : 0]);
    canvas_set_custom_u8g2_font(canvas, u8g2_font_10x20_t_cyrillic);
    canvas_draw_str_aligned(canvas, 64, 43, AlignCenter, AlignBottom, furi_string_get_cstr(str));

    // "(OK) Меню" at the bottom
    canvas_draw_icon(canvas, 36, 52, &I_Ok_btn_9x9);
    canvas_draw_str(canvas, 49, 62, "Меню");
    canvas_set_font(canvas, FontSecondary);
    furi_string_free(str);
}

static void desktop_main_draw_callback(Canvas* canvas, void* model) {
    static const char* const weekdays[] = {"Пн", "Вт", "Ср", "Чт", "Пт", "Сб", "Вс"};
    static const char* const months[] = {
        "янв", "фев", "мар", "апр", "мая", "июн", "июл", "авг", "сен", "окт", "ноя", "дек"};
    DesktopMainViewModel* m = model;
    if(!m->has_datetime) return;

    canvas_set_color(canvas, ColorWhite);
    canvas_draw_box(canvas, 0, 0, 128, 64);
    canvas_set_color(canvas, ColorBlack);

    if(m->simple_mode) {
        desktop_main_draw_simple(canvas, m);
        return;
    }

    r0n1n_ui_battery(canvas, 127, 1, m->battery_pct);
    if(m->wallpaper) canvas_draw_icon(canvas, 77, 17, m->wallpaper);

    FuriString* str = furi_string_alloc();
    locale_format_time(str, &m->datetime, locale_get_time_format(), false);
    canvas_set_font(canvas, FontBigNumbers);
    canvas_draw_str(canvas, 1, DASHBOARD_CLOCK_Y, furi_string_get_cstr(str));

    const DateTime* dt = &m->datetime;
    furi_string_printf(
        str,
        "%s, %u %s %u",
        weekdays[(dt->weekday >= 1 && dt->weekday <= 7) ? dt->weekday - 1 : 0],
        dt->day,
        months[(dt->month >= 1 && dt->month <= 12) ? dt->month - 1 : 0],
        dt->year);
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 2, DASHBOARD_DATE_Y, furi_string_get_cstr(str));
    furi_string_free(str);

    canvas_draw_icon(canvas, 2, DASHBOARD_PROFILE_Y - 7, &I_R_Profile_7x7);
    canvas_draw_str(canvas, 12, DASHBOARD_PROFILE_Y, m->profile_name);
}

static bool desktop_main_dashboard_input_callback(InputEvent* event, void* context) {
    DesktopMainView* main_view = context;
    if(main_view->dummy_mode) {
        return false;
    }
    if(main_view->simple_mode) {
        // Simple mode: Right, like every arrow and OK, opens its menu
        if(event->key != InputKeyBack && event->type == InputTypeShort) {
            main_view->callback(DesktopMainEventOpenSimpleMenu, main_view->context);
            return true;
        }
        return false;
    }
    if(event->key == InputKeyRight && event->type == InputTypeShort) {
        main_view->callback(DesktopMainEventOpenSectionsRight, main_view->context);
        return true;
    }
    // Right press/release/long must still reach the views below (favorites).
    return false;
}

bool desktop_main_input_callback(InputEvent* event, void* context) {
    furi_assert(event);
    furi_assert(context);

    DesktopMainView* main_view = context;

    if(main_view->simple_mode) {
        // Only the basics: any arrow or OK opens the simple menu; holding
        // Back still reaches the power-off menu (below).
        if(event->type == InputTypeShort && event->key != InputKeyBack) {
            main_view->callback(DesktopMainEventOpenSimpleMenu, main_view->context);
        }
    } else if(main_view->dummy_mode == false) {
        if(event->type == InputTypeShort) {
            if(event->key == InputKeyOk) {
                main_view->callback(DesktopMainEventOpenMenu, main_view->context);
            } else if(event->key == InputKeyUp) {
                // R0N1N navigation law (docs/UX_DESIGN.md): Up = Quick Actions.
                main_view->callback(DesktopMainEventOpenFavorites, main_view->context);
            } else if(event->key == InputKeyDown) {
                // Down = Control Center.
                main_view->callback(DesktopMainEventOpenControlCenter, main_view->context);
            } else if(event->key == InputKeyLeft) {
                // Left/Right = sections; Right is taken by the dashboard view
                // on top.
                main_view->callback(DesktopMainEventOpenSectionsLeft, main_view->context);
            }
        } else if(event->type == InputTypeLong) {
            if(event->key == InputKeyUp) {
                main_view->callback(DesktopMainEventLock, main_view->context);
            } else if(event->key == InputKeyDown) {
                main_view->callback(DesktopMainEventOpenDebug, main_view->context);
            } else if(event->key == InputKeyLeft) {
                main_view->callback(DesktopMainEventOpenFavoriteLeftLong, main_view->context);
            } else if(event->key == InputKeyRight) {
                main_view->callback(DesktopMainEventOpenFavoriteRightLong, main_view->context);
            } else if(event->key == InputKeyOk) {
                // R0N1N navigation law: hold OK = Recent.
                main_view->callback(DesktopMainEventOpenRecent, main_view->context);
            } else if(event->key == InputKeyBack) {
                // Hold Back = Search, opened on release so that holding on
                // for the stock 5 s power-off menu still works.
                main_view->back_held = true;
            }
        }
    } else {
        if(event->type == InputTypeShort) {
            if(event->key == InputKeyOk) {
                main_view->callback(DesktopDummyEventOpenOk, main_view->context);
            } else if(event->key == InputKeyUp) {
                main_view->callback(DesktopMainEventOpenLockMenu, main_view->context);
            } else if(event->key == InputKeyDown) {
                main_view->callback(DesktopDummyEventOpenDown, main_view->context);
            } else if(event->key == InputKeyLeft) {
                main_view->callback(DesktopDummyEventOpenLeft, main_view->context);
            }
            // Right key short is handled by animation manager
        }
    }

    if(event->key == InputKeyBack) {
        if(event->type == InputTypePress) {
            main_view->back_held = false;
            furi_timer_start(main_view->poweroff_timer, DESKTOP_MAIN_VIEW_POWEROFF_TIMEOUT);
        } else if(event->type == InputTypeRelease) {
            furi_timer_stop(main_view->poweroff_timer);
            if(main_view->back_held) {
                main_view->back_held = false;
                main_view->callback(DesktopMainEventOpenSearch, main_view->context);
            }
        }
    }

    return true;
}

DesktopMainView* desktop_main_alloc(void) {
    DesktopMainView* main_view = malloc(sizeof(DesktopMainView));

    main_view->view = view_alloc();
    view_set_context(main_view->view, main_view);
    view_set_input_callback(main_view->view, desktop_main_input_callback);

    // See the dashboard_view comment on the struct above.
    main_view->dashboard_view = view_alloc();
    view_allocate_model(
        main_view->dashboard_view, ViewModelTypeLocking, sizeof(DesktopMainViewModel));
    view_set_context(main_view->dashboard_view, main_view);
    view_set_draw_callback(main_view->dashboard_view, desktop_main_draw_callback);
    view_set_input_callback(main_view->dashboard_view, desktop_main_dashboard_input_callback);

    main_view->poweroff_timer =
        furi_timer_alloc(desktop_main_poweroff_timer_callback, FuriTimerTypeOnce, main_view);

    return main_view;
}

void desktop_main_free(DesktopMainView* main_view) {
    furi_assert(main_view);
    view_free(main_view->view);
    view_free(main_view->dashboard_view);
    furi_timer_free(main_view->poweroff_timer);
    free(main_view);
}
