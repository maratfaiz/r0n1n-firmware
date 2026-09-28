#include <furi.h>
#include <furi_hal.h>
#include <loader/loader.h>

#include "../desktop_i.h"
#include "../views/desktop_events.h"
#include "../views/desktop_view_main.h"
#include "desktop_scene.h"

#define TAG "DesktopSrv"

static void desktop_scene_main_new_idle_animation_callback(void* context) {
    furi_assert(context);
    Desktop* desktop = context;
    view_dispatcher_send_custom_event(
        desktop->view_dispatcher, DesktopAnimationEventNewIdleAnimation);
}

static void desktop_scene_main_check_animation_callback(void* context) {
    furi_assert(context);
    Desktop* desktop = context;
    view_dispatcher_send_custom_event(
        desktop->view_dispatcher, DesktopAnimationEventCheckAnimation);
}

static void desktop_scene_main_interact_animation_callback(void* context) {
    furi_assert(context);
    Desktop* desktop = context;
    view_dispatcher_send_custom_event(
        desktop->view_dispatcher, DesktopAnimationEventInteractAnimation);
}

static void desktop_scene_main_open_app_or_profile(Desktop* desktop, FavoriteApp* application) {
    if(strlen(application->name_or_path) > 0) {
        loader_start_detached_with_gui_error(desktop->loader, application->name_or_path, NULL);
    } else {
        loader_start_detached_with_gui_error(desktop->loader, "Passport", NULL);
    }
}

static void desktop_scene_main_start_favorite(Desktop* desktop, FavoriteApp* application) {
    if(strlen(application->name_or_path) > 0) {
        loader_start_detached_with_gui_error(desktop->loader, application->name_or_path, NULL);
    } else {
        loader_start_detached_with_gui_error(desktop->loader, LOADER_APPLICATIONS_NAME, NULL);
    }
}

void desktop_scene_main_callback(DesktopEvent event, void* context) {
    Desktop* desktop = (Desktop*)context;
    if(desktop->in_transition) return;
    view_dispatcher_send_custom_event(desktop->view_dispatcher, event);
}

void desktop_scene_main_on_enter(void* context) {
    Desktop* desktop = (Desktop*)context;
    DesktopMainView* main_view = desktop->main_view;

    animation_manager_set_context(desktop->animation_manager, desktop);
    animation_manager_set_new_idle_callback(
        desktop->animation_manager, desktop_scene_main_new_idle_animation_callback);
    animation_manager_set_check_callback(
        desktop->animation_manager, desktop_scene_main_check_animation_callback);
    animation_manager_set_interact_callback(
        desktop->animation_manager, desktop_scene_main_interact_animation_callback);

    desktop_main_set_callback(main_view, desktop_scene_main_callback, desktop);

    // R0N1N Home dashboard: paint immediately so the clock isn't blank/stale
    // for the first second, then keep it ticking while this scene is shown.
    DateTime datetime;
    furi_hal_rtc_get_datetime(&datetime);
    const uint8_t wp =
        desktop->r0n1n.wallpaper < r0n1n_wallpapers_count ? desktop->r0n1n.wallpaper : 0;
    desktop_main_update_dashboard(
        main_view,
        &datetime,
        r0n1n_profiles[desktop->r0n1n.profile].name,
        furi_hal_power_get_pct(),
        r0n1n_wallpapers[wp].icon);
    furi_timer_start(desktop->dashboard_update_timer, furi_ms_to_ticks(1000));

    view_dispatcher_switch_to_view(desktop->view_dispatcher, DesktopViewIdMain);
}

