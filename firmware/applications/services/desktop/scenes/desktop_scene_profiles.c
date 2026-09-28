#include <furi.h>
#include <assets_icons.h>

#include "desktop_scene.h"
#include "desktop_scene_r0n1n.h"

// R0N1N profiles: the check marks the active profile, the caption describes
// the selected one. A profile decides which sections Left/Right show on Home.

void desktop_scene_profiles_on_enter(void* context) {
    Desktop* desktop = context;
    R0n1nList* list = desktop->r0n1n_list;

    desktop_r0n1n_prepare_list(desktop);
    r0n1n_list_set_title(list, NULL, "Профиль");
    for(uint32_t i = 0; i < R0n1nProfileCount; i++) {
        r0n1n_list_add_item(
            list,
            r0n1n_profiles[i].icon,
            r0n1n_profiles[i].name,
            NULL,
            i == desktop->r0n1n.profile ? &I_R_Check_7x6 : NULL,
            r0n1n_profiles[i].description,
            i);
    }
    r0n1n_list_set_selected_item(list, desktop->r0n1n.profile);
    view_dispatcher_switch_to_view(desktop->view_dispatcher, DesktopViewIdR0n1nList);
}

bool desktop_scene_profiles_on_event(void* context, SceneManagerEvent event) {
    Desktop* desktop = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    if((event.event & R0N1N_EVT_KIND) != R0N1N_EVT_OK) return false;

    const uint32_t profile = event.event & R0N1N_EVT_VALUE;
    if(profile < R0n1nProfileCount && profile != desktop->r0n1n.profile) {
        desktop->r0n1n.profile = profile;
        r0n1n_settings_save(&desktop->r0n1n);
        // Sections come from the profile: start the carousel from the beginning.
        scene_manager_set_scene_state(desktop->scene_manager, DesktopSceneSections, 0);
    }
    scene_manager_previous_scene(desktop->scene_manager);
    return true;
}

void desktop_scene_profiles_on_exit(void* context) {
    UNUSED(context);
}
