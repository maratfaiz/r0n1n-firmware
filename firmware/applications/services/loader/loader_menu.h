#pragma once
#include <furi.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LoaderMenu LoaderMenu;

LoaderMenu* loader_menu_alloc(void (*closed_cb)(void*), void* context);

void loader_menu_free(LoaderMenu* loader_menu);

/** R0N1N: the Russian name shown for app `name`, or `name` itself. */
const char* loader_display_name(const char* name);

#ifdef __cplusplus
}
#endif
