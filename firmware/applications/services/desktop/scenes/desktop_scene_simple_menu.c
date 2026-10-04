#include <furi.h>
#include <assets_icons.h>

#include "desktop_scene.h"
#include "desktop_scene_r0n1n.h"
#include "../views/desktop_view_simple.h"

// R0N1N simple mode (docs/UX_DESIGN.md): the basic functions only, one big
// item per screen. "Обычный вид" leaves simple mode after a confirmation.

typedef struct {
    const Icon* icon;
    const char* label;
    const char* target; // for desktop_r0n1n_launch(); NULL = leave simple mode
    const char* args;
} DesktopSimpleItem;

static const DesktopSimpleItem desktop_simple_items[] = {
    {&I_R_BigTv_32x32, "Пульт ТВ", "Infrared", "@tv"},
    {&I_R_BigGate_32x32, "Ворота", "Sub-GHz", NULL},
    {&I_R_BigCard_32x32, "Карты", "NFC", NULL},
    {&I_R_BigTablet_32x32, "Домофон", "iButton", NULL},
    {&I_R_BigFob_32x32, "Брелок", "125 kHz RFID", NULL},
    {&I_R_BigFolder_32x32, "Файлы", R0N1N_APP_ARCHIVE, NULL},
    {&I_R_BigAcademy_32x32, "Академия", "academy.fap", NULL},
    {&I_R_BigPower_32x32, "Выключить", "Power", "off"},
    {&I_R_BigGrid_32x32, "Обычный вид", NULL, NULL},
};

#define DESKTOP_SIMPLE_CONFIRM_EXIT (1UL << 15)

static void desktop_scene_simple_menu_callback(uint32_t index, void* context) {
    Desktop* desktop = context;
    view_dispatcher_send_custom_event(desktop->view_dispatcher, R0N1N_EVT_OK | index);
}

static void desktop_scene_simple_menu_dialog_callback(DialogExResult result, void* context) {
    Desktop* desktop = context;
    if(result == DialogExResultRight) {
        view_dispatcher_send_custom_event(
            desktop->view_dispatcher, R0N1N_EVT_OK | DESKTOP_SIMPLE_CONFIRM_EXIT);
    } else {
        view_dispatcher_switch_to_view(desktop->view_dispatcher, DesktopViewIdSimpleMenu);
    }
}

static void desktop_scene_simple_menu_ask_exit(Desktop* desktop) {
    DialogEx* dialog = desktop->dialog_ex;
    dialog_ex_reset(dialog);
    dialog_ex_set_header(dialog, "Обычный вид?", 64, 2, AlignCenter, AlignTop);
    dialog_ex_set_text(
        dialog, "Вернуть все функции\nи обычный экран", 64, 28, AlignCenter, AlignCenter);
    dialog_ex_set_left_button_text(dialog, "Нет");
    dialog_ex_set_right_button_text(dialog, "Да");
    dialog_ex_set_context(dialog, desktop);
    dialog_ex_set_result_callback(dialog, desktop_scene_simple_menu_dialog_callback);
    view_dispatcher_switch_to_view(desktop->view_dispatcher, DesktopViewIdDialog);
}

void desktop_scene_simple_menu_on_enter(void* context) {
    Desktop* desktop = context;
    DesktopSimpleMenu* menu = desktop->simple_menu;

    desktop_simple_menu_reset(menu);
    for(size_t i = 0; i < COUNT_OF(desktop_simple_items); i++) {
        desktop_simple_menu_add_item(
            menu, desktop_simple_items[i].icon, desktop_simple_items[i].label);
    }
    desktop_simple_menu_set_callback(menu, desktop_scene_simple_menu_callback, desktop);
    desktop_simple_menu_set_selected(
        menu, scene_manager_get_scene_state(desktop->scene_manager, DesktopSceneSimpleMenu));
    view_dispatcher_switch_to_view(desktop->view_dispatcher, DesktopViewIdSimpleMenu);
}

bool desktop_scene_simple_menu_on_event(void* context, SceneManagerEvent event) {
    Desktop* desktop = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    if((event.event & R0N1N_EVT_KIND) != R0N1N_EVT_OK) return false;

    const uint32_t value = event.event & R0N1N_EVT_VALUE;
    if(value == DESKTOP_SIMPLE_CONFIRM_EXIT) {
        desktop->r0n1n.simple_mode = false;
        r0n1n_settings_save(&desktop->r0n1n);
        desktop_main_set_simple_mode(desktop->main_view, false);
        scene_manager_set_scene_state(desktop->scene_manager, DesktopSceneSimpleMenu, 0);
        scene_manager_search_and_switch_to_previous_scene(
            desktop->scene_manager, DesktopSceneMain);
        return true;
    }
    if(value >= COUNT_OF(desktop_simple_items)) return false;

    scene_manager_set_scene_state(desktop->scene_manager, DesktopSceneSimpleMenu, value);
    const DesktopSimpleItem* item = &desktop_simple_items[value];
    if(!item->target) {
        desktop_scene_simple_menu_ask_exit(desktop);
    } else if(!desktop_r0n1n_launch(desktop, item->target, item->args)) {
        desktop_r0n1n_show_info(desktop, "Приложение\nне найдено на SD");
    }
    return true;
}

void desktop_scene_simple_menu_on_exit(void* context) {
    Desktop* desktop = context;
    dialog_ex_reset(desktop->dialog_ex);
}
