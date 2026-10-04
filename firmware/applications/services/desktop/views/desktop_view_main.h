#pragma once

#include <gui/view.h>
#include <gui/icon.h>
#include <datetime/datetime.h>
#include "desktop_events.h"

typedef struct DesktopMainView DesktopMainView;

typedef void (*DesktopMainViewCallback)(DesktopEvent event, void* context);

void desktop_main_set_callback(
    DesktopMainView* main_view,
    DesktopMainViewCallback callback,
    void* context);

View* desktop_main_get_view(DesktopMainView* main_view);

// R0N1N Home dashboard: a separate View meant to be stacked *above* the
// dolphin animation view in desktop.c, so the dashboard draws on top of it.
// See the dashboard_view comment in desktop_view_main.c for why this isn't
// just desktop_main_get_view() reordered in the stack instead.
View* desktop_main_get_dashboard_view(DesktopMainView* main_view);

void desktop_main_set_dummy_mode_state(DesktopMainView* main_view, bool dummy_mode);

/** R0N1N simple mode: big clock and date on Home, any arrow or OK opens its menu. */
void desktop_main_set_simple_mode(DesktopMainView* main_view, bool simple_mode);

// R0N1N Home dashboard (see docs/UX_DESIGN.md): pushes the clock/date/profile
// content the draw callback renders. Called from a periodic timer while the
// Main scene is active (desktop_scene_main.c), not read from inside the view
// itself, matching how desktop_view_lock_menu takes its state
// from the scene layer rather than touching RTC/locale directly.
void desktop_main_update_dashboard(
    DesktopMainView* main_view,
    const DateTime* datetime,
    const char* profile_name,
    uint8_t battery_pct,
    const Icon* wallpaper);

DesktopMainView* desktop_main_alloc(void);
void desktop_main_free(DesktopMainView* main_view);
