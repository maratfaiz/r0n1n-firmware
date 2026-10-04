#pragma once

#include "desktop.h"
#include "desktop_settings.h"

#include "animations/animation_manager.h"
#include "views/desktop_view_pin_timeout.h"
#include "views/desktop_view_pin_input.h"
#include "views/desktop_view_locked.h"
#include "views/desktop_view_main.h"
#include "views/desktop_view_lock_menu.h"
#include "views/desktop_view_debug.h"
#include "views/desktop_view_slideshow.h"

#include <gui/gui.h>
#include <gui/view_stack.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/popup.h>
#include <gui/modules/dialog_ex.h>
#include <gui/modules/text_input.h>
#include <gui/modules/r0n1n_list.h>
#include <gui/modules/r0n1n_grid.h>
#include <gui/modules/r0n1n_carousel.h>
#include "views/desktop_view_simple.h"
#include <gui/scene_manager.h>

#include <loader/loader.h>
#include <notification/notification_app.h>
#include <toolbox/path.h>

#include "r0n1n_settings.h"
#include <flipper_application/flipper_application.h>
#include <gui/icon_i.h>
#include "r0n1n_catalog.h"

#define STATUS_BAR_Y_SHIFT 13

// R0N1N Recent apps (docs/UX_DESIGN.md): most-recently-launched first, no
// persistence across reboot in Stage 1 -- see desktop.c/desktop_scene_recent.c.
// Entries are what the Loader was asked to start: an internal app's name or
// a full .fap path, so the length matches FavoriteApp.name_or_path -- a
// shorter buffer would truncate paths into something that can't relaunch.
#define DESKTOP_RECENT_APPS_COUNT   6
#define DESKTOP_RECENT_APP_NAME_LEN sizeof(((FavoriteApp*)NULL)->name_or_path)

typedef enum {
    DesktopViewIdMain,
    DesktopViewIdLockMenu,
    DesktopViewIdLocked,
    DesktopViewIdDebug,
    DesktopViewIdPopup,
    DesktopViewIdPinInput,
    DesktopViewIdPinTimeout,
    DesktopViewIdSlideshow,
    // R0N1N shell screens (docs/UX_DESIGN.md): every list, tile grid and
    // carousel screen reuses one instance of its view, reset on scene entry.
    DesktopViewIdR0n1nList,
    DesktopViewIdR0n1nGrid,
    DesktopViewIdR0n1nCarousel,
    DesktopViewIdSimpleMenu,
    DesktopViewIdTextInput,
    DesktopViewIdDialog,
    DesktopViewIdTotal,
} DesktopViewId;

typedef struct {
    uint8_t hour;
    uint8_t minute;
    bool format_12; // 1 - 12 hour, 0 - 24H
} DesktopClock;

// R0N1N file-backed list rows (captures, search results, Hub): `app` is
// what to launch (Loader name, .fap path or R0N1N_APP_* target), `path` its
// argument or empty, `label` the row text.
#define R0N1N_ENTRIES_MAX 64
#define R0N1N_QUERY_SIZE  48
#define R0N1N_HISTORY_MAX 5

// Icon of an SD app, from its manifest, usable wherever an Icon is
typedef struct {
    uint8_t data[FAP_MANIFEST_MAX_ICON_SIZE];
    const uint8_t* frame;
    Icon icon;
} R0n1nIconSlot;

typedef struct {
    FuriString* app;
    FuriString* path;
    FuriString* label;
    const Icon* icon;
    uint32_t timestamp;
} R0n1nEntry;

struct Desktop {
    FuriThread* scene_thread;

    Gui* gui;
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;

    Popup* popup;
    DesktopLockMenuView* lock_menu;
    DesktopDebugView* debug_view;
    DesktopViewLocked* locked_view;
    DesktopMainView* main_view;
    DesktopViewPinTimeout* pin_timeout_view;
    DesktopSlideshowView* slideshow_view;
    DesktopViewPinInput* pin_input_view;
    R0n1nList* r0n1n_list;
    R0n1nGrid* r0n1n_grid;
    R0n1nCarousel* r0n1n_carousel;
    DesktopSimpleMenu* simple_menu;
    TextInput* text_input;
    DialogEx* dialog_ex;

    ViewStack* main_view_stack;
    ViewStack* locked_view_stack;

    ViewPort* lock_icon_viewport;
    ViewPort* dummy_mode_icon_viewport;
    ViewPort* clock_viewport;
    ViewPort* stealth_mode_icon_viewport;

    Loader* loader;
    Storage* storage;
    NotificationApp* notification;

    FuriPubSub* status_pubsub;
    FuriPubSub* input_events_pubsub;
    FuriPubSubSubscription* input_events_subscription;

    FuriTimer* auto_lock_timer;
    FuriTimer* update_clock_timer;
    // R0N1N Home dashboard (desktop_view_main.c): runs only while the Main
    // scene is on screen, started/stopped in desktop_scene_main_on_enter/exit.
    FuriTimer* dashboard_update_timer;

    AnimationManager* animation_manager;
    FuriSemaphore* animation_semaphore;

    DesktopClock clock;
    DesktopSettings settings;

