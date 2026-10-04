/**
 * @file desktop_view_simple.h
 * R0N1N simple mode menu: one big item per screen (32 px icon, large
 * label), Left/Right (or Up/Down) to move, OK to open, Back to leave.
 */
#pragma once

#include <gui/view.h>

#define DESKTOP_SIMPLE_MENU_MAX_ITEMS 12

typedef struct DesktopSimpleMenu DesktopSimpleMenu;
typedef void (*DesktopSimpleMenuCallback)(uint32_t index, void* context);

DesktopSimpleMenu* desktop_simple_menu_alloc(void);
void desktop_simple_menu_free(DesktopSimpleMenu* menu);
View* desktop_simple_menu_get_view(DesktopSimpleMenu* menu);

void desktop_simple_menu_reset(DesktopSimpleMenu* menu);
/** `icon` is a 32x32 icon; `label` must outlive the menu (up to ~11 letters). */
void desktop_simple_menu_add_item(DesktopSimpleMenu* menu, const Icon* icon, const char* label);
void desktop_simple_menu_set_selected(DesktopSimpleMenu* menu, uint32_t index);
uint32_t desktop_simple_menu_get_selected(DesktopSimpleMenu* menu);
void desktop_simple_menu_set_callback(
    DesktopSimpleMenu* menu,
    DesktopSimpleMenuCallback callback,
    void* context);
