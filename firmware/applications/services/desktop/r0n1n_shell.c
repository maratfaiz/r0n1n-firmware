// R0N1N shell helpers shared by the R0N1N desktop scenes: launching, file
// scanning for Captures/Search/Hub, and time formatting.

#include "desktop_i.h"
#include "scenes/desktop_scene.h"
#include "scenes/desktop_scene_r0n1n.h"

#include <assets_icons.h>
#include <gui/utf8_i.h>
#include <datetime/datetime.h>
#include <furi_hal_rtc.h>
#include <storage/storage.h>
#include <storage/storage_mtime.h>
#include <loader/loader_menu.h>
#include <toolbox/path.h>

#define TAG "R0n1nShell"

#define R0N1N_APPS_DIR EXT_PATH("apps")

typedef struct {
    const char* dir;
    const char* extension;
    const char* app;
    const Icon* icon;
} R0n1nCaptureType;

static const R0n1nCaptureType r0n1n_capture_types[] = {
    {EXT_PATH("nfc"), ".nfc", "NFC", &I_Nfc_10px},
    {EXT_PATH("subghz"), ".sub", "Sub-GHz", &I_sub1_10px},
    {EXT_PATH("infrared"), ".ir", "Infrared", &I_ir_10px},
    {EXT_PATH("lfrfid"), ".rfid", "125 kHz RFID", &I_125_10px},
    {EXT_PATH("ibutton"), ".ibtn", "iButton", &I_ibutt_10px},
};

// Case folding for search: ASCII and Russian letters, ё counts as е
static uint16_t desktop_r0n1n_fold(uint16_t code) {
    if(code < 0x80) return tolower(code);
    if(code >= 0x410 && code <= 0x42F) code += 0x20;
    if(code == 0x401 || code == 0x451) code = 0x435;
    return code;
}

bool desktop_r0n1n_matches(const char* text, const char* query) {
    if(!query || !query[0]) return true;
    for(const char* start = text; *start;) {
        const char* t = start;
        const char* q = query;
        uint16_t tc, qc;
        while(*q && *t) {
            size_t tl = gui_utf8_char(t, &tc);
            size_t ql = gui_utf8_char(q, &qc);
            if(desktop_r0n1n_fold(tc) != desktop_r0n1n_fold(qc)) break;
            t += tl;
            q += ql;
        }
        if(!*q) return true;
        start += gui_utf8_char(start, &tc);
    }
    return false;
}

bool desktop_r0n1n_find_fap(Desktop* desktop, const char* file_name, FuriString* path) {
    bool found = false;
    File* dir = storage_file_alloc(desktop->storage);
    char name[64];
    FileInfo info;

    if(storage_dir_open(dir, R0N1N_APPS_DIR)) {
        while(!found && storage_dir_read(dir, &info, name, sizeof(name))) {
            if(!file_info_is_dir(&info)) continue;
            furi_string_printf(path, "%s/%s/%s", R0N1N_APPS_DIR, name, file_name);
            found = storage_file_exists(desktop->storage, furi_string_get_cstr(path));
        }
    }
    storage_dir_close(dir);
    storage_file_free(dir);
    return found;
}

bool desktop_r0n1n_launch(Desktop* desktop, const char* target, const char* args) {
    furi_assert(desktop);
    furi_assert(target);

    if(strcmp(target, R0N1N_APP_SETTINGS) == 0) {
        scene_manager_next_scene(desktop->scene_manager, DesktopSceneR0n1nSettings);
        return true;
    }

    if(strcmp(target, R0N1N_APP_ARCHIVE) == 0) {
        desktop_run_archive(desktop);
    } else if(strcmp(target, R0N1N_APP_ALL) == 0) {
        loader_show_menu(desktop->loader);
    } else if(target[0] != '/' && strstr(target, ".fap")) {
        FuriString* path = furi_string_alloc();
        const bool found = desktop_r0n1n_find_fap(desktop, target, path);
        if(found) {
            loader_start_detached_with_gui_error(
                desktop->loader, furi_string_get_cstr(path), args && args[0] ? args : NULL);
        }
        furi_string_free(path);
        if(!found) return false;
    } else {
        loader_start_detached_with_gui_error(
            desktop->loader, target, args && args[0] ? args : NULL);
    }

    // Whatever was launched, coming back from it lands on Home, not deep in a menu.
    scene_manager_search_and_switch_to_previous_scene(desktop->scene_manager, DesktopSceneMain);
    return true;
}

