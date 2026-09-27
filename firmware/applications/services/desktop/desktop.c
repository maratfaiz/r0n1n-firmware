#include "desktop_i.h"

#include <cli/cli_vcp.h>

#include <gui/gui_i.h>

#include <locale/locale.h>
#include <storage/storage.h>

#include <applications.h>
#include <assets_icons.h>

#include "scenes/desktop_scene.h"
#include "scenes/desktop_scene_r0n1n.h"
#include "scenes/desktop_scene_locked.h"
#include "helpers/r0n1n_boot.h"

#define TAG "Desktop"

static void desktop_auto_lock_arm(Desktop*);
static void desktop_auto_lock_inhibit(Desktop*);
static void desktop_start_auto_lock_timer(Desktop*);
static void desktop_apply_settings(Desktop*);
static void desktop_dashboard_update(Desktop*);

static void desktop_loader_callback(const void* message, void* context) {
    furi_assert(context);
    Desktop* desktop = context;
    const LoaderEvent* event = message;

    if(event->type == LoaderEventTypeApplicationBeforeLoad) {
        // R0N1N Recent apps: stash the name here (Loader's thread) before the
        // custom event is even enqueued, so the handler on the ViewDispatcher's
        // thread (DesktopGlobalBeforeAppStarted below) sees it once dequeued.
        // A name too long to store whole is dropped rather than truncated: a
        // truncated .fap path would sit in Recent and fail to relaunch.
        if(!event->name ||
           strlcpy(desktop->pending_app_name, event->name, sizeof(desktop->pending_app_name)) >=
               sizeof(desktop->pending_app_name)) {
            desktop->pending_app_name[0] = '\0';
        }
        view_dispatcher_send_custom_event(desktop->view_dispatcher, DesktopGlobalBeforeAppStarted);
        furi_check(furi_semaphore_acquire(desktop->animation_semaphore, 3000) == FuriStatusOk);
    } else if(event->type == LoaderEventTypeApplicationStopped) {
        view_dispatcher_send_custom_event(desktop->view_dispatcher, DesktopGlobalAppStopped);
    } else if(event->type == LoaderEventTypeNoMoreAppsInQueue) {
        view_dispatcher_send_custom_event(desktop->view_dispatcher, DesktopGlobalAfterAppFinished);
    }
}

static void desktop_storage_callback(const void* message, void* context) {
    furi_assert(context);
    Desktop* desktop = context;
    const StorageEvent* event = message;

    if(event->type == StorageEventTypeCardMount) {
        view_dispatcher_send_custom_event(desktop->view_dispatcher, DesktopGlobalReloadSettings);
    }
}

static void desktop_lock_icon_draw_callback(Canvas* canvas, void* context) {
    UNUSED(context);
    furi_assert(canvas);
    canvas_draw_icon(canvas, 0, 0, &I_Lock_7x8);
}

static void desktop_dummy_mode_icon_draw_callback(Canvas* canvas, void* context) {
    UNUSED(context);
    furi_assert(canvas);
    canvas_draw_icon(canvas, 0, 0, &I_GameMode_11x8);
}

static void desktop_clock_update(Desktop* desktop) {
    furi_assert(desktop);

    DateTime curr_dt;
    furi_hal_rtc_get_datetime(&curr_dt);
    bool time_format_12 = locale_get_time_format() == LocaleTimeFormat12h;

    if(desktop->clock.hour != curr_dt.hour || desktop->clock.minute != curr_dt.minute ||
       desktop->clock.format_12 != time_format_12) {
        desktop->clock.format_12 = time_format_12;
        desktop->clock.hour = curr_dt.hour;
        desktop->clock.minute = curr_dt.minute;
        view_port_update(desktop->clock_viewport);
    }
}

static void desktop_clock_reconfigure(Desktop* desktop) {
    furi_assert(desktop);

    desktop_clock_update(desktop);

    if(desktop->settings.display_clock) {
        furi_timer_start(desktop->update_clock_timer, furi_ms_to_ticks(1000));
    } else {
        furi_timer_stop(desktop->update_clock_timer);
    }

    view_port_enabled_set(desktop->clock_viewport, desktop->settings.display_clock);
}

