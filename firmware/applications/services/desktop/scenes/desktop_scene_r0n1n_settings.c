#include <furi.h>
#include <applications.h>
#include <assets_icons.h>

#include "desktop_scene.h"
#include "desktop_scene_r0n1n.h"

// R0N1N Settings: the R0N1N profile, then the stock settings apps with
// Russian labels. Each stock entry launches the unchanged settings app.

#define R0N1N_SETTINGS_PROFILE   0xFFFF
#define R0N1N_SETTINGS_SIMPLE    0xFFFE
#define R0N1N_SETTINGS_FEEDBACK  0xFFFD
#define R0N1N_SETTINGS_WALLPAPER 0xFFFC

void desktop_scene_r0n1n_settings_on_enter(void* context) {
    Desktop* desktop = context;
    R0n1nList* list = desktop->r0n1n_list;

    desktop_r0n1n_prepare_list(desktop);
    r0n1n_list_set_title(list, &I_R_Gear_9x7, "Настройки");
    r0n1n_list_add_item(
        list, &I_R_Star_9x7, "Простой режим", "выкл", NULL, NULL, R0N1N_SETTINGS_SIMPLE);
    r0n1n_list_add_item(
        list,
        &I_R_Sound_9x8,
        "Отклик (звук/вибро)",
        desktop->r0n1n.feedback ? "вкл" : "выкл",
        NULL,
        NULL,
        R0N1N_SETTINGS_FEEDBACK);
    r0n1n_list_add_item(
        list,
        &I_R_Star_9x7,
        "Фон",
        r0n1n_wallpapers
            [desktop->r0n1n.wallpaper < r0n1n_wallpapers_count ? desktop->r0n1n.wallpaper : 0]
                .name,
        NULL,
        NULL,
        R0N1N_SETTINGS_WALLPAPER);
    r0n1n_list_add_item(
        list,
        &I_R_Profile_7x7,
        "Профиль",
        r0n1n_profiles[desktop->r0n1n.profile].name,
        NULL,
        NULL,
        R0N1N_SETTINGS_PROFILE);
    for(uint32_t i = 0; i < FLIPPER_SETTINGS_APPS_COUNT; i++) {
        const char* name = FLIPPER_SETTINGS_APPS[i].name;
        r0n1n_list_add_item(
            list,
            r0n1n_catalog_settings_icon(name),
            r0n1n_catalog_settings_label(name),
            NULL,
            NULL,
            NULL,
            i);
    }

    r0n1n_list_set_selected_item(
        list, scene_manager_get_scene_state(desktop->scene_manager, DesktopSceneR0n1nSettings));
    view_dispatcher_switch_to_view(desktop->view_dispatcher, DesktopViewIdR0n1nList);
}

bool desktop_scene_r0n1n_settings_on_event(void* context, SceneManagerEvent event) {
    Desktop* desktop = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    if((event.event & R0N1N_EVT_KIND) != R0N1N_EVT_OK) return false;

    const uint32_t item = event.event & R0N1N_EVT_VALUE;
    scene_manager_set_scene_state(desktop->scene_manager, DesktopSceneR0n1nSettings, item);
    if(item == R0N1N_SETTINGS_WALLPAPER) {
        // Cycle to the next Home picture
        desktop->r0n1n.wallpaper = (desktop->r0n1n.wallpaper + 1) % r0n1n_wallpapers_count;
        r0n1n_settings_save(&desktop->r0n1n);
        desktop_r0n1n_feedback(desktop, R0n1nFeedbackClick);
        scene_manager_set_scene_state(
            desktop->scene_manager, DesktopSceneR0n1nSettings, R0N1N_SETTINGS_WALLPAPER);
        scene_manager_previous_scene(desktop->scene_manager);
        scene_manager_next_scene(desktop->scene_manager, DesktopSceneR0n1nSettings);
    } else if(item == R0N1N_SETTINGS_FEEDBACK) {
        desktop->r0n1n.feedback = !desktop->r0n1n.feedback;
        r0n1n_settings_save(&desktop->r0n1n);
        desktop_r0n1n_feedback(desktop, R0n1nFeedbackClick); // hear the new state
        scene_manager_previous_scene(desktop->scene_manager);
        scene_manager_next_scene(desktop->scene_manager, DesktopSceneR0n1nSettings);
    } else if(item == R0N1N_SETTINGS_SIMPLE) {
        // Simple mode: big Home and menu, basic functions only. Leaving it is
        // the last item of its own menu.
        desktop->r0n1n.simple_mode = true;
        r0n1n_settings_save(&desktop->r0n1n);
        desktop_main_set_simple_mode(desktop->main_view, true);
        scene_manager_set_scene_state(desktop->scene_manager, DesktopSceneR0n1nSettings, 0);
        scene_manager_search_and_switch_to_previous_scene(
            desktop->scene_manager, DesktopSceneMain);
    } else if(item == R0N1N_SETTINGS_PROFILE) {
        scene_manager_next_scene(desktop->scene_manager, DesktopSceneProfiles);
    } else if(item < FLIPPER_SETTINGS_APPS_COUNT) {
        desktop_r0n1n_launch(desktop, FLIPPER_SETTINGS_APPS[item].name, NULL);
    }
    return true;
}

void desktop_scene_r0n1n_settings_on_exit(void* context) {
    UNUSED(context);
}