void desktop_r0n1n_entries_clear(Desktop* desktop) {
    for(size_t i = 0; i < desktop->r0n1n_entry_count; i++) {
        furi_string_free(desktop->r0n1n_entries[i].app);
        furi_string_free(desktop->r0n1n_entries[i].path);
        furi_string_free(desktop->r0n1n_entries[i].label);
    }
    desktop->r0n1n_entry_count = 0;
    desktop->r0n1n_icon_count = 0;
}

void desktop_r0n1n_entries_add(
    Desktop* desktop,
    const char* app,
    const char* path,
    const char* label,
    const Icon* icon,
    uint32_t timestamp) {
    size_t pos = desktop->r0n1n_entry_count;
    while(pos > 0 && desktop->r0n1n_entries[pos - 1].timestamp < timestamp) {
        pos--;
    }
    if(pos >= R0N1N_ENTRIES_MAX) return;

    R0n1nEntry entry;
    if(desktop->r0n1n_entry_count == R0N1N_ENTRIES_MAX) {
        // Full: recycle the oldest entry's strings for the new one.
        entry = desktop->r0n1n_entries[R0N1N_ENTRIES_MAX - 1];
        desktop->r0n1n_entry_count--;
    } else {
        entry.app = furi_string_alloc();
        entry.path = furi_string_alloc();
        entry.label = furi_string_alloc();
    }
    furi_string_set(entry.app, app);
    furi_string_set(entry.path, path ? path : "");
    furi_string_set(entry.label, label);
    entry.icon = icon;
    entry.timestamp = timestamp;

    memmove(
        &desktop->r0n1n_entries[pos + 1],
        &desktop->r0n1n_entries[pos],
        (desktop->r0n1n_entry_count - pos) * sizeof(R0n1nEntry));
    desktop->r0n1n_entries[pos] = entry;
    desktop->r0n1n_entry_count++;
}

void desktop_r0n1n_scan_captures(Desktop* desktop, const char* filter) {
    File* dir = storage_file_alloc(desktop->storage);
    FuriString* path = furi_string_alloc();
    FuriString* label = furi_string_alloc();
    char name[64];
    FileInfo info;

    for(size_t t = 0; t < COUNT_OF(r0n1n_capture_types); t++) {
        const R0n1nCaptureType* type = &r0n1n_capture_types[t];
        if(!storage_dir_open(dir, type->dir)) {
            storage_dir_close(dir);
            continue;
        }
        while(storage_dir_read(dir, &info, name, sizeof(name))) {
            if(file_info_is_dir(&info) || name[0] == '.') continue;
            const size_t len = strlen(name);
            const size_t ext_len = strlen(type->extension);
            if(len <= ext_len || strcmp(name + len - ext_len, type->extension) != 0) continue;
            if(!desktop_r0n1n_matches(name, filter)) continue;

            furi_string_printf(path, "%s/%s", type->dir, name);
            uint32_t timestamp = 0;
            storage_common_mtime(desktop->storage, furi_string_get_cstr(path), &timestamp);
            furi_string_set_strn(label, name, len - ext_len);
            desktop_r0n1n_entries_add(
                desktop,
                type->app,
                furi_string_get_cstr(path),
                furi_string_get_cstr(label),
                type->icon,
                timestamp);
        }
        storage_dir_close(dir);
    }

    furi_string_free(label);
    furi_string_free(path);
    storage_file_free(dir);
}

