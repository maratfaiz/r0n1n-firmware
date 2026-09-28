#include <furi.h>
#include <assets_icons.h>

#include "desktop_scene.h"
#include "desktop_scene_r0n1n.h"

// R0N1N Applications menu (OK on Home, docs/UX_DESIGN.md): folders of apps
// like on a phone -- one per section that has apps, with the count in the
// caption -- then the shell's own screens. Everything is reachable from here
// even if a profile hides it from the Left/Right sections.

typedef enum {
    MenuFiles = 100,
    MenuCaptures,
    MenuSearch,
    MenuHub,
    MenuAllApps,
    MenuSettings,
} MenuItem;

typedef struct {
    MenuItem item;
    const Icon* icon;
    const char* caption;
} DesktopMenuTile;

static const DesktopMenuTile desktop_menu_tiles[] = {
    {MenuFiles, &A_FileManager_14, "Файлы"},
    {MenuCaptures, &I_R_TileClock_14x14, "Захваты"},
    {MenuSearch, &I_R_TileSearch_14x14, "Поиск"},
    {MenuHub, &I_R_TileHub_14x14, "R0N1N Hub"},
    {MenuAllApps, &I_R_TileAll_14x14, "Все приложения"},
    {MenuSettings, &A_Settings_14, "Настройки"},
};

// Captions live as long as the menu is shown
static FuriString* desktop_menu_captions[R0n1nSectionCount];

void desktop_scene_menu_on_enter(void* context) {
    Desktop* desktop = context;
    R0n1nGrid* grid = desktop->r0n1n_grid;

    desktop_r0n1n_prepare_grid(desktop);
    r0n1n_grid_set_title(grid, NULL, "Приложения");
    r0n1n_grid_set_layout(grid, 4, 26, 18, 13);

    for(uint32_t s = 0; s < R0n1nSectionCount; s++) {
        const size_t count = desktop_r0n1n_section_count(desktop, s);
        if(!count) continue; // no empty folders
        if(!desktop_menu_captions[s]) desktop_menu_captions[s] = furi_string_alloc();
        furi_string_printf(
            desktop_menu_captions[s], "%s (%u)", r0n1n_sections[s].title, (unsigned)count);
        r0n1n_grid_add_item(
            grid,
            r0n1n_sections[s].tile_icon,
            furi_string_get_cstr(desktop_menu_captions[s]),
            false,
            s);
    }
    for(size_t i = 0; i < COUNT_OF(desktop_menu_tiles); i++) {
        r0n1n_grid_add_item(
            grid,
            desktop_menu_tiles[i].icon,
            desktop_menu_tiles[i].caption,
            false,
            desktop_menu_tiles[i].item);
    }
    r0n1n_grid_set_selected_item(
        grid, scene_manager_get_scene_state(desktop->scene_manager, DesktopSceneMenu));
    view_dispatcher_switch_to_view(desktop->view_dispatcher, DesktopViewIdR0n1nGrid);
}

bool desktop_scene_menu_on_event(void* context, SceneManagerEvent event) {
    Desktop* desktop = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    if((event.event & R0N1N_EVT_KIND) != R0N1N_EVT_OK) return false;

    const uint32_t item = event.event & R0N1N_EVT_VALUE;
    scene_manager_set_scene_state(desktop->scene_manager, DesktopSceneMenu, item);

    if(item < R0n1nSectionCount) {
        desktop->section = item;
        scene_manager_next_scene(desktop->scene_manager, DesktopSceneSectionApps);
    } else if(item == MenuFiles) {
        desktop_r0n1n_launch(desktop, R0N1N_APP_ARCHIVE, NULL);
    } else if(item == MenuCaptures) {
        scene_manager_next_scene(desktop->scene_manager, DesktopSceneCaptures);
    } else if(item == MenuSearch) {
        scene_manager_next_scene(desktop->scene_manager, DesktopSceneSearch);
    } else if(item == MenuHub) {
        scene_manager_next_scene(desktop->scene_manager, DesktopSceneHub);
    } else if(item == MenuAllApps) {
        desktop_r0n1n_launch(desktop, R0N1N_APP_ALL, NULL);
    } else if(item == MenuSettings) {
        scene_manager_next_scene(desktop->scene_manager, DesktopSceneR0n1nSettings);
    }
    return true;
}

void desktop_scene_menu_on_exit(void* context) {
    UNUSED(context);
}
