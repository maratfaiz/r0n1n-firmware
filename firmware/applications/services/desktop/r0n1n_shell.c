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
#include <notification/notification_messages.h>
#include <notification/notification_messages_notes.h>
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

/* Smart search (docs/UX_DESIGN.md, "Feedback and search"): a query matches
 * when it, or its transliteration into the other alphabet ("pitanie" finds
 * "Питание", "ибуттон" finds "iButton"), appears in the text; a query of 4+
 * letters also matches the start of a word with one typo, a missing or extra
 * letter, or two neighbouring letters swapped. Buffers are static: the
 * desktop thread is the only caller and its stack is small. */
#define R0N1N_MATCH_TEXT  80
#define R0N1N_MATCH_QUERY R0N1N_QUERY_SIZE
#define R0N1N_MATCH_FUZZY 4

static uint16_t r0n1n_match_text[R0N1N_MATCH_TEXT];
static uint16_t r0n1n_match_query[3][R0N1N_MATCH_QUERY];

static size_t desktop_r0n1n_decode(const char* s, uint16_t* out, size_t max) {
    size_t n = 0;
    uint16_t c;
    while(*s && n < max) {
        s += gui_utf8_char(s, &c);
        out[n++] = desktop_r0n1n_fold(c);
    }
    return n;
}

static bool desktop_r0n1n_is_cyrillic(uint16_t c) {
    return c >= 0x430 && c <= 0x44F;
}

// Latin -> Russian, longest spelling first ("sch" before "sh" before "s")
static size_t desktop_r0n1n_to_cyrillic(const uint16_t* q, size_t n, uint16_t* out) {
    static const struct {
        const char* lat;
        uint16_t cyr[2];
    } table[] = {
        {"sch", {0x449}}, {"sh", {0x448}},  {"ch", {0x447}},         {"zh", {0x436}},
        {"kh", {0x445}},  {"ts", {0x446}},  {"yu", {0x44E}},         {"ya", {0x44F}},
        {"yo", {0x435}},  {"ye", {0x435}},  {"a", {0x430}},          {"b", {0x431}},
        {"c", {0x43A}},   {"d", {0x434}},   {"e", {0x435}},          {"f", {0x444}},
        {"g", {0x433}},   {"h", {0x445}},   {"i", {0x438}},          {"j", {0x439}},
        {"k", {0x43A}},   {"l", {0x43B}},   {"m", {0x43C}},          {"n", {0x43D}},
        {"o", {0x43E}},   {"p", {0x43F}},   {"q", {0x43A}},          {"r", {0x440}},
        {"s", {0x441}},   {"t", {0x442}},   {"u", {0x443}},          {"v", {0x432}},
        {"w", {0x432}},   {"x", {0x43A, 0x441}}, {"y", {0x44B}},    {"z", {0x437}},
    };
    size_t o = 0;
    for(size_t i = 0; i < n && o < R0N1N_MATCH_QUERY - 1;) {
        bool done = false;
        for(size_t k = 0; k < COUNT_OF(table) && !done; k++) {
            const char* lat = table[k].lat;
            size_t len = strlen(lat);
            if(i + len > n) continue;
            size_t j = 0;
            while(j < len && q[i + j] == (uint8_t)lat[j]) j++;
            if(j < len) continue;
            out[o++] = table[k].cyr[0];
            if(table[k].cyr[1] && o < R0N1N_MATCH_QUERY) out[o++] = table[k].cyr[1];
            i += len;
            done = true;
        }
        if(!done) out[o++] = q[i++];
    }
    return o;
}

// Russian -> Latin, as Russian speakers spell English names
static size_t desktop_r0n1n_to_latin(const uint16_t* q, size_t n, uint16_t* out) {
    static const char* const table[32] = {
        "a", "b", "v", "g", "d", "e", "zh", "z", "i", "y", "k", "l", "m", "n", "o", "p",
        "r", "s", "t", "u", "f", "h", "c", "ch", "sh", "sch", "", "y", "", "e", "yu", "ya"};
    size_t o = 0;
    for(size_t i = 0; i < n; i++) {
        const char* lat = desktop_r0n1n_is_cyrillic(q[i]) ? table[q[i] - 0x430] : NULL;
        if(!lat) {
            if(o < R0N1N_MATCH_QUERY) out[o++] = q[i];
            continue;
        }
        for(; *lat && o < R0N1N_MATCH_QUERY; lat++) out[o++] = (uint8_t)*lat;
    }
    return o;
}

