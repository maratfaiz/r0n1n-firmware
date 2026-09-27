#include <furi.h>

#include "desktop_scene.h"
#include "desktop_scene_r0n1n.h"

// One folder of apps (desktop->section), opened from the Applications menu
// or the sections carousel: the section's built-in apps and every app in its
// SD folders, as icon tiles like on a phone; the caption names the selected
// one.

void desktop_scene_section_apps_on_enter(void* context) {
    Desktop* desktop = context;
    const R0n1nSectionInfo* section = &r0n1n_sections[desktop->section];
    R0n1nGrid* grid = desktop->r0n1n_grid;

    desktop_r0n1n_section_entries(desktop, desktop->section, true);

    desktop_r0n1n_prepare_grid(desktop);
    r0n1n_grid_set_title(grid, section->icon, section->title);
    r0n1n_grid_set_layout(grid, 4, 26, 18, 13);
    r0n1n_grid_set_empty_text(
        grid, "В папке пока пусто.\nПриложения ставятся\nчерез qFlipper или телефон.");
    for(uint32_t i = 0; i < desktop->r0n1n_entry_count; i++) {
        const R0n1nEntry* entry = &desktop->r0n1n_entries[i];
        r0n1n_grid_add_item(grid, entry->icon, furi_string_get_cstr(entry->label), false, i);
    }
    r0n1n_grid_set_selected_item(
        grid, scene_manager_get_scene_state(desktop->scene_manager, DesktopSceneSectionApps));
    view_dispatcher_switch_to_view(desktop->view_dispatcher, DesktopViewIdR0n1nGrid);
}

bool desktop_scene_section_apps_on_event(void* context, SceneManagerEvent event) {
    Desktop* desktop = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    if((event.event & R0N1N_EVT_KIND) != R0N1N_EVT_OK) return false;

    const uint32_t i = event.event & R0N1N_EVT_VALUE;
    if(i < desktop->r0n1n_entry_count) {
        scene_manager_set_scene_state(desktop->scene_manager, DesktopSceneSectionApps, i);
        desktop_r0n1n_launch(desktop, furi_string_get_cstr(desktop->r0n1n_entries[i].app), NULL);
    }
    return true;
}

void desktop_scene_section_apps_on_exit(void* context) {
    Desktop* desktop = context;
    scene_manager_set_scene_state(desktop->scene_manager, DesktopSceneSectionApps, 0);
}