static void desktop_clock_draw_callback(Canvas* canvas, void* context) {
    furi_assert(context);
    furi_assert(canvas);

    Desktop* desktop = context;

    canvas_set_font(canvas, FontPrimary);

    uint8_t hour = desktop->clock.hour;
    if(desktop->clock.format_12) {
        if(hour > 12) {
            hour -= 12;
        }
        if(hour == 0) {
            hour = 12;
        }
    }

    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%02u:%02u", hour, desktop->clock.minute);

    view_port_set_width(
        desktop->clock_viewport,
        canvas_string_width(canvas, buffer) - 1 + (desktop->clock.minute % 10 == 1));

    canvas_draw_str_aligned(canvas, 0, 8, AlignLeft, AlignBottom, buffer);
}

static void desktop_stealth_mode_icon_draw_callback(Canvas* canvas, void* context) {
    UNUSED(context);
    furi_assert(canvas);
    canvas_draw_icon(canvas, 0, 0, &I_Muted_8x8);
}

// R0N1N Recent apps (docs/UX_DESIGN.md): most-recent first, capped at
// DESKTOP_RECENT_APPS_COUNT. Relaunching an app already in the list moves it
// to the front instead of adding a duplicate; otherwise the oldest entry is
// dropped once the list is full.
static void desktop_recent_apps_push(Desktop* desktop, const char* name) {
    if(name[0] == '\0') return;

    uint8_t i = 0;
    while(i < desktop->recent_apps_count && strcmp(desktop->recent_apps[i], name) != 0) {
        i++;
    }
    if(i == desktop->recent_apps_count) {
        if(desktop->recent_apps_count < DESKTOP_RECENT_APPS_COUNT) {
            desktop->recent_apps_count++;
        }
        i = desktop->recent_apps_count - 1;
    }
    for(; i > 0; i--) {
        strlcpy(desktop->recent_apps[i], desktop->recent_apps[i - 1], DESKTOP_RECENT_APP_NAME_LEN);
        desktop->recent_apps_time[i] = desktop->recent_apps_time[i - 1];
    }
    strlcpy(desktop->recent_apps[0], name, DESKTOP_RECENT_APP_NAME_LEN);
    desktop->recent_apps_time[0] = furi_hal_rtc_get_timestamp();
}

static bool desktop_custom_event_callback(void* context, uint32_t event) {
    furi_assert(context);
    Desktop* desktop = (Desktop*)context;

    if(event == DesktopGlobalBeforeAppStarted) {
        if(animation_manager_is_animation_loaded(desktop->animation_manager)) {
            animation_manager_unload_and_stall_animation(desktop->animation_manager);
        }

        desktop_auto_lock_inhibit(desktop);
        desktop->app_running = true;
        strlcpy(
            desktop->launched_app_name,
            desktop->pending_app_name,
            sizeof(desktop->launched_app_name));

        furi_semaphore_release(desktop->animation_semaphore);

    } else if(event == DesktopGlobalAfterAppFinished) {
        animation_manager_load_and_continue_animation(desktop->animation_manager);
        desktop_auto_lock_arm(desktop);
        desktop->app_running = false;
        // The dashboard timer skips ticks while an app runs (see its
        // callback); repaint now so Home doesn't show a stale clock for up
        // to a second after returning.
        if(furi_timer_is_running(desktop->dashboard_update_timer)) {
            desktop_dashboard_update(desktop);
        }

    } else if(event == DesktopGlobalAppStopped) {
        desktop_recent_apps_push(desktop, desktop->launched_app_name);
        desktop->launched_app_name[0] = '\0';

    } else if(event == DesktopGlobalAutoLock) {
        if(!desktop->app_running && !desktop->locked) {
            desktop_lock(desktop);
        }

    } else if(event == DesktopGlobalSaveSettings) {
        desktop_settings_save(&desktop->settings);
        desktop_apply_settings(desktop);

    } else if(event == DesktopGlobalReloadSettings) {
        desktop_settings_load(&desktop->settings);
        r0n1n_settings_load(&desktop->r0n1n);
        desktop_apply_settings(desktop);

    } else {
        // R0N1N: a short click when something is selected in the shell
        const uint32_t kind = event & R0N1N_EVT_KIND;
        if(kind == R0N1N_EVT_OK || kind == R0N1N_EVT_HOLD) {
            desktop_r0n1n_feedback(desktop, R0n1nFeedbackClick);
        }
        return scene_manager_handle_custom_event(desktop->scene_manager, event);
    }

    return true;
}

