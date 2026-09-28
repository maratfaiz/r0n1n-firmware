#include <applications.h>
#include <lib/toolbox/value_index.h>

#include "../desktop_settings_app.h"
#include "desktop_settings_scene.h"

typedef enum {
    DesktopSettingsPinSetup = 0,
    DesktopSettingsAutoLockDelay,
    DesktopSettingsClockDisplay,
    DesktopSettingsFavoriteApps,
    DesktopSettingsHappyMode,
} DesktopSettingsEntry;

#define AUTO_LOCK_DELAY_COUNT 6
static const char* const auto_lock_delay_text[AUTO_LOCK_DELAY_COUNT] = {
    "ВЫКЛ",
    "30s",
    "60s",
    "2мин",
    "5мин",
    "10мин",
};
static const uint32_t auto_lock_delay_value[AUTO_LOCK_DELAY_COUNT] =
    {0, 30000, 60000, 120000, 300000, 600000};

#define CLOCK_ENABLE_COUNT 2
const char* const clock_enable_text[CLOCK_ENABLE_COUNT] = {
    "ВЫКЛ",
    "ВКЛ",
};

const uint32_t clock_enable_value[CLOCK_ENABLE_COUNT] = {0, 1};

static void desktop_settings_scene_start_var_list_enter_callback(void* context, uint32_t index) {
    DesktopSettingsApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

static void desktop_settings_scene_start_clock_enable_changed(VariableItem* item) {
    DesktopSettingsApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);

    variable_item_set_current_value_text(item, clock_enable_text[index]);
    app->settings.display_clock = index;
}

static void desktop_settings_scene_start_auto_lock_delay_changed(VariableItem* item) {
    DesktopSettingsApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);

    variable_item_set_current_value_text(item, auto_lock_delay_text[index]);
    app->settings.auto_lock_delay_ms = auto_lock_delay_value[index];
}

void desktop_settings_scene_start_on_enter(void* context) {
    DesktopSettingsApp* app = context;
    VariableItemList* variable_item_list = app->variable_item_list;

    VariableItem* item;
    uint8_t value_index;

    variable_item_list_add(variable_item_list, "PIN-код", 1, NULL, NULL);

    item = variable_item_list_add(
        variable_item_list,
        "Автоблок.",
        AUTO_LOCK_DELAY_COUNT,
        desktop_settings_scene_start_auto_lock_delay_changed,
        app);

    value_index = value_index_uint32(
        app->settings.auto_lock_delay_ms, auto_lock_delay_value, AUTO_LOCK_DELAY_COUNT);
    variable_item_set_current_value_index(item, value_index);
    variable_item_set_current_value_text(item, auto_lock_delay_text[value_index]);

    item = variable_item_list_add(
        variable_item_list,
        "Часы в строке",
        CLOCK_ENABLE_COUNT,
        desktop_settings_scene_start_clock_enable_changed,
        app);

    value_index =
        value_index_uint32(app->settings.display_clock, clock_enable_value, CLOCK_ENABLE_COUNT);
    variable_item_set_current_value_index(item, value_index);
    variable_item_set_current_value_text(item, clock_enable_text[value_index]);

    variable_item_list_add(variable_item_list, "Быстрый запуск", 1, NULL, NULL);

    variable_item_list_add(variable_item_list, "Всегда рад", 1, NULL, NULL);

    variable_item_list_set_enter_callback(
        variable_item_list, desktop_settings_scene_start_var_list_enter_callback, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, DesktopSettingsAppViewVarItemList);
}

bool desktop_settings_scene_start_on_event(void* context, SceneManagerEvent event) {
    DesktopSettingsApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        switch(event.event) {
        case DesktopSettingsPinSetup:
            scene_manager_next_scene(app->scene_manager, DesktopSettingsAppScenePinMenu);
            break;

        case DesktopSettingsFavoriteApps:
            scene_manager_next_scene(app->scene_manager, DesktopSettingsAppSceneQuickAppsMenu);
            break;

        case DesktopSettingsHappyMode:
            scene_manager_next_scene(app->scene_manager, DesktopSettingsAppSceneHappyMode);
            break;

        default:
            break;
        }
        consumed = true;
    }
    return consumed;
}

void desktop_settings_scene_start_on_exit(void* context) {
    DesktopSettingsApp* app = context;
    variable_item_list_reset(app->variable_item_list);
}
