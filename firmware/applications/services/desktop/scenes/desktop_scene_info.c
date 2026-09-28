#include <furi.h>

#include "desktop_scene.h"
#include "desktop_scene_r0n1n.h"

// Short message (desktop->info_text) that closes itself or on any key.

static void desktop_scene_info_callback(void* context) {
    Desktop* desktop = context;
    view_dispatcher_send_custom_event(desktop->view_dispatcher, R0N1N_EVT_OK);
}

void desktop_scene_info_on_enter(void* context) {
    Desktop* desktop = context;
    Popup* popup = desktop->popup;

    popup_reset(popup);
    popup_set_text(popup, desktop->info_text, 64, 32, AlignCenter, AlignCenter);
    popup_set_context(popup, desktop);
    popup_set_callback(popup, desktop_scene_info_callback);
    popup_set_timeout(popup, 3000);
    popup_enable_timeout(popup);
    view_dispatcher_switch_to_view(desktop->view_dispatcher, DesktopViewIdPopup);
}

bool desktop_scene_info_on_event(void* context, SceneManagerEvent event) {
    Desktop* desktop = context;
    if(event.type == SceneManagerEventTypeCustom && event.event == R0N1N_EVT_OK) {
        scene_manager_previous_scene(desktop->scene_manager);
        return true;
    }
    return false;
}

void desktop_scene_info_on_exit(void* context) {
    Desktop* desktop = context;
    popup_reset(desktop->popup);
}