static bool desktop_r0n1n_prefix(const uint16_t* t, size_t tn, const uint16_t* q, size_t qn) {
    if(qn > tn) return false;
    for(size_t i = 0; i < qn; i++) {
        if(t[i] != q[i]) return false;
    }
    return true;
}

// Does the text starting here begin with the query, give or take one edit?
static bool desktop_r0n1n_near_prefix(const uint16_t* t, size_t tn, const uint16_t* q, size_t qn) {
    size_t i = 0;
    while(i < qn && i < tn && t[i] == q[i]) i++;
    if(i == qn) return true;
    if(i < tn && desktop_r0n1n_prefix(t + i + 1, tn - i - 1, q + i + 1, qn - i - 1)) {
        return true; // one wrong letter
    }
    if(desktop_r0n1n_prefix(t + i, tn - i, q + i + 1, qn - i - 1)) return true; // extra letter
    if(i < tn && desktop_r0n1n_prefix(t + i + 1, tn - i - 1, q + i, qn - i)) {
        return true; // missing letter
    }
    if(i + 1 < qn && i + 1 < tn && t[i] == q[i + 1] && t[i + 1] == q[i] &&
       desktop_r0n1n_prefix(t + i + 2, tn - i - 2, q + i + 2, qn - i - 2)) {
        return true; // swapped letters
    }
    return false;
}

static bool desktop_r0n1n_is_word_char(uint16_t c) {
    return (c < 0x80 && isalnum(c)) || desktop_r0n1n_is_cyrillic(c);
}