static bool desktop_back_event_callback(void* context) {
    furi_assert(context);
    Desktop* desktop = (Desktop*)context;
    return scene_manager_handle_back_event(desktop->scene_manager);
}

static void desktop_tick_event_callback(void* context) {
    furi_assert(context);
    Desktop* app = context;
    scene_manager_handle_tick_event(app->scene_manager);
}

static void desktop_input_event_callback(const void* value, void* context) {
    furi_assert(value);
    furi_assert(context);
    const InputEvent* event = value;
    Desktop* desktop = context;
    if(event->type == InputTypePress) {
        desktop_start_auto_lock_timer(desktop);
    }
}

static void desktop_auto_lock_timer_callback(void* context) {
    furi_assert(context);
    Desktop* desktop = context;
    view_dispatcher_send_custom_event(desktop->view_dispatcher, DesktopGlobalAutoLock);
}

static void desktop_start_auto_lock_timer(Desktop* desktop) {
    furi_timer_start(
        desktop->auto_lock_timer, furi_ms_to_ticks(desktop->settings.auto_lock_delay_ms));
}

static void desktop_stop_auto_lock_timer(Desktop* desktop) {
    furi_timer_stop(desktop->auto_lock_timer);
}

static void desktop_auto_lock_arm(Desktop* desktop) {
    if(desktop->settings.auto_lock_delay_ms) {
        desktop->input_events_subscription = furi_pubsub_subscribe(
            desktop->input_events_pubsub, desktop_input_event_callback, desktop);
        desktop_start_auto_lock_timer(desktop);
    }
}

static void desktop_auto_lock_inhibit(Desktop* desktop) {
    desktop_stop_auto_lock_timer(desktop);
    if(desktop->input_events_subscription) {
        furi_pubsub_unsubscribe(desktop->input_events_pubsub, desktop->input_events_subscription);
        desktop->input_events_subscription = NULL;
    }
}

static void desktop_clock_timer_callback(void* context) {
    furi_assert(context);
    Desktop* desktop = context;

    const bool clock_enabled = gui_active_view_port_count(desktop->gui, GuiLayerStatusBarLeft) < 6;

    if(clock_enabled) {
        desktop_clock_update(desktop);
    }

    view_port_enabled_set(desktop->clock_viewport, clock_enabled);
}

// R0N1N Home dashboard (docs/UX_DESIGN.md): drives desktop_view_main's big
// clock/date/profile display. Only ticks while desktop_scene_main is active
// (started/stopped there), independent of the small status-bar clock above.
static void desktop_dashboard_update(Desktop* desktop) {
    DateTime datetime;
    furi_hal_rtc_get_datetime(&datetime);
    desktop_main_update_dashboard(
        desktop->main_view,
        &datetime,
        r0n1n_profiles[desktop->r0n1n.profile].name,
        furi_hal_power_get_pct());
}

static void desktop_dashboard_update_timer_callback(void* context) {
    furi_assert(context);
    Desktop* desktop = context;

    // The Main scene stays current underneath a running app, so without this
    // every tick would request a full GUI redraw for a screen nobody sees.
    if(desktop->app_running) return;

    desktop_dashboard_update(desktop);
}

static void desktop_apply_settings(Desktop* desktop) {
    desktop->in_transition = true;

    desktop_clock_reconfigure(desktop);

    // R0N1N has no dummy ("game") mode: its Home is always the dashboard.
    // Clear a flag saved by an earlier firmware so nobody gets stuck in it.
    desktop->settings.dummy_mode = false;

    view_port_enabled_set(desktop->dummy_mode_icon_viewport, desktop->settings.dummy_mode);
    desktop_main_set_simple_mode(desktop->main_view, desktop->r0n1n.simple_mode);
    desktop_main_set_dummy_mode_state(desktop->main_view, desktop->settings.dummy_mode);
    animation_manager_set_dummy_mode_state(
        desktop->animation_manager, desktop->settings.dummy_mode);

    if(!desktop->app_running && !desktop->locked) {
        desktop_auto_lock_arm(desktop);
    }

    desktop->in_transition = false;
}

