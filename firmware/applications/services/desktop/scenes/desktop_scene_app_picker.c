#include <furi.h>
#include <assets_icons.h>

#include "desktop_scene.h"
#include "desktop_scene_r0n1n.h"

// Pick the app for a Quick Actions slot (hold OK on a tile in Favorites).
// Item index: section << 8 | app, or R0N1N_PICKER_SYSTEM | system app.

#define R0N1N_PICKER_SYSTEM 0xFF00

static const R0n1nApp* desktop_scene_app_picker_get(uint32_t index) {
    if((index & 0xFF00) == R0N1N_PICKER_SYSTEM) {
        const uint32_t i = index & 0xFF;
        return i < r0n1n_system_apps_count ? &r0n1n_system_apps[i] : NULL;
    }
    const uint32_t section = index >> 8;
    const uint32_t app = index & 0xFF;
    if(section >= R0n1nSectionCount || app >= r0n1n_sections[section].app_count) return NULL;
    return &r0n1n_sections[section].apps[app];
}

void desktop_scene_app_picker_on_enter(void* context) {
    Desktop* desktop = context;
    R0n1nList* list = desktop->r0n1n_list;

    desktop_r0n1n_prepare_list(desktop);
    char title[24];
    snprintf(title, sizeof(title), "Слот %u", desktop->picker_slot + 1);
    r0n1n_list_set_title(list, NULL, title);

    for(uint32_t s = 0; s < R0n1nSectionCount; s++) {
        for(uint32_t i = 0; i < r0n1n_sections[s].app_count; i++) {
            const R0n1nApp* app = &r0n1n_sections[s].apps[i];
            const bool current = strcmp(app->name, desktop->r0n1n.quick[desktop->picker_slot]) ==
                                 0;
            r0n1n_list_add_item(
                list,
                app->icon,
                app->label,
                NULL,
                current ? &I_R_Check_7x6 : NULL,
                NULL,
                (s << 8) | i);
        }
    }
    for(uint32_t i = 0; i < r0n1n_system_apps_count; i++) {
        const R0n1nApp* app = &r0n1n_system_apps[i];
        const bool current = strcmp(app->name, desktop->r0n1n.quick[desktop->picker_slot]) == 0;
        r0n1n_list_add_item(
            list,
            app->icon,
            app->label,
            NULL,
            current ? &I_R_Check_7x6 : NULL,
            NULL,
            R0N1N_PICKER_SYSTEM | i);
    }

    view_dispatcher_switch_to_view(desktop->view_dispatcher, DesktopViewIdR0n1nList);
}

bool desktop_scene_app_picker_on_event(void* context, SceneManagerEvent event) {
    Desktop* desktop = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    if((event.event & R0N1N_EVT_KIND) != R0N1N_EVT_OK) return false;

    const R0n1nApp* app = desktop_scene_app_picker_get(event.event & R0N1N_EVT_VALUE);
    if(app) {
        strlcpy(desktop->r0n1n.quick[desktop->picker_slot], app->name, R0N1N_SLOT_NAME_SIZE);
        r0n1n_settings_save(&desktop->r0n1n);
    }
    scene_manager_previous_scene(desktop->scene_manager);
    return true;
}

void desktop_scene_app_picker_on_exit(void* context) {
    UNUSED(context);
}
