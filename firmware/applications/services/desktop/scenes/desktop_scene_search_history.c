#include <furi.h>
#include <assets_icons.h>

#include "desktop_scene.h"
#include "desktop_scene_r0n1n.h"

// R0N1N Global Search, step 0: "Новый поиск" opens the keyboard, the rows
// below repeat one of the last searches (desktop_r0n1n_history_push). Only
// shown when there is a history (desktop_r0n1n_open_search).

#define R0N1N_SEARCH_NEW 0xFFFF

void desktop_scene_search_history_on_enter(void* context) {
    Desktop* desktop = context;
    R0n1nList* list = desktop->r0n1n_list;

    desktop_r0n1n_prepare_list(desktop);
    r0n1n_list_set_title(list, &I_R_Search_8x8, "Поиск");
    r0n1n_list_add_item(list, &I_R_Search_8x8, "Новый поиск", NULL, NULL, NULL, R0N1N_SEARCH_NEW);
    for(uint8_t i = 0; i < desktop->search_history_count; i++) {
        r0n1n_list_add_item(list, &I_R_Clock_9x8, desktop->search_history[i], NULL, NULL, NULL, i);
    }
    view_dispatcher_switch_to_view(desktop->view_dispatcher, DesktopViewIdR0n1nList);
}

bool desktop_scene_search_history_on_event(void* context, SceneManagerEvent event) {
    Desktop* desktop = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    if((event.event & R0N1N_EVT_KIND) != R0N1N_EVT_OK) return false;

    const uint32_t item = event.event & R0N1N_EVT_VALUE;
    if(item == R0N1N_SEARCH_NEW) {
        desktop->search_query[0] = 0;
        scene_manager_next_scene(desktop->scene_manager, DesktopSceneSearch);
    } else if(item < desktop->search_history_count) {
        strlcpy(desktop->search_query, desktop->search_history[item], R0N1N_QUERY_SIZE);
        scene_manager_next_scene(desktop->scene_manager, DesktopSceneSearchResults);
    }
    return true;
}

void desktop_scene_search_history_on_exit(void* context) {
    UNUSED(context);
}