size_t desktop_r0n1n_scan_apps(
    Desktop* desktop,
    const char* category,
    const char* filter,
    char (*categories)[R0N1N_CATEGORY_SIZE],
    size_t max_categories) {
    File* top = storage_file_alloc(desktop->storage);
    File* dir = storage_file_alloc(desktop->storage);
    FuriString* path = furi_string_alloc();
    FuriString* label = furi_string_alloc();
    char cat[R0N1N_CATEGORY_SIZE];
    char name[64];
    FileInfo info;
    size_t category_count = 0;

    if(storage_dir_open(top, R0N1N_APPS_DIR)) {
        while(storage_dir_read(top, &info, cat, sizeof(cat))) {
            if(!file_info_is_dir(&info) || cat[0] == '.') continue;
            if(categories && category_count < max_categories) {
                strlcpy(categories[category_count++], cat, R0N1N_CATEGORY_SIZE);
            }
            if(category && strcmp(category, cat) != 0) continue;

            furi_string_printf(path, "%s/%s", R0N1N_APPS_DIR, cat);
            if(storage_dir_open(dir, furi_string_get_cstr(path))) {
                while(storage_dir_read(dir, &info, name, sizeof(name))) {
                    const size_t len = strlen(name);
                    if(file_info_is_dir(&info) || len <= 4 ||
                       strcmp(name + len - 4, ".fap") != 0) {
                        continue;
                    }
                    if(!desktop_r0n1n_matches(name, filter)) continue;
                    // "hid_usb.fap" -> "Hid usb": readable without loading the manifest.
                    furi_string_set_strn(label, name, len - 4);
                    furi_string_replace_all(label, "_", " ");
                    char first = furi_string_get_char(label, 0);
                    if(first >= 'a' && first <= 'z') furi_string_set_char(label, 0, first - 32);

                    furi_string_printf(path, "%s/%s/%s", R0N1N_APPS_DIR, cat, name);
                    // Alphabetical order: timestamps sort descending, so invert the first letters.
                    const uint32_t order =
                        UINT32_MAX - (((uint32_t)(uint8_t)tolower((unsigned char)name[0]) << 24) |
                                      ((uint32_t)(uint8_t)tolower((unsigned char)name[1]) << 16));
                    desktop_r0n1n_entries_add(
                        desktop,
                        furi_string_get_cstr(path),
                        NULL,
                        furi_string_get_cstr(label),
                        &I_R_App_9x7,
                        order);
                }
            }
            storage_dir_close(dir);
        }
    }
    storage_dir_close(top);

    furi_string_free(label);
    furi_string_free(path);
    storage_file_free(dir);
    storage_file_free(top);
    return category_count;
}

void desktop_r0n1n_format_age(uint32_t timestamp, char* out, size_t size) {
    const uint32_t now = furi_hal_rtc_get_timestamp();
    const uint32_t age = now > timestamp ? now - timestamp : 0;
    if(age < 60) {
        strlcpy(out, "сейчас", size);
    } else if(age < 3600) {
        snprintf(out, size, "%lu мин", age / 60);
    } else if(age < 86400) {
        snprintf(out, size, "%lu ч", age / 3600);
    } else {
        snprintf(out, size, "%lu д", age / 86400);
    }
}

void desktop_r0n1n_format_time(uint32_t timestamp, char* out, size_t size) {
    DateTime now;
    DateTime then;
    furi_hal_rtc_get_datetime(&now);
    datetime_timestamp_to_datetime(timestamp, &then);
    if(now.year == then.year && now.month == then.month && now.day == then.day) {
        snprintf(out, size, "%02u:%02u", then.hour, then.minute);
    } else {
        snprintf(out, size, "%02u.%02u", then.day, then.month);
    }
}

static void desktop_r0n1n_send(Desktop* desktop, uint32_t kind, uint32_t value) {
    view_dispatcher_send_custom_event(desktop->view_dispatcher, kind | (value & R0N1N_EVT_VALUE));
}