static void desktop_init_settings(Desktop* desktop) {
    furi_pubsub_subscribe(storage_get_pubsub(desktop->storage), desktop_storage_callback, desktop);

    if(storage_sd_status(desktop->storage) != FSE_OK) {
        FURI_LOG_D(TAG, "SD Card not ready, skipping settings");
        return;
    }

    desktop_settings_load(&desktop->settings);
    r0n1n_settings_load(&desktop->r0n1n);
    desktop_apply_settings(desktop);
}

static Desktop* desktop_alloc(void) {
    Desktop* desktop = malloc(sizeof(Desktop));
    // R0N1N defaults until the SD card (and /int on it) is ready.
    r0n1n_settings_load(&desktop->r0n1n);

    desktop->animation_semaphore = furi_semaphore_alloc(1, 0);
    desktop->animation_manager = animation_manager_alloc();
    desktop->gui = furi_record_open(RECORD_GUI);
    desktop->scene_thread = furi_thread_alloc();
    desktop->view_dispatcher = view_dispatcher_alloc();
    desktop->scene_manager = scene_manager_alloc(&desktop_scene_handlers, desktop);

    view_dispatcher_attach_to_gui(
        desktop->view_dispatcher, desktop->gui, ViewDispatcherTypeDesktop);
    view_dispatcher_set_tick_event_callback(
        desktop->view_dispatcher, desktop_tick_event_callback, 500);

    view_dispatcher_set_event_callback_context(desktop->view_dispatcher, desktop);
    view_dispatcher_set_custom_event_callback(
        desktop->view_dispatcher, desktop_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(
        desktop->view_dispatcher, desktop_back_event_callback);

    desktop->lock_menu = desktop_lock_menu_alloc();
    desktop->debug_view = desktop_debug_alloc();
    desktop->popup = popup_alloc();
    desktop->locked_view = desktop_view_locked_alloc();
    desktop->pin_input_view = desktop_view_pin_input_alloc();
    desktop->pin_timeout_view = desktop_view_pin_timeout_alloc();
    desktop->slideshow_view = desktop_view_slideshow_alloc();
    desktop->r0n1n_list = r0n1n_list_alloc();
    desktop->r0n1n_grid = r0n1n_grid_alloc();
    desktop->r0n1n_carousel = r0n1n_carousel_alloc();
    desktop->simple_menu = desktop_simple_menu_alloc();
    desktop->text_input = text_input_alloc();
    desktop->dialog_ex = dialog_ex_alloc();

    desktop->main_view_stack = view_stack_alloc();
    desktop->main_view = desktop_main_alloc();
    View* dolphin_view = animation_manager_get_animation_view(desktop->animation_manager);
    view_stack_add_view(desktop->main_view_stack, desktop_main_get_view(desktop->main_view));
    view_stack_add_view(desktop->main_view_stack, dolphin_view);
    // R0N1N Home dashboard: added after dolphin_view so it draws on top of
    // it; safe to do purely for draw order because it has no input callback
    // (see desktop_view_main.c). desktop_main_get_view() above stays put for
    // input priority -- do not reorder that one.
    view_stack_add_view(
        desktop->main_view_stack, desktop_main_get_dashboard_view(desktop->main_view));
    view_stack_add_view(
        desktop->main_view_stack, desktop_view_locked_get_view(desktop->locked_view));

    /* locked view (as animation view) attends in 2 scenes: main & locked,
     * because it has to draw "Unlocked" label on main scene */
    desktop->locked_view_stack = view_stack_alloc();
    view_stack_add_view(desktop->locked_view_stack, dolphin_view);
    view_stack_add_view(
        desktop->locked_view_stack, desktop_view_locked_get_view(desktop->locked_view));

    view_dispatcher_add_view(
        desktop->view_dispatcher,
        DesktopViewIdMain,
        view_stack_get_view(desktop->main_view_stack));
    view_dispatcher_add_view(
        desktop->view_dispatcher,
        DesktopViewIdLocked,
        view_stack_get_view(desktop->locked_view_stack));
    view_dispatcher_add_view(
        desktop->view_dispatcher,
        DesktopViewIdLockMenu,
        desktop_lock_menu_get_view(desktop->lock_menu));
    view_dispatcher_add_view(
        desktop->view_dispatcher, DesktopViewIdDebug, desktop_debug_get_view(desktop->debug_view));
    view_dispatcher_add_view(
        desktop->view_dispatcher, DesktopViewIdPopup, popup_get_view(desktop->popup));
    view_dispatcher_add_view(
        desktop->view_dispatcher,
        DesktopViewIdPinTimeout,
        desktop_view_pin_timeout_get_view(desktop->pin_timeout_view));
    view_dispatcher_add_view(
        desktop->view_dispatcher,
        DesktopViewIdPinInput,
        desktop_view_pin_input_get_view(desktop->pin_input_view));
    view_dispatcher_add_view(
        desktop->view_dispatcher,
        DesktopViewIdSlideshow,
        desktop_view_slideshow_get_view(desktop->slideshow_view));
    view_dispatcher_add_view(
        desktop->view_dispatcher,
        DesktopViewIdR0n1nList,
        r0n1n_list_get_view(desktop->r0n1n_list));
    view_dispatcher_add_view(
        desktop->view_dispatcher,
        DesktopViewIdR0n1nGrid,
        r0n1n_grid_get_view(desktop->r0n1n_grid));
    view_dispatcher_add_view(
        desktop->view_dispatcher,
        DesktopViewIdR0n1nCarousel,
        r0n1n_carousel_get_view(desktop->r0n1n_carousel));
    view_dispatcher_add_view(
        desktop->view_dispatcher,
        DesktopViewIdSimpleMenu,
        desktop_simple_menu_get_view(desktop->simple_menu));
    view_dispatcher_add_view(
        desktop->view_dispatcher,
        DesktopViewIdTextInput,
        text_input_get_view(desktop->text_input));
    view_dispatcher_add_view(
        desktop->view_dispatcher, DesktopViewIdDialog, dialog_ex_get_view(desktop->dialog_ex));

    // Lock icon
    desktop->lock_icon_viewport = view_port_alloc();
    view_port_set_width(desktop->lock_icon_viewport, icon_get_width(&I_Lock_7x8));
    view_port_draw_callback_set(
        desktop->lock_icon_viewport, desktop_lock_icon_draw_callback, desktop);
    view_port_enabled_set(desktop->lock_icon_viewport, false);
    gui_add_view_port(desktop->gui, desktop->lock_icon_viewport, GuiLayerStatusBarLeft);

    // Dummy mode icon
    desktop->dummy_mode_icon_viewport = view_port_alloc();
    view_port_set_width(desktop->dummy_mode_icon_viewport, icon_get_width(&I_GameMode_11x8));
    view_port_draw_callback_set(
        desktop->dummy_mode_icon_viewport, desktop_dummy_mode_icon_draw_callback, desktop);
    view_port_enabled_set(desktop->dummy_mode_icon_viewport, false);
    gui_add_view_port(desktop->gui, desktop->dummy_mode_icon_viewport, GuiLayerStatusBarLeft);

    // Clock
    desktop->clock_viewport = view_port_alloc();
    view_port_set_width(desktop->clock_viewport, 25);
    view_port_draw_callback_set(desktop->clock_viewport, desktop_clock_draw_callback, desktop);
    view_port_enabled_set(desktop->clock_viewport, false);
    gui_add_view_port(desktop->gui, desktop->clock_viewport, GuiLayerStatusBarRight);

    // Stealth mode icon
    desktop->stealth_mode_icon_viewport = view_port_alloc();
    view_port_set_width(desktop->stealth_mode_icon_viewport, icon_get_width(&I_Muted_8x8));
    view_port_draw_callback_set(
        desktop->stealth_mode_icon_viewport, desktop_stealth_mode_icon_draw_callback, desktop);
    if(furi_hal_rtc_is_flag_set(FuriHalRtcFlagStealthMode)) {
        view_port_enabled_set(desktop->stealth_mode_icon_viewport, true);
    } else {
        view_port_enabled_set(desktop->stealth_mode_icon_viewport, false);
    }
    gui_add_view_port(desktop->gui, desktop->stealth_mode_icon_viewport, GuiLayerStatusBarLeft);

    // Unload animations before starting an application
    desktop->loader = furi_record_open(RECORD_LOADER);
    furi_pubsub_subscribe(loader_get_pubsub(desktop->loader), desktop_loader_callback, desktop);

    desktop->storage = furi_record_open(RECORD_STORAGE);
    desktop->notification = furi_record_open(RECORD_NOTIFICATION);
    desktop->input_events_pubsub = furi_record_open(RECORD_INPUT_EVENTS);

    desktop->auto_lock_timer =
        furi_timer_alloc(desktop_auto_lock_timer_callback, FuriTimerTypeOnce, desktop);

    desktop->status_pubsub = furi_pubsub_alloc();

    desktop->update_clock_timer =
        furi_timer_alloc(desktop_clock_timer_callback, FuriTimerTypePeriodic, desktop);

    desktop->dashboard_update_timer =
        furi_timer_alloc(desktop_dashboard_update_timer_callback, FuriTimerTypePeriodic, desktop);

    desktop->app_running = loader_is_locked(desktop->loader);

    furi_record_create(RECORD_DESKTOP, desktop);

    return desktop;
}

/*
 * Private API
 */

void desktop_lock(Desktop* desktop) {
    furi_assert(!desktop->locked);

    furi_hal_rtc_set_flag(FuriHalRtcFlagLock);

    if(desktop_pin_code_is_set()) {
        CliVcp* cli_vcp = furi_record_open(RECORD_CLI_VCP);
        cli_vcp_disable(cli_vcp);
        furi_record_close(RECORD_CLI_VCP);
    }

    desktop_auto_lock_inhibit(desktop);
    scene_manager_set_scene_state(
        desktop->scene_manager, DesktopSceneLocked, DesktopSceneLockedStateFirstEnter);
    scene_manager_next_scene(desktop->scene_manager, DesktopSceneLocked);

    DesktopStatus status = {.locked = true};
    furi_pubsub_publish(desktop->status_pubsub, &status);

    desktop->locked = true;
}

void desktop_unlock(Desktop* desktop) {
    furi_assert(desktop->locked);

    view_port_enabled_set(desktop->lock_icon_viewport, false);
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_set_lockdown(gui, false);
    furi_record_close(RECORD_GUI);
    desktop_view_locked_unlock(desktop->locked_view);
    scene_manager_search_and_switch_to_previous_scene(desktop->scene_manager, DesktopSceneMain);
    desktop_auto_lock_arm(desktop);
    furi_hal_rtc_reset_flag(FuriHalRtcFlagLock);
    furi_hal_rtc_set_pin_fails(0);

    if(desktop_pin_code_is_set()) {
        CliVcp* cli_vcp = furi_record_open(RECORD_CLI_VCP);
        cli_vcp_enable(cli_vcp);
        furi_record_close(RECORD_CLI_VCP);
    }

    DesktopStatus status = {.locked = false};
    furi_pubsub_publish(desktop->status_pubsub, &status);

    desktop->locked = false;
}

void desktop_set_dummy_mode_state(Desktop* desktop, bool enabled) {
    desktop->in_transition = true;

    view_port_enabled_set(desktop->dummy_mode_icon_viewport, enabled);
    desktop_main_set_dummy_mode_state(desktop->main_view, enabled);
    animation_manager_set_dummy_mode_state(desktop->animation_manager, enabled);
    desktop->settings.dummy_mode = enabled;

    desktop->in_transition = false;

    desktop_settings_save(&desktop->settings);
}

void desktop_set_stealth_mode_state(Desktop* desktop, bool enabled) {
    desktop->in_transition = true;

    if(enabled) {
        furi_hal_rtc_set_flag(FuriHalRtcFlagStealthMode);
    } else {
        furi_hal_rtc_reset_flag(FuriHalRtcFlagStealthMode);
    }

    view_port_enabled_set(desktop->stealth_mode_icon_viewport, enabled);

    desktop->in_transition = false;
}

// Archive is not a Loader app: it runs on the desktop's own thread so it can
// start other apps through the Loader itself. The stock firmware only reaches
// it from Down-short on Home; R0N1N gives Down to Control Center, so Quick
// Actions (desktop_scene_favorites.c) is where it's launched from instead.
void desktop_run_archive(Desktop* desktop) {
    furi_assert(desktop);
#ifdef APP_ARCHIVE
    const FlipperInternalApplication* flipper_app = &FLIPPER_ARCHIVE;

    if(furi_thread_get_state(desktop->scene_thread) != FuriThreadStateStopped) {
        FURI_LOG_E("Desktop", "Thread is already running");
        return;
    }

    FuriHalRtcHeapTrackMode mode = furi_hal_rtc_get_heap_track_mode();
    if(mode > FuriHalRtcHeapTrackModeNone) {
        furi_thread_enable_heap_trace(desktop->scene_thread);
    } else {
        furi_thread_disable_heap_trace(desktop->scene_thread);
    }

    furi_thread_set_name(desktop->scene_thread, flipper_app->name);
    furi_thread_set_stack_size(desktop->scene_thread, flipper_app->stack_size);
    furi_thread_set_callback(desktop->scene_thread, flipper_app->app);

    furi_thread_start(desktop->scene_thread);
#endif
}

/*
 *  Public API
 */

bool desktop_api_is_locked(Desktop* instance) {
    furi_assert(instance);
    return furi_hal_rtc_is_flag_set(FuriHalRtcFlagLock);
}

void desktop_api_unlock(Desktop* instance) {
    furi_assert(instance);
    view_dispatcher_send_custom_event(instance->view_dispatcher, DesktopGlobalApiUnlock);
}

FuriPubSub* desktop_api_get_status_pubsub(Desktop* instance) {
    furi_assert(instance);
    return instance->status_pubsub;
}

void desktop_api_reload_settings(Desktop* instance) {
    furi_assert(instance);
    view_dispatcher_send_custom_event(instance->view_dispatcher, DesktopGlobalReloadSettings);
}

void desktop_api_get_settings(Desktop* instance, DesktopSettings* settings) {
    furi_assert(instance);
    furi_assert(settings);

    *settings = instance->settings;
}

void desktop_api_set_settings(Desktop* instance, const DesktopSettings* settings) {
    furi_assert(instance);
    furi_assert(settings);

    instance->settings = *settings;
    view_dispatcher_send_custom_event(instance->view_dispatcher, DesktopGlobalSaveSettings);
}

/*
 * Application thread
 */

int32_t desktop_srv(void* p) {
    UNUSED(p);

    if(furi_hal_rtc_get_boot_mode() != FuriHalRtcBootModeNormal) {
        FURI_LOG_W(TAG, "Skipping start in special boot mode");

        furi_thread_suspend(furi_thread_get_current_id());
        return 0;
    }

    Desktop* desktop = desktop_alloc();

    desktop_init_settings(desktop);

    scene_manager_next_scene(desktop->scene_manager, DesktopSceneMain);

    if(desktop_pin_code_is_set()) {
        desktop_lock(desktop);
    } else {
        CliVcp* cli_vcp = furi_record_open(RECORD_CLI_VCP);
        cli_vcp_enable(cli_vcp);
        furi_record_close(RECORD_CLI_VCP);
    }

    if(storage_file_exists(desktop->storage, SLIDESHOW_FS_PATH)) {
        scene_manager_next_scene(desktop->scene_manager, DesktopSceneSlideshow);
    }

    if(!furi_hal_version_do_i_belong_here()) {
        scene_manager_next_scene(desktop->scene_manager, DesktopSceneHwMismatch);
    }

    if(furi_hal_rtc_get_fault_data()) {
        scene_manager_next_scene(desktop->scene_manager, DesktopSceneFault);
    }

    uint8_t keys_total, keys_valid;
    if(!furi_hal_crypto_enclave_verify(&keys_total, &keys_valid)) {
        FURI_LOG_E(
            TAG,
            "Secure Enclave verification failed: total %hhu, valid %hhu",
            keys_total,
            keys_valid);

        scene_manager_next_scene(desktop->scene_manager, DesktopSceneSecureEnclave);
    }

    // Special case: autostart application is already running
    if(desktop->app_running && animation_manager_is_animation_loaded(desktop->animation_manager)) {
        animation_manager_unload_and_stall_animation(desktop->animation_manager);
    }

    // R0N1N boot splash, unless an autostart application is already on screen
    if(!desktop->app_running) {
        r0n1n_boot_run(desktop->gui);
    }

    view_dispatcher_run(desktop->view_dispatcher);

    // Should never get here (a service thread will crash automatically if it returns)
    return 0;
}
