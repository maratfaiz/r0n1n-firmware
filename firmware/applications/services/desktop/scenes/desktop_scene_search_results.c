#include <furi.h>
#include <applications.h>
#include <assets_icons.h>

#include "desktop_scene.h"
#include "desktop_scene_r0n1n.h"

// R0N1N Global Search, step 2: apps (built-in and on SD), settings and saved
// captures whose names contain the query. Apps and settings come first (they
// get timestamps above any real file time), then files, newest first.

static uint32_t desktop_search_rank;

static void desktop_search_add_app(Desktop* desktop, const R0n1nApp* app) {
    if(desktop_r0n1n_matches(app->label, desktop->search_query) ||
       desktop_r0n1n_matches(app->name, desktop->search_query)) {
        desktop_r0n1n_entries_add(
            desktop, app->name, NULL, app->label, app->icon, desktop_search_rank--);
    }
}

void desktop_scene_search_results_on_enter(void* context) {
    Desktop* desktop = context;
    R0n1nList* list = desktop->r0n1n_list;
    const char* query = desktop->search_query;

    desktop_r0n1n_entries_clear(desktop);
    desktop_search_rank = UINT32_MAX;
    for(size_t s = 0; s < R0n1nSectionCount; s++) {
        for(size_t i = 0; i < r0n1n_sections[s].app_count; i++) {
            desktop_search_add_app(desktop, &r0n1n_sections[s].apps[i]);
        }
    }
    for(size_t i = 0; i < r0n1n_system_apps_count; i++) {
        desktop_search_add_app(desktop, &r0n1n_system_apps[i]);
    }
    for(size_t i = 0; i < FLIPPER_SETTINGS_APPS_COUNT; i++) {
        const char* name = FLIPPER_SETTINGS_APPS[i].name;
        const char* label = r0n1n_catalog_settings_label(name);
        if(desktop_r0n1n_matches(label, query) || desktop_r0n1n_matches(name, query)) {
            desktop_r0n1n_entries_add(
                desktop,
                name,
                NULL,
                label,
                r0n1n_catalog_settings_icon(name),
                desktop_search_rank--);
        }
    }
    desktop_r0n1n_scan_apps(desktop, NULL, query, NULL, 0);
    desktop_r0n1n_scan_captures(desktop, query);

    desktop_r0n1n_prepare_list(desktop);
    r0n1n_list_set_title(list, NULL, "Поиск");
    r0n1n_list_set_query(list, query);
    r0n1n_list_set_empty_text(list, "Ничего не найдено");
    for(size_t i = 0; i < desktop->r0n1n_entry_count; i++) {
        const R0n1nEntry* entry = &desktop->r0n1n_entries[i];
        r0n1n_list_add_item(
            list, entry->icon, furi_string_get_cstr(entry->label), NULL, NULL, NULL, i);
    }
    view_dispatcher_switch_to_view(desktop->view_dispatcher, DesktopViewIdR0n1nList);
}

bool desktop_scene_search_results_on_event(void* context, SceneManagerEvent event) {
    Desktop* desktop = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    if((event.event & R0N1N_EVT_KIND) != R0N1N_EVT_OK) return false;

    const uint32_t i = event.event & R0N1N_EVT_VALUE;
    if(i < desktop->r0n1n_entry_count) {
        const R0n1nEntry* entry = &desktop->r0n1n_entries[i];
        desktop_r0n1n_launch(
            desktop, furi_string_get_cstr(entry->app), furi_string_get_cstr(entry->path));
    }
    return true;
}

void desktop_scene_search_results_on_exit(void* context) {
    UNUSED(context);
}