bool desktop_scene_main_on_event(void* context, SceneManagerEvent event) {
    Desktop* desktop = (Desktop*)context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        switch(event.event) {
        case DesktopMainEventOpenMenu:
            scene_manager_next_scene(desktop->scene_manager, DesktopSceneMenu);
            consumed = true;
            break;

        case DesktopMainEventLock:
            desktop_lock(desktop);
            consumed = true;
            break;

        case DesktopMainEventOpenLockMenu:
            scene_manager_next_scene(desktop->scene_manager, DesktopSceneLockMenu);
            consumed = true;
            break;

        case DesktopMainEventOpenDebug:
            scene_manager_next_scene(desktop->scene_manager, DesktopSceneDebug);
            consumed = true;
            break;

        case DesktopMainEventOpenFavorites:
            scene_manager_next_scene(desktop->scene_manager, DesktopSceneFavorites);
            consumed = true;
            break;

        case DesktopMainEventOpenRecent:
            scene_manager_next_scene(desktop->scene_manager, DesktopSceneRecent);
            consumed = true;
            break;

        case DesktopMainEventOpenControlCenter:
            scene_manager_set_scene_state(desktop->scene_manager, DesktopSceneControlCenter, 0);
            scene_manager_next_scene(desktop->scene_manager, DesktopSceneControlCenter);
            consumed = true;
            break;

        case DesktopMainEventOpenSectionsLeft:
        case DesktopMainEventOpenSectionsRight:
            // Like swiping: Right starts at the first section, Left at the last.
            scene_manager_set_scene_state(
                desktop->scene_manager,
                DesktopSceneSections,
                event.event == DesktopMainEventOpenSectionsLeft ? UINT32_MAX : 0);
            scene_manager_next_scene(desktop->scene_manager, DesktopSceneSections);
            consumed = true;
            break;

        case DesktopMainEventOpenSearch:
            scene_manager_next_scene(desktop->scene_manager, DesktopSceneSearch);
            consumed = true;
            break;

        case DesktopMainEventOpenSimpleMenu:
            scene_manager_next_scene(desktop->scene_manager, DesktopSceneSimpleMenu);
            consumed = true;
            break;

        case DesktopMainEventOpenPowerOff: {
            loader_start_detached_with_gui_error(desktop->loader, "Power", "off");
            consumed = true;
            break;
        }

        case DesktopMainEventOpenFavoriteLeftShort:
            desktop_scene_main_start_favorite(
                desktop, &desktop->settings.favorite_apps[FavoriteAppLeftShort]);
            consumed = true;
            break;
        case DesktopMainEventOpenFavoriteLeftLong:
            desktop_scene_main_start_favorite(
                desktop, &desktop->settings.favorite_apps[FavoriteAppLeftLong]);
            consumed = true;
            break;
        case DesktopMainEventOpenFavoriteRightShort:
            desktop_scene_main_start_favorite(
                desktop, &desktop->settings.favorite_apps[FavoriteAppRightShort]);
            consumed = true;
            break;
        case DesktopMainEventOpenFavoriteRightLong:
            desktop_scene_main_start_favorite(
                desktop, &desktop->settings.favorite_apps[FavoriteAppRightLong]);
            consumed = true;
            break;

        case DesktopAnimationEventCheckAnimation:
            animation_manager_check_blocking_process(desktop->animation_manager);
            consumed = true;
            break;
        case DesktopAnimationEventNewIdleAnimation:
            animation_manager_new_idle_process(desktop->animation_manager);
            consumed = true;
            break;
        case DesktopAnimationEventInteractAnimation:
            if(!animation_manager_interact_process(desktop->animation_manager)) {
                if(!desktop->settings.dummy_mode) {
                    desktop_scene_main_open_app_or_profile(
                        desktop, &desktop->settings.favorite_apps[FavoriteAppRightShort]);
                } else {
                    desktop_scene_main_open_app_or_profile(
                        desktop, &desktop->settings.dummy_apps[DummyAppRight]);
                }
            }
            consumed = true;
            break;

        case DesktopDummyEventOpenLeft:
            desktop_scene_main_open_app_or_profile(
                desktop, &desktop->settings.dummy_apps[DummyAppLeft]);
            break;
        case DesktopDummyEventOpenDown:
            desktop_scene_main_open_app_or_profile(
                desktop, &desktop->settings.dummy_apps[DummyAppDown]);
            break;
        case DesktopDummyEventOpenOk:
            desktop_scene_main_open_app_or_profile(
                desktop, &desktop->settings.dummy_apps[DummyAppOk]);
            break;

        case DesktopLockedEventUpdate:
            desktop_view_locked_update(desktop->locked_view);
            consumed = true;
            break;

        default:
            break;
        }
    }

    return consumed;
}

void desktop_scene_main_on_exit(void* context) {
    Desktop* desktop = (Desktop*)context;

    furi_timer_stop(desktop->dashboard_update_timer);

    animation_manager_set_new_idle_callback(desktop->animation_manager, NULL);
    animation_manager_set_check_callback(desktop->animation_manager, NULL);
    animation_manager_set_interact_callback(desktop->animation_manager, NULL);
    animation_manager_set_context(desktop->animation_manager, desktop);
}
