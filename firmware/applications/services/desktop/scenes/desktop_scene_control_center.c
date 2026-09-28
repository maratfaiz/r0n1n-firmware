#include <furi.h>
#include <furi_hal_power.h>
#include <furi_hal_rtc.h>
#include <assets_icons.h>
#include <bt/bt_service/bt_settings_api_i.h>
#include <notification/notification_messages.h>

#include "desktop_scene.h"
#include "desktop_scene_r0n1n.h"

// R0N1N Control Center (Down on Home, docs/UX_DESIGN.md): a 4x2 grid of
// toggles and shortcuts plus a brightness slider. The selected tile's name
// and state are shown in the header; a small mark in a tile's corner = on.

typedef enum {
    CcBluetooth,
    CcSound,
    CcVibro,
    CcStealth,
    CcLock,
    CcPower,
    CcProfile,
    CcSettings,
    CcCount,
} CcItem;

#define CC_MIN_BRIGHTNESS 10

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
        [CcLock] = "Заблокировать",
        [CcPower] = "Питание",
        [CcProfile] = "Профиль",
        [CcSettings] = "Настройки",
    };
    if(item <= CcStealth) {
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
        desktop->r0n1n_grid, item, caption, item <= CcStealth && desktop_cc_state(desktop, item));
}

void desktop_scene_control_center_on_enter(void* context) {
    Desktop* desktop = context;
    R0n1nGrid* grid = desktop->r0n1n_grid;
    static const Icon* const icons[CcCount] = {
        [CcBluetooth] = &I_R_Bluetooth_7x9,
        [CcSound] = &I_R_Sound_9x8,
        [CcVibro] = &I_R_Vibro_9x7,
        [CcStealth] = &I_R_Stealth_9x7,
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
    case CcSound:
        if(settings->speaker_volume > 0.0f) {
            desktop->cc_saved_volume = settings->speaker_volume;
            settings->speaker_volume = 0.0f;
        } else {
            settings->speaker_volume = desktop->cc_saved_volume > 0.0f ? desktop->cc_saved_volume :
                                                                         1.0f;
        }
        notification_message_save_settings(desktop->notification);
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