static void desktop_r0n1n_ok_callback(void* context, uint32_t index) {
    desktop_r0n1n_send(context, R0N1N_EVT_OK, index);
}

static void desktop_r0n1n_hold_callback(void* context, uint32_t index) {
    desktop_r0n1n_send(context, R0N1N_EVT_HOLD, index);
}

static void desktop_r0n1n_tab_callback(void* context, uint32_t tab) {
    desktop_r0n1n_send(context, R0N1N_EVT_TAB, tab);
}

static void desktop_r0n1n_slider_callback(void* context, uint8_t value) {
    desktop_r0n1n_send(context, R0N1N_EVT_SLIDER, value);
}

void desktop_r0n1n_prepare_list(Desktop* desktop) {
    r0n1n_list_reset(desktop->r0n1n_list);
    r0n1n_list_set_callbacks(
        desktop->r0n1n_list,
        desktop_r0n1n_ok_callback,
        desktop_r0n1n_hold_callback,
        desktop_r0n1n_tab_callback,
        desktop);
}

void desktop_r0n1n_prepare_grid(Desktop* desktop) {
    r0n1n_grid_reset(desktop->r0n1n_grid);
    r0n1n_grid_set_callbacks(
        desktop->r0n1n_grid,
        desktop_r0n1n_ok_callback,
        desktop_r0n1n_hold_callback,
        desktop_r0n1n_slider_callback,
        desktop);
}

void desktop_r0n1n_prepare_carousel(Desktop* desktop) {
    r0n1n_carousel_reset(desktop->r0n1n_carousel);
    r0n1n_carousel_set_callback(desktop->r0n1n_carousel, desktop_r0n1n_ok_callback, desktop);
}

void desktop_r0n1n_show_info(Desktop* desktop, const char* text) {
    desktop->info_text = text;
    scene_manager_next_scene(desktop->scene_manager, DesktopSceneInfo);
}

// SD folder `dir` belongs to `section`: named by it, or (Other) by none
static bool desktop_r0n1n_dir_in_section(const char* dir, R0n1nSection section) {
    for(size_t s = 0; s < R0n1nSectionCount; s++) {
        const char* const* dirs = r0n1n_sections[s].sd_dirs;
        for(size_t i = 0; dirs && dirs[i]; i++) {
            if(strcmp(dirs[i], dir) == 0) return s == section;
        }
    }
    return section == R0n1nSectionOther;
}

// An SD app already listed as a built-in entry of some section
static bool desktop_r0n1n_is_builtin_path(const char* path) {
    for(size_t s = 0; s < R0n1nSectionCount; s++) {
        for(size_t i = 0; i < r0n1n_sections[s].app_count; i++) {
            if(strcmp(r0n1n_sections[s].apps[i].name, path) == 0) return true;
        }
    }
    return false;
}

// Visit the .fap files of `section`'s SD folders; returns how many there are
static size_t desktop_r0n1n_section_scan(
    Desktop* desktop,
    R0n1nSection section,
    void (*visit)(Desktop* desktop, FuriString* path, void* context),
    void* context) {
    File* top = storage_file_alloc(desktop->storage);
    File* dir = storage_file_alloc(desktop->storage);
    FuriString* path = furi_string_alloc();
    char cat[R0N1N_CATEGORY_SIZE * 2];
    char name[64];
    FileInfo info;
    size_t count = 0;

    if(storage_dir_open(top, R0N1N_APPS_DIR)) {
        while(storage_dir_read(top, &info, cat, sizeof(cat))) {
            if(!file_info_is_dir(&info) || cat[0] == '.') continue;
            if(!desktop_r0n1n_dir_in_section(cat, section)) continue;
            furi_string_printf(path, "%s/%s", R0N1N_APPS_DIR, cat);
            if(!storage_dir_open(dir, furi_string_get_cstr(path))) {
                storage_dir_close(dir);
                continue;
            }
            while(storage_dir_read(dir, &info, name, sizeof(name))) {
                const size_t len = strlen(name);
                if(file_info_is_dir(&info) || len <= 4 || strcmp(name + len - 4, ".fap") != 0) {
                    continue;
                }
                furi_string_printf(path, "%s/%s/%s", R0N1N_APPS_DIR, cat, name);
                if(desktop_r0n1n_is_builtin_path(furi_string_get_cstr(path))) continue;
                count++;
                if(visit) visit(desktop, path, context);
            }
            storage_dir_close(dir);
        }
    }
    storage_dir_close(top);

    furi_string_free(path);
    storage_file_free(dir);
    storage_file_free(top);
    return count;
}

