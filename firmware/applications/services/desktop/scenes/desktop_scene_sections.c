#include <furi.h>

#include "desktop_scene.h"
#include "desktop_scene_r0n1n.h"

// R0N1N sections (Left/Right on Home, docs/UX_DESIGN.md): the active
// profile's sections as a carousel. Scene state on entry: the position to
// start at (UINT32_MAX = last, i.e. entered with Left).

void desktop_scene_sections_on_enter(void* context) {
    Desktop* desktop = context;
    const R0n1nProfileInfo* profile = &r0n1n_profiles[desktop->r0n1n.profile];

    desktop_r0n1n_prepare_carousel(desktop);
    r0n1n_carousel_set_title(desktop->r0n1n_carousel, profile->name);
    for(uint32_t i = 0; i < profile->section_count; i++) {
        const R0n1nSectionInfo* section = &r0n1n_sections[profile->sections[i]];
        if(!desktop_r0n1n_section_count(desktop, profile->sections[i])) continue; // empty
        r0n1n_carousel_add_item(
            desktop->r0n1n_carousel,
            section->tile_icon,
            section->short_label,
            profile->sections[i]);
    }
    r0n1n_carousel_set_position(
        desktop->r0n1n_carousel,
        scene_manager_get_scene_state(desktop->scene_manager, DesktopSceneSections));

    view_dispatcher_switch_to_view(desktop->view_dispatcher, DesktopViewIdR0n1nCarousel);
}

bool desktop_scene_sections_on_event(void* context, SceneManagerEvent event) {
    Desktop* desktop = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    if((event.event & R0N1N_EVT_KIND) != R0N1N_EVT_OK) return false;

    const uint32_t section = event.event & R0N1N_EVT_VALUE;
    if(section >= R0n1nSectionCount) return false;

    scene_manager_set_scene_state(
        desktop->scene_manager,
        DesktopSceneSections,
        r0n1n_carousel_get_position(desktop->r0n1n_carousel));
    desktop->section = section;
    scene_manager_next_scene(desktop->scene_manager, DesktopSceneSectionApps);
    return true;
}

void desktop_scene_sections_on_exit(void* context) {
    UNUSED(context);
}