    // R0N1N Recent apps (docs/UX_DESIGN.md). pending_app_name is a
    // cross-thread scratch field: desktop_loader_callback (Loader's thread)
    // writes it, then blocks on animation_semaphore until the
    // DesktopGlobalBeforeAppStarted handler (ViewDispatcher's thread) has
    // copied it into launched_app_name. That copy is only pushed into
    // recent_apps on DesktopGlobalAppStopped, so launches that fail (app not
    // found, bad .fap) never show up in Recent -- the Loader only publishes
    // ApplicationStopped for an app that actually ran.
    char pending_app_name[DESKTOP_RECENT_APP_NAME_LEN];
    char launched_app_name[DESKTOP_RECENT_APP_NAME_LEN];
    char recent_apps[DESKTOP_RECENT_APPS_COUNT][DESKTOP_RECENT_APP_NAME_LEN];
    uint32_t recent_apps_time[DESKTOP_RECENT_APPS_COUNT]; // RTC timestamp of the last exit
    uint8_t recent_apps_count;

    // R0N1N shell state (r0n1n_shell.c and the R0N1N scenes).
    R0n1nSettings r0n1n;
    R0n1nEntry r0n1n_entries[R0N1N_ENTRIES_MAX];
    size_t r0n1n_entry_count;
    R0n1nIconSlot r0n1n_icons[R0N1N_ENTRIES_MAX];
    size_t r0n1n_icon_count;
    uint32_t capture_index_token; // storage change counter the index was built at; 0 = stale
    char search_query[R0N1N_QUERY_SIZE];
    char search_history[R0N1N_HISTORY_MAX][R0N1N_QUERY_SIZE]; // newest first
    uint8_t search_history_count;
    bool search_history_loaded;
    const char* info_text; // shown by DesktopSceneInfo
    uint8_t section; // section opened from the carousel or the menu
    uint8_t picker_slot; // Quick Actions slot being reassigned

    bool in_transition;
    bool app_running;
    bool locked;
};

// R0N1N Quick Actions/Recent: .fap paths are too long for a 128px-wide
// list, so show just the file name without extension; internal app names
// (no leading '/') are shown as-is.
static inline void desktop_app_display_name(const char* name_or_path, FuriString* label) {
    if(name_or_path[0] == '/') {
        path_extract_filename_no_ext(name_or_path, label);
    } else {
        furi_string_set(label, name_or_path);
    }
}

void desktop_lock(Desktop* desktop);
void desktop_unlock(Desktop* desktop);
void desktop_set_dummy_mode_state(Desktop* desktop, bool enabled);
void desktop_set_stealth_mode_state(Desktop* desktop, bool enabled);
void desktop_run_archive(Desktop* desktop);

/* R0N1N shell helpers (r0n1n_shell.c) */

/** Launch a Loader name, .fap path, "<file>.fap" (searched under /ext/apps) or
 * R0N1N_APP_* target. Returns false if a "<file>.fap" target isn't installed. */
bool desktop_r0n1n_launch(Desktop* desktop, const char* target, const char* args);

/** Open Search: recent queries first if there are any, else the keyboard. */
void desktop_r0n1n_open_search(Desktop* desktop);

/** Put `query` at the top of the recent searches and save them to the SD card. */
void desktop_r0n1n_history_push(Desktop* desktop, const char* query);

/** Resolve "<file>.fap" to its path under /ext/apps/<category>/. */
bool desktop_r0n1n_find_fap(Desktop* desktop, const char* file_name, FuriString* path);

/** Keep up to R0N1N_ENTRIES_MAX entries, newest `timestamp` first. */
void desktop_r0n1n_entries_add(
    Desktop* desktop,
    const char* app,
    const char* path,
    const char* label,
    const Icon* icon,
    uint32_t timestamp);
void desktop_r0n1n_entries_clear(Desktop* desktop);

/** Saved captures (NFC, Sub-GHz, IR, RFID, iButton), newest first. */
void desktop_r0n1n_scan_captures(Desktop* desktop, const char* filter);

/** .fap apps under /ext/apps; `category` NULL for all, `filter` NULL for no
 * name filter. Fills `categories` (up to `max_categories` names of
 * R0N1N_CATEGORY_SIZE) if non-NULL; returns how many were found. */
#define R0N1N_CATEGORY_SIZE 16
size_t desktop_r0n1n_scan_apps(
    Desktop* desktop,
    const char* category,
    const char* filter,
    char (*categories)[R0N1N_CATEGORY_SIZE],
    size_t max_categories);

/** Entries for `section`: its built-in apps (with 14 px `tile icons` or 10 px
 * list icons), then the apps in its SD folders under their manifest names
 * and icons, alphabetically. */
typedef enum {
    R0n1nFeedbackClick, // a tap: selecting/opening something
    R0n1nFeedbackSuccess, // a card read, a save
    R0n1nFeedbackError, // nothing found, an app is missing
} R0n1nFeedback;

/** Short click/vibro + optional success/error cue, if feedback is enabled. */
void desktop_r0n1n_feedback(Desktop* desktop, R0n1nFeedback kind);

void desktop_r0n1n_section_entries(Desktop* desktop, R0n1nSection section, bool tile_icons);

/** How many apps `section` holds: built-in plus SD (a directory scan only). */
size_t desktop_r0n1n_section_count(Desktop* desktop, R0n1nSection section);

/** "сейчас", "5 мин", "2 ч", "3 д" relative to now. */
void desktop_r0n1n_format_age(uint32_t timestamp, char* out, size_t size);

/** "16:32" today, "23.09" otherwise. */
void desktop_r0n1n_format_time(uint32_t timestamp, char* out, size_t size);

/** Case-insensitive (ASCII) substring match. */
bool desktop_r0n1n_matches(const char* text, const char* query);
