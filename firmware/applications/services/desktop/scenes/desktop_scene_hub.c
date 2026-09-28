#include <furi.h>
#include <assets_icons.h>

#include "desktop_scene.h"
#include "desktop_scene_r0n1n.h"

// R0N1N Hub (docs/UX_DESIGN.md, docs/ECOSYSTEM.md): the apps installed on
// the SD card, one tab per /ext/apps category. The Flipper has no network of
// its own, so installing new apps stays with qFlipper / the mobile app (and
// later the R0N1N companion); this screen browses and launches what's there.
// Scene state: the selected tab.

#define R0N1N_HUB_MAX_CATEGORIES 7

static char desktop_hub_categories[R0N1N_HUB_MAX_CATEGORIES][R0N1N_CATEGORY_SIZE];
static const char* desktop_hub_tabs[R0N1N_HUB_MAX_CATEGORIES + 1];

static void desktop_scene_hub_fill(Desktop* desktop, uint8_t tab) {
    R0n1nList* list = desktop->r0n1n_list;

    desktop_r0n1n_entries_clear(desktop);
    const size_t categories = desktop_r0n1n_scan_apps(
        desktop,
        tab ? desktop_hub_categories[tab - 1] : NULL,
        NULL,
        desktop_hub_categories,
        R0N1N_HUB_MAX_CATEGORIES);

    desktop_r0n1n_prepare_list(desktop);
    r0n1n_list_set_title(list, &I_R_Download_8x8, "R0N1N Hub");
    r0n1n_list_set_empty_text(
        list, "Приложений нет.\nУстановите их через qFlipper\nили мобильное приложение.");
    desktop_hub_tabs[0] = "Все";
    for(size_t i = 0; i < categories; i++) {
        desktop_hub_tabs[i + 1] = desktop_hub_categories[i];
    }
    r0n1n_list_set_tabs(list, desktop_hub_tabs, categories + 1, tab);
    for(size_t i = 0; i < desktop->r0n1n_entry_count; i++) {
        const R0n1nEntry* entry = &desktop->r0n1n_entries[i];
        r0n1n_list_add_item(
            list, entry->icon, furi_string_get_cstr(entry->label), NULL, &I_R_Check_7x6, NULL, i);
    }
}

void desktop_scene_hub_on_enter(void* context) {
    Desktop* desktop = context;
    desktop_scene_hub_fill(
        desktop, scene_manager_get_scene_state(desktop->scene_manager, DesktopSceneHub));
    view_dispatcher_switch_to_view(desktop->view_dispatcher, DesktopViewIdR0n1nList);
}

bool desktop_scene_hub_on_event(void* context, SceneManagerEvent event) {
    Desktop* desktop = context;
    if(event.type != SceneManagerEventTypeCustom) return false;

    const uint32_t kind = event.event & R0N1N_EVT_KIND;
    const uint32_t value = event.event & R0N1N_EVT_VALUE;
    if(kind == R0N1N_EVT_TAB) {
        scene_manager_set_scene_state(desktop->scene_manager, DesktopSceneHub, value);
        desktop_scene_hub_fill(desktop, value);
        return true;
    } else if(kind == R0N1N_EVT_OK && value < desktop->r0n1n_entry_count) {
        desktop_r0n1n_launch(
            desktop, furi_string_get_cstr(desktop->r0n1n_entries[value].app), NULL);
        return true;
    }
    return false;
}

void desktop_scene_hub_on_exit(void* context) {
    Desktop* desktop = context;
    scene_manager_set_scene_state(desktop->scene_manager, DesktopSceneHub, 0);
}
