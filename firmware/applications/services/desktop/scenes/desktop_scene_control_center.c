#include <furi.h>
#include <furi_hal_power.h>
#include <furi_hal_rtc.h>
#include <assets_icons.h>
#include <bt/bt_service/bt_settings_api_i.h>
#include <notification/notification_messages.h>

#include "desktop_scene.h"
#include "desktop_scene_r0n1n.h"

// R0N1N Control Center (Down on Home, docs/UX_DESIGN.md): a grid of toggles
// and shortcuts plus a brightness slider. The selected tile's name and state
// are shown in the header; a small mark in a tile's corner = on. Sound,
// Backlight and Auto-lock step through values on each OK instead of
// switching on/off, so they can be set without opening Settings.

typedef enum {
    CcBluetooth,
    CcSound,
    CcVibro,
    CcStealth,
    CcBacklight,
    CcAutoLock,
    CcLock,
    CcPower,
    CcProfile,
    CcSettings,
    CcCount,
} CcItem;

#define CC_MIN_BRIGHTNESS 10

// Steps for the cycling tiles (the same values the settings apps offer)
static const float cc_volume_steps[] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
static const uint32_t cc_backlight_steps[] = {5000, 15000, 30000, 60000, 120000, 300000};
static const uint32_t cc_autolock_steps[] = {0, 30000, 60000, 120000, 300000, 600000};

static void desktop_cc_format_ms(uint32_t ms, char* out, size_t size) {
    if(!ms) {
        strlcpy(out, "выкл", size);
    } else if(ms < 60000) {
        snprintf(out, size, "%lu с", ms / 1000);
    } else {
        snprintf(out, size, "%lu мин", ms / 60000);
    }
}

// The step after `current` in `steps`, wrapping round
static uint32_t desktop_cc_next_ms(uint32_t current, const uint32_t* steps, size_t count) {
    for(size_t i = 0; i < count; i++) {
        if(steps[i] > current) return steps[i];
    }
    return steps[0];
}

static bool desktop_cc_bt_enabled(void) {
    Bt* bt = furi_record_open(RECORD_BT);
    BtSettings settings;
    bt_get_settings(bt, &settings);
    furi_record_close(RECORD_BT);
    return settings.enabled;
}

static void desktop_cc_bt_set(bool enabled) {
    Bt* bt = furi_record_open(RECORD_BT);
    BtSettings settings;
    bt_get_settings(bt, &settings);
    settings.enabled = enabled;
    bt_set_settings(bt, &settings);
    furi_record_close(RECORD_BT);
}

static bool desktop_cc_state(Desktop* desktop, CcItem item) {
    switch(item) {
    case CcBluetooth:
        return desktop_cc_bt_enabled();
    case CcSound:
        return desktop->notification->settings.speaker_volume > 0.0f;
    case CcVibro:
        return desktop->notification->settings.vibro_on;
    case CcStealth:
        return furi_hal_rtc_is_flag_set(FuriHalRtcFlagStealthMode);
    case CcAutoLock:
        return desktop->settings.auto_lock_delay_ms > 0;
    default:
        return false;
    }
}

static void desktop_cc_caption(Desktop* desktop, CcItem item, char* out, size_t size) {
    static const char* const names[CcCount] = {
        [CcBluetooth] = "Bluetooth",
        [CcSound] = "Звук",
        [CcVibro] = "Вибро",
        [CcStealth] = "Тихий режим",
        [CcBacklight] = "Подсветка",
        [CcAutoLock] = "Автоблок",
        [CcLock] = "Заблокировать",
        [CcPower] = "Питание",
        [CcProfile] = "Профиль",
        [CcSettings] = "Настройки",
    };
    char value[16];
    if(item == CcSound) {
        const float volume = desktop->notification->settings.speaker_volume;
        if(volume > 0.0f) {
            snprintf(value, sizeof(value), "%u%%", (unsigned)(volume * 100.0f + 0.5f));
        } else {
            strlcpy(value, "выкл", sizeof(value));
        }
        snprintf(out, size, "%s: %s", names[item], value);
    } else if(item == CcBacklight) {
        desktop_cc_format_ms(
            desktop->notification->settings.display_off_delay_ms, value, sizeof(value));
        snprintf(out, size, "%s: %s", names[item], value);
    } else if(item == CcAutoLock) {
        desktop_cc_format_ms(desktop->settings.auto_lock_delay_ms, value, sizeof(value));
        snprintf(out, size, "%s: %s", names[item], value);
    } else if(item <= CcStealth) {
        snprintf(
            out, size, "%s: %s", names[item], desktop_cc_state(desktop, item) ? "вкл" : "выкл");
    } else if(item == CcProfile) {
        snprintf(out, size, "%s: %s", names[item], r0n1n_profiles[desktop->r0n1n.profile].name);
    } else {
        strlcpy(out, names[item], size);
    }
}

static void desktop_cc_refresh(Desktop* desktop, CcItem item) {
    char caption[40];
    desktop_cc_caption(desktop, item, caption, sizeof(caption));
    r0n1n_grid_update_item(
        desktop->r0n1n_grid, item, caption, desktop_cc_state(desktop, item));
}

