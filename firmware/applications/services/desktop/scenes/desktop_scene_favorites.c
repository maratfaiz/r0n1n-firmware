#include <furi.h>
#include <assets_icons.h>

#include "desktop_scene.h"
#include "desktop_scene_r0n1n.h"

// R0N1N Quick Actions (Up on Home, docs/UX_DESIGN.md): six configurable
// slots as a 3x2 grid of 14 px icons; the selected slot's name is the caption.
// Hold OK on a tile to reassign it (DesktopSceneAppPicker).

static const char* const desktop_quick_not_installed =
    "Приложения нет на SD.\nУдерживайте OK на плитке,\nчтобы выбрать другое.";

void desktop_scene_favorites_on_enter(void* context) {
    Desktop* desktop = context;
    R0n1nGrid* grid = desktop->r0n1n_grid;

    desktop_r0n1n_prepare_grid(desktop);
    r0n1n_grid_set_title(grid, &I_R_Star_9x7, "Избранное");
    r0n1n_grid_set_layout(grid, 3, 41, 19, 12);

    FuriString* label = furi_string_alloc();
    for(uint32_t i = 0; i < R0N1N_QUICK_SLOTS; i++) {
        const char* target = desktop->r0n1n.quick[i];
        const R0n1nApp* app = r0n1n_catalog_find(target);
        if(app) {
            r0n1n_grid_add_item(grid, app->tile_icon, app->label, false, i);
        } else {
            desktop_app_display_name(target, label);
            r0n1n_grid_add_item(grid, &A_Plugins_14, furi_string_get_cstr(label), false, i);
        }
    }
    furi_string_free(label);

    r0n1n_grid_set_selected_item(
        grid, scene_manager_get_scene_state(desktop->scene_manager, DesktopSceneFavorites));
    view_dispatcher_switch_to_view(desktop->view_dispatcher, DesktopViewIdR0n1nGrid);
}

bool desktop_scene_favorites_on_event(void* context, SceneManagerEvent event) {
    Desktop* desktop = context;
    if(event.type != SceneManagerEventTypeCustom) return false;

    const uint32_t kind = event.event & R0N1N_EVT_KIND;
    const uint32_t slot = event.event & R0N1N_EVT_VALUE;
    if(slot >= R0N1N_QUICK_SLOTS) return false;

    scene_manager_set_scene_state(desktop->scene_manager, DesktopSceneFavorites, slot);
    if(kind == R0N1N_EVT_OK) {
        if(!desktop_r0n1n_launch(desktop, desktop->r0n1n.quick[slot], NULL)) {
            desktop_r0n1n_show_info(desktop, desktop_quick_not_installed);
        }
        return true;
    } else if(kind == R0N1N_EVT_HOLD) {
        desktop->picker_slot = slot;
        scene_manager_next_scene(desktop->scene_manager, DesktopSceneAppPicker);
        return true;
    }
    return false;
}

void desktop_scene_favorites_on_exit(void* context) {
    UNUSED(context);
}
