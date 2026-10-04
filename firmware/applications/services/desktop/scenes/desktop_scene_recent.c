#include <furi.h>
#include <assets_icons.h>

#include "desktop_scene.h"
#include "desktop_scene_r0n1n.h"

// R0N1N Recent apps (hold OK on Home, docs/UX_DESIGN.md). Tracking is in
// desktop.c (desktop_recent_apps_push: the name comes from the Loader's
// LoaderEventTypeApplicationBeforeLoad -- see loader.h/loader.c for the
// event.name field this needed -- and is committed with the time on
// LoaderEventTypeApplicationStopped, so only apps that actually ran are
// listed). In-memory only: the list is empty again after a reboot.
// Relaunches start the app without the args it was originally given (see
// docs/ROADMAP.md, Stage 1).

void desktop_scene_recent_on_enter(void* context) {
    Desktop* desktop = context;
    R0n1nList* list = desktop->r0n1n_list;

    desktop_r0n1n_prepare_list(desktop);
    r0n1n_list_set_title(list, &I_R_Clock_9x8, "Недавние");
    r0n1n_list_set_empty_text(list, "Пока пусто.\nЗапущенные приложения\nпоявятся здесь.");

    FuriString* label = furi_string_alloc();
    char age[12];
    for(uint32_t i = 0; i < desktop->recent_apps_count; i++) {
        const char* name = desktop->recent_apps[i];
        const R0n1nApp* app = r0n1n_catalog_find(name);
        if(app) {
            furi_string_set(label, app->label);
        } else {
            desktop_app_display_name(name, label);
        }
        desktop_r0n1n_format_age(desktop->recent_apps_time[i], age, sizeof(age));
        r0n1n_list_add_item(
            list,
            app ? app->icon : &I_unknown_10px,
            furi_string_get_cstr(label),
            age,
            NULL,
            NULL,
            i);
    }
    furi_string_free(label);

    view_dispatcher_switch_to_view(desktop->view_dispatcher, DesktopViewIdR0n1nList);
}

bool desktop_scene_recent_on_event(void* context, SceneManagerEvent event) {
    Desktop* desktop = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    if((event.event & R0N1N_EVT_KIND) != R0N1N_EVT_OK) return false;

    const uint32_t i = event.event & R0N1N_EVT_VALUE;
    if(i < desktop->recent_apps_count) {
        desktop_r0n1n_launch(desktop, desktop->recent_apps[i], NULL);
    }
    return true;
}

void desktop_scene_recent_on_exit(void* context) {
    UNUSED(context);
}