void desktop_scene_control_center_on_enter(void* context) {
    Desktop* desktop = context;
    R0n1nGrid* grid = desktop->r0n1n_grid;
    static const Icon* const icons[CcCount] = {
        [CcBluetooth] = &I_R_Bluetooth_7x9,
        [CcSound] = &I_R_Sound_9x8,
        [CcVibro] = &I_R_Vibro_9x7,
        [CcStealth] = &I_R_Stealth_9x7,
        [CcBacklight] = &I_R_Display_9x6,
        [CcAutoLock] = &I_R_Clock_9x8,
        [CcLock] = &I_R_Lock_7x7,
        [CcPower] = &I_R_Power_9x8,
        [CcProfile] = &I_R_Profile_7x7,
        [CcSettings] = &I_R_Gear_9x7,
    };

    desktop_r0n1n_prepare_grid(desktop);
    r0n1n_grid_set_layout(grid, 4, 30, 16, 13);
    r0n1n_grid_set_caption_in_header(grid, true, furi_hal_power_get_pct());
    for(uint32_t i = 0; i < CcCount; i++) {
        r0n1n_grid_add_item(grid, icons[i], NULL, false, i);
        desktop_cc_refresh(desktop, i);
    }
    const float brightness = desktop->notification->settings.display_brightness;
    r0n1n_grid_set_slider(
        grid, &I_R_Brightness_9x9, "Яркость", (uint8_t)(brightness * 100.0f + 0.5f));
    r0n1n_grid_set_selected_item(
        grid, scene_manager_get_scene_state(desktop->scene_manager, DesktopSceneControlCenter));

    view_dispatcher_switch_to_view(desktop->view_dispatcher, DesktopViewIdR0n1nGrid);
}

bool desktop_scene_control_center_on_event(void* context, SceneManagerEvent event) {
    Desktop* desktop = context;
    if(event.type != SceneManagerEventTypeCustom) return false;

    const uint32_t kind = event.event & R0N1N_EVT_KIND;
    const uint32_t value = event.event & R0N1N_EVT_VALUE;
    NotificationSettings* settings = &desktop->notification->settings;

    if(kind == R0N1N_EVT_SLIDER) {
        settings->display_brightness = MAX(value, (uint32_t)CC_MIN_BRIGHTNESS) / 100.0f;
        notification_message(desktop->notification, &sequence_display_backlight_on);
        notification_message_save_settings(desktop->notification);
        return true;
    }
    if(kind != R0N1N_EVT_OK || value >= CcCount) return false;

    scene_manager_set_scene_state(desktop->scene_manager, DesktopSceneControlCenter, value);
    switch(value) {
    case CcBluetooth:
        desktop_cc_bt_set(!desktop_cc_bt_enabled());
        break;
    case CcSound: {
        // Off -> 25% -> 50% -> 75% -> 100% -> off
        float next = cc_volume_steps[0];
        for(size_t i = 0; i < COUNT_OF(cc_volume_steps); i++) {
            if(cc_volume_steps[i] > settings->speaker_volume + 0.01f) {
                next = cc_volume_steps[i];
                break;
            }
        }
        settings->speaker_volume = next;
        notification_message_save_settings(desktop->notification);
        break;
    }
    case CcBacklight:
        settings->display_off_delay_ms = desktop_cc_next_ms(
            settings->display_off_delay_ms, cc_backlight_steps, COUNT_OF(cc_backlight_steps));
        notification_message(desktop->notification, &sequence_display_backlight_on);
        notification_message_save_settings(desktop->notification);
        break;
    case CcAutoLock:
        desktop->settings.auto_lock_delay_ms = desktop_cc_next_ms(
            desktop->settings.auto_lock_delay_ms, cc_autolock_steps, COUNT_OF(cc_autolock_steps));
        // Saved and applied the way the Desktop settings app does it
        view_dispatcher_send_custom_event(desktop->view_dispatcher, DesktopGlobalSaveSettings);
        break;
    case CcVibro:
        settings->vibro_on = !settings->vibro_on;
        notification_message_save_settings(desktop->notification);
        break;
    case CcStealth:
        desktop_set_stealth_mode_state(
            desktop, !furi_hal_rtc_is_flag_set(FuriHalRtcFlagStealthMode));
        break;
    case CcLock:
        scene_manager_set_scene_state(desktop->scene_manager, DesktopSceneControlCenter, 0);
        desktop_lock(desktop);
        return true;
    case CcPower:
        loader_start_detached_with_gui_error(desktop->loader, "Power", "off");
        return true;
    case CcProfile:
        scene_manager_next_scene(desktop->scene_manager, DesktopSceneProfiles);
        return true;
    case CcSettings:
        scene_manager_next_scene(desktop->scene_manager, DesktopSceneR0n1nSettings);
        return true;
    }
    desktop_cc_refresh(desktop, value);
    return true;
}

void desktop_scene_control_center_on_exit(void* context) {
    UNUSED(context);
}
