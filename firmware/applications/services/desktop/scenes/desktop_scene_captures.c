#include <furi.h>
#include <assets_icons.h>

#include "desktop_scene.h"
#include "desktop_scene_r0n1n.h"

// R0N1N Capture Timeline (docs/UX_DESIGN.md): every saved NFC / Sub-GHz / IR
// / RFID / iButton file in one list, newest first. OK opens it in its app,
// hold OK offers to delete it. Scene state: the selected entry.

void desktop_scene_captures_on_enter(void* context) {
    Desktop* desktop = context;
    R0n1nList* list = desktop->r0n1n_list;

    desktop_r0n1n_entries_clear(desktop);
    desktop_r0n1n_scan_captures(desktop, NULL);

    desktop_r0n1n_prepare_list(desktop);
    r0n1n_list_set_title(list, NULL, "Захваты");
    r0n1n_list_set_empty_text(
        list, "Захватов пока нет.\nСохраненные сигналы\nи карты появятся здесь.");
    char time[8];
    for(size_t i = 0; i < desktop->r0n1n_entry_count; i++) {
        const R0n1nEntry* entry = &desktop->r0n1n_entries[i];
        desktop_r0n1n_format_time(entry->timestamp, time, sizeof(time));
        r0n1n_list_add_item(
            list, entry->icon, furi_string_get_cstr(entry->label), time, NULL, NULL, i);
    }
    r0n1n_list_set_selected_item(
        list, scene_manager_get_scene_state(desktop->scene_manager, DesktopSceneCaptures));
    view_dispatcher_switch_to_view(desktop->view_dispatcher, DesktopViewIdR0n1nList);
}

bool desktop_scene_captures_on_event(void* context, SceneManagerEvent event) {
    Desktop* desktop = context;
    if(event.type != SceneManagerEventTypeCustom) return false;

    const uint32_t kind = event.event & R0N1N_EVT_KIND;
    const uint32_t i = event.event & R0N1N_EVT_VALUE;
    if(i >= desktop->r0n1n_entry_count) return false;

    scene_manager_set_scene_state(desktop->scene_manager, DesktopSceneCaptures, i);
    if(kind == R0N1N_EVT_OK) {
        const R0n1nEntry* entry = &desktop->r0n1n_entries[i];
        desktop_r0n1n_launch(
            desktop, furi_string_get_cstr(entry->app), furi_string_get_cstr(entry->path));
        return true;
    } else if(kind == R0N1N_EVT_HOLD) {
        scene_manager_next_scene(desktop->scene_manager, DesktopSceneCaptureDelete);
        return true;
    }
    return false;
}

void desktop_scene_captures_on_exit(void* context) {
    UNUSED(context);
}