size_t desktop_r0n1n_section_count(Desktop* desktop, R0n1nSection section) {
    return r0n1n_sections[section].app_count +
           desktop_r0n1n_section_scan(desktop, section, NULL, NULL);
}

static void desktop_r0n1n_add_sd_app(Desktop* desktop, FuriString* path, void* context) {
    UNUSED(context);
    FuriString* name = furi_string_alloc();
    const Icon* icon = &I_R_App_9x7;

    // The manifest has the app's real name and its 10x10 icon
    uint8_t* icon_data = NULL;
    R0n1nIconSlot* slot = NULL;
    if(desktop->r0n1n_icon_count < R0N1N_ENTRIES_MAX) {
        slot = &desktop->r0n1n_icons[desktop->r0n1n_icon_count];
        memset(slot->data, 0, sizeof(slot->data));
        icon_data = slot->data;
    } else {
        icon_data = malloc(FAP_MANIFEST_MAX_ICON_SIZE); // name only, icon dropped
    }
    const bool loaded =
        flipper_application_load_name_and_icon(path, desktop->storage, &icon_data, name);
    if(loaded && slot && (slot->data[0] || slot->data[1] || slot->data[2])) {
        slot->frame = slot->data;
        const Icon fap_icon = {
            .width = 10, .height = 10, .frame_count = 1, .frame_rate = 0, .frames = &slot->frame};
        memcpy(&slot->icon, &fap_icon, sizeof(Icon));
        icon = &slot->icon;
        desktop->r0n1n_icon_count++;
    }
    if(!slot) free(icon_data);
    if(!loaded) {
        // Fall back to the file name: "hid_usb.fap" -> "Hid usb"
        path_extract_filename(path, name, true);
        furi_string_replace_all(name, "_", " ");
    }

    // Alphabetical after the built-ins: entries sort by descending timestamp
    const char* n = furi_string_get_cstr(name);
    const uint32_t order =
        (UINT32_MAX >> 1) - (((uint32_t)(uint8_t)tolower((unsigned char)n[0]) << 16) |
                             ((uint32_t)(uint8_t)tolower((unsigned char)n[1]) << 8));
    desktop_r0n1n_entries_add(
        desktop, furi_string_get_cstr(path), NULL, loader_display_name(n), icon, order);
    furi_string_free(name);
}

void desktop_r0n1n_section_entries(Desktop* desktop, R0n1nSection section, bool tile_icons) {
    const R0n1nSectionInfo* info = &r0n1n_sections[section];
    desktop_r0n1n_entries_clear(desktop);
    for(size_t i = 0; i < info->app_count; i++) {
        const R0n1nApp* app = &info->apps[i];
        // .fap built-ins live on SD and may be missing
        if(app->name[0] == '/' && !storage_file_exists(desktop->storage, app->name)) continue;
        desktop_r0n1n_entries_add(
            desktop,
            app->name,
            NULL,
            app->label,
            tile_icons ? app->tile_icon : app->icon,
            UINT32_MAX - i);
    }
    desktop_r0n1n_section_scan(desktop, section, desktop_r0n1n_add_sd_app, NULL);
}
