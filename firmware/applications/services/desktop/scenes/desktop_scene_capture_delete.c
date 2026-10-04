#include <furi.h>
#include <storage/storage.h>

#include "desktop_scene.h"
#include "desktop_scene_r0n1n.h"

// Confirm deleting the capture selected in DesktopSceneCaptures.

static void desktop_scene_capture_delete_callback(DialogExResult result, void* context) {
    Desktop* desktop = context;
    view_dispatcher_send_custom_event(desktop->view_dispatcher, R0N1N_EVT_OK | result);
}

static const R0n1nEntry* desktop_scene_capture_delete_entry(Desktop* desktop) {
    const uint32_t i = scene_manager_get_scene_state(desktop->scene_manager, DesktopSceneCaptures);
    return i < desktop->r0n1n_entry_count ? &desktop->r0n1n_entries[i] : NULL;
}

void desktop_scene_capture_delete_on_enter(void* context) {
    Desktop* desktop = context;
    DialogEx* dialog = desktop->dialog_ex;
    const R0n1nEntry* entry = desktop_scene_capture_delete_entry(desktop);

    dialog_ex_reset(dialog);
    dialog_ex_set_header(dialog, "Удалить захват?", 64, 2, AlignCenter, AlignTop);
    if(entry) {
        dialog_ex_set_text(
            dialog, furi_string_get_cstr(entry->label), 64, 26, AlignCenter, AlignCenter);
    }
    dialog_ex_set_left_button_text(dialog, "Отмена");
    dialog_ex_set_right_button_text(dialog, "Удалить");
    dialog_ex_set_context(dialog, desktop);
    dialog_ex_set_result_callback(dialog, desktop_scene_capture_delete_callback);
    view_dispatcher_switch_to_view(desktop->view_dispatcher, DesktopViewIdDialog);
}

bool desktop_scene_capture_delete_on_event(void* context, SceneManagerEvent event) {
    Desktop* desktop = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    if((event.event & R0N1N_EVT_KIND) != R0N1N_EVT_OK) return false;

    if((event.event & R0N1N_EVT_VALUE) == DialogExResultRight) {
        const R0n1nEntry* entry = desktop_scene_capture_delete_entry(desktop);
        if(entry) storage_simply_remove(desktop->storage, furi_string_get_cstr(entry->path));
    }
    scene_manager_previous_scene(desktop->scene_manager);
    return true;
}

void desktop_scene_capture_delete_on_exit(void* context) {
    Desktop* desktop = context;
    dialog_ex_reset(desktop->dialog_ex);
}
