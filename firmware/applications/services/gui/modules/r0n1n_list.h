/**
 * @file r0n1n_list.h
 * R0N1N list view: header with an "n/N" counter, 13 px rows with icons, an
 * optional right-hand text or icon per row, a scrollbar, and optional extras:
 * a search field above the rows, tabs (Left/Right), or a per-item caption
 * shown at the bottom for the selected row.
 */
#pragma once

#include <gui/view.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct R0n1nList R0n1nList;

/** Called with the item's `index` (OK, hold OK) or the new tab number (tabs). */
typedef void (*R0n1nListCallback)(void* context, uint32_t index);

R0n1nList* r0n1n_list_alloc(void);
void r0n1n_list_free(R0n1nList* list);
View* r0n1n_list_get_view(R0n1nList* list);

/** Remove all items and extras (query, tabs, empty text); keeps callbacks. */
void r0n1n_list_reset(R0n1nList* list);

void r0n1n_list_set_title(R0n1nList* list, const Icon* icon, const char* title);

/** `right`, `right_icon`, `caption` may be NULL. */
void r0n1n_list_add_item(
    R0n1nList* list,
    const Icon* icon,
    const char* label,
    const char* right,
    const Icon* right_icon,
    const char* caption,
    uint32_t index);

/** Show a search field with `query` above the rows. */
void r0n1n_list_set_query(R0n1nList* list, const char* query);

/** Show tabs (strings must outlive the list); Left/Right switch them. */
void r0n1n_list_set_tabs(R0n1nList* list, const char* const* tabs, uint8_t count, uint8_t selected);

/** Text shown instead of rows when the list is empty. */
void r0n1n_list_set_empty_text(R0n1nList* list, const char* text);

void r0n1n_list_set_callbacks(
    R0n1nList* list,
    R0n1nListCallback ok,
    R0n1nListCallback hold_ok,
    R0n1nListCallback tab_changed,
    void* context);

/** Select the item with the given `index` value. */
void r0n1n_list_set_selected_item(R0n1nList* list, uint32_t index);

/** `index` value of the selected item, or UINT32_MAX if the list is empty. */
uint32_t r0n1n_list_get_selected_item(R0n1nList* list);

#ifdef __cplusplus
}
#endif