bool desktop_r0n1n_matches(const char* text, const char* query) {
    if(!query || !query[0]) return true;
    if(!text) return false;

    const size_t tn = desktop_r0n1n_decode(text, r0n1n_match_text, R0N1N_MATCH_TEXT);
    size_t qn[3];
    qn[0] = desktop_r0n1n_decode(query, r0n1n_match_query[0], R0N1N_MATCH_QUERY);
    qn[1] = desktop_r0n1n_to_cyrillic(r0n1n_match_query[0], qn[0], r0n1n_match_query[1]);
    qn[2] = desktop_r0n1n_to_latin(r0n1n_match_query[0], qn[0], r0n1n_match_query[2]);

    for(size_t v = 0; v < 3; v++) {
        const uint16_t* q = r0n1n_match_query[v];
        if(v > 0 && qn[v] == qn[0] && memcmp(q, r0n1n_match_query[0], qn[0] * 2) == 0) {
            continue; // nothing to transliterate
        }
        for(size_t s = 0; s < tn; s++) {
            if(desktop_r0n1n_prefix(r0n1n_match_text + s, tn - s, q, qn[v])) return true;
        }
        if(qn[v] < R0N1N_MATCH_FUZZY) continue;
        for(size_t s = 0; s < tn; s++) {
            if(s > 0 && desktop_r0n1n_is_word_char(r0n1n_match_text[s - 1])) continue;
            if(desktop_r0n1n_near_prefix(r0n1n_match_text + s, tn - s, q, qn[v])) return true;
        }
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
        desktop_scene_r0n1n_settings_open(desktop, args);
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

#define R0N1N_INDEX_DIR    EXT_PATH(".r0n1n")
#define R0N1N_INDEX_PATH   R0N1N_INDEX_DIR "/captures.idx"
#define R0N1N_HISTORY_PATH R0N1N_INDEX_DIR "/searches.txt"

// Recent searches: one query per line, newest first. Without an SD card they
// still last until reboot.
static void desktop_r0n1n_history_load(Desktop* desktop) {
    if(desktop->search_history_loaded) return;
    desktop->search_history_loaded = true;
    desktop->search_history_count = 0;

    char buf[R0N1N_HISTORY_MAX * R0N1N_QUERY_SIZE];
    File* in = storage_file_alloc(desktop->storage);
    size_t size = 0;
    if(storage_file_open(in, R0N1N_HISTORY_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        size = storage_file_read(in, buf, sizeof(buf) - 1);
    }
    storage_file_close(in);
    storage_file_free(in);
    buf[size] = 0;

    for(char* line = buf; *line && desktop->search_history_count < R0N1N_HISTORY_MAX;) {
        char* end = strchr(line, '\n');
        if(end) *end = 0;
        if(*line) {
            strlcpy(
                desktop->search_history[desktop->search_history_count++],
                line,
                R0N1N_QUERY_SIZE);
        }
        if(!end) break;
        line = end + 1;
    }
}

void desktop_r0n1n_history_push(Desktop* desktop, const char* query) {
    if(!query || !query[0]) return;
    desktop_r0n1n_history_load(desktop);

    // Drop an older copy, shift down, put the query on top
    uint8_t count = desktop->search_history_count;
    for(uint8_t i = 0; i < count; i++) {
        if(strcmp(desktop->search_history[i], query) == 0) {
            memmove(
                desktop->search_history[i],
                desktop->search_history[i + 1],
                (count - i - 1) * R0N1N_QUERY_SIZE);
            count--;
            break;
        }
    }
    if(count == R0N1N_HISTORY_MAX) count--;
    memmove(desktop->search_history[1], desktop->search_history[0], count * R0N1N_QUERY_SIZE);
    strlcpy(desktop->search_history[0], query, R0N1N_QUERY_SIZE);
    desktop->search_history_count = count + 1;

    // Saving bumps the SD change counter; don't let that make a fresh capture
    // index look stale (it would be rebuilt on every search).
    uint32_t before = 0;
    storage_common_timestamp(desktop->storage, EXT_PATH(""), &before);
    const bool index_fresh = desktop->capture_index_token &&
                             desktop->capture_index_token == before;

    storage_simply_mkdir(desktop->storage, R0N1N_INDEX_DIR);
    File* out = storage_file_alloc(desktop->storage);
    if(storage_file_open(out, R0N1N_HISTORY_PATH, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        for(uint8_t i = 0; i < desktop->search_history_count; i++) {
            storage_file_write(
                out, desktop->search_history[i], strlen(desktop->search_history[i]));
            storage_file_write(out, "\n", 1);
        }
    }
    storage_file_close(out);
    storage_file_free(out);

    if(index_fresh) {
        storage_common_timestamp(desktop->storage, EXT_PATH(""), &desktop->capture_index_token);
    }
}

void desktop_r0n1n_open_search(Desktop* desktop) {
    desktop_r0n1n_history_load(desktop);
    desktop->search_query[0] = 0;
    scene_manager_next_scene(
        desktop->scene_manager,
        desktop->search_history_count ? DesktopSceneSearchHistory : DesktopSceneSearch);
}

// One capture as an index line: "<type>\t<timestamp>\t<label>\t<path>".
// Reading the index avoids re-walking every capture directory and stat-ing
// every file on each search.
static void desktop_r0n1n_capture_write_line(
    File* out,
    size_t type,
    uint32_t timestamp,
    const char* label,
    const char* path) {
    FuriString* line =
        furi_string_alloc_printf("%u\t%lu\t%s\t%s\n", (unsigned)type, timestamp, label, path);
    storage_file_write(out, furi_string_get_cstr(line), furi_string_size(line));
    furi_string_free(line);
}

// Walk the capture directories and (re)write the index; returns success.
static bool desktop_r0n1n_capture_reindex(Desktop* desktop) {
    storage_common_mkdir(desktop->storage, R0N1N_INDEX_DIR);
    File* out = storage_file_alloc(desktop->storage);
    if(!storage_file_open(out, R0N1N_INDEX_PATH, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        storage_file_free(out);
        return false;
    }
    File* dir = storage_file_alloc(desktop->storage);
    FuriString* path = furi_string_alloc();
    char name[64];
    FileInfo info;
    for(size_t t = 0; t < COUNT_OF(r0n1n_capture_types); t++) {
        const R0n1nCaptureType* type = &r0n1n_capture_types[t];
        if(!storage_dir_open(dir, type->dir)) {
            storage_dir_close(dir);
            continue;
        }
        const size_t ext_len = strlen(type->extension);
        while(storage_dir_read(dir, &info, name, sizeof(name))) {
            if(file_info_is_dir(&info) || name[0] == '.') continue;
            const size_t len = strlen(name);
            if(len <= ext_len || strcmp(name + len - ext_len, type->extension) != 0) continue;
            furi_string_printf(path, "%s/%s", type->dir, name);
            uint32_t timestamp = 0;
            storage_common_mtime(desktop->storage, furi_string_get_cstr(path), &timestamp);
            name[len - ext_len] = '\0'; // label without extension
            desktop_r0n1n_capture_write_line(out, t, timestamp, name, furi_string_get_cstr(path));
        }
        storage_dir_close(dir);
    }
    furi_string_free(path);
    storage_file_free(dir);
    storage_file_free(out);
    return true;
}

// Rebuild the index only if the SD card changed since it was built. The
// change counter is read AFTER building and kept in RAM, so the index's own
// write doesn't invalidate it; any later write elsewhere (a saved or deleted
// capture) does, and the next search rebuilds once.
static void desktop_r0n1n_capture_ensure_index(Desktop* desktop) {
    uint32_t token = 0;
    storage_common_timestamp(desktop->storage, EXT_PATH(""), &token);
    if(desktop->capture_index_token != 0 && desktop->capture_index_token == token &&
       storage_file_exists(desktop->storage, R0N1N_INDEX_PATH)) {
        return;
    }
    if(desktop_r0n1n_capture_reindex(desktop)) {
        storage_common_timestamp(desktop->storage, EXT_PATH(""), &desktop->capture_index_token);
    }
}

void desktop_r0n1n_scan_captures(Desktop* desktop, const char* filter) {
    desktop_r0n1n_capture_ensure_index(desktop);

    File* in = storage_file_alloc(desktop->storage);
    if(!storage_file_open(in, R0N1N_INDEX_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        storage_file_free(in);
        return;
    }

    FuriString* line = furi_string_alloc();
    char buf[256];
    size_t got;
    furi_string_reset(line);
    do {
        got = storage_file_read(in, buf, sizeof(buf));
        for(size_t i = 0; i <= got; i++) {
            const bool end = (i == got);
            if(!end && buf[i] != '\n') {
                furi_string_push_back(line, buf[i]);
                continue;
            }
            if(end) break; // continue accumulating from the next chunk
            if(furi_string_size(line)) {
                // type \t timestamp \t label \t path
                const char* s = furi_string_get_cstr(line);
                const char* p1 = strchr(s, '\t');
                const char* p2 = p1 ? strchr(p1 + 1, '\t') : NULL;
                const char* p3 = p2 ? strchr(p2 + 1, '\t') : NULL;
                if(p1 && p2 && p3) {
                    const size_t type = (size_t)strtoul(s, NULL, 10);
                    const uint32_t timestamp = (uint32_t)strtoul(p1 + 1, NULL, 10);
                    FuriString* label = furi_string_alloc();
                    furi_string_set_strn(label, p2 + 1, (size_t)(p3 - (p2 + 1)));
                    if(type < COUNT_OF(r0n1n_capture_types) &&
                       desktop_r0n1n_matches(furi_string_get_cstr(label), filter)) {
                        desktop_r0n1n_entries_add(
                            desktop,
                            r0n1n_capture_types[type].app,
                            p3 + 1,
                            furi_string_get_cstr(label),
                            r0n1n_capture_types[type].icon,
                            timestamp);
                    }
                    furi_string_free(label);
                }
            }
            furi_string_reset(line);
        }
    } while(got == sizeof(buf));

    furi_string_free(line);
    storage_file_free(in);
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
    desktop_r0n1n_feedback(desktop, R0n1nFeedbackError);
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

// R0N1N feedback: a short click on select, plus success/error cues, all
// gated by the feedback setting. Sound and vibro further honour the
// Control Center toggles and stealth mode (the notification service does
// that itself), so a silent Flipper stays silent.
static const NotificationSequence r0n1n_seq_click = {
    &message_vibro_on,
    &message_note_g5,
    &message_delay_25,
    &message_sound_off,
    &message_vibro_off,
    NULL,
};

void desktop_r0n1n_feedback(Desktop* desktop, R0n1nFeedback kind) {
    if(!desktop->r0n1n.feedback) return;
    const NotificationSequence* seq = &r0n1n_seq_click;
    if(kind == R0n1nFeedbackSuccess) {
        seq = &sequence_success;
    } else if(kind == R0n1nFeedbackError) {
        seq = &sequence_error;
    }
    notification_message(desktop->notification, seq);
}
