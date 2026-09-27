#include <furi.h>

#include "desktop_scene.h"
#include "desktop_scene_r0n1n.h"

// R0N1N Global Search, step 1 (hold Back on Home, docs/UX_DESIGN.md): type a
// query with the system keyboard; results follow in DesktopSceneSearchResults,
// and Back from there returns here to edit the query.

static void desktop_scene_search_text_callback(void* context) {
    Desktop* desktop = context;
    view_dispatcher_send_custom_event(desktop->view_dispatcher, R0N1N_EVT_OK);
}

void desktop_scene_search_on_enter(void* context) {
    Desktop* desktop = context;
    TextInput* text_input = desktop->text_input;

    text_input_reset(text_input);
    text_input_set_header_text(text_input, "Что найти?");
    text_input_set_result_callback(
        text_input,
        desktop_scene_search_text_callback,
        desktop,
        desktop->search_query,
        R0N1N_QUERY_SIZE,
        false);
    view_dispatcher_switch_to_view(desktop->view_dispatcher, DesktopViewIdTextInput);
}

bool desktop_scene_search_on_event(void* context, SceneManagerEvent event) {
    Desktop* desktop = context;
    if(event.type == SceneManagerEventTypeCustom && event.event == R0N1N_EVT_OK) {
        scene_manager_next_scene(desktop->scene_manager, DesktopSceneSearchResults);
        return true;
    }
    return false;
}

void desktop_scene_search_on_exit(void* context) {
    Desktop* desktop = context;
    text_input_reset(desktop->text_input);
}
