/**
 * @file r0n1n_catalog.h
 * What the R0N1N shell knows about: built-in apps grouped into sections,
 * the four profiles (which sections they show, in what order), the Dev
 * tool grid, and Russian labels for the stock settings apps.
 */
#pragma once

#include <gui/icon.h>
#include <stddef.h>
#include <stdint.h>

#include "r0n1n_settings.h"

/* Targets that aren't Loader app names (see desktop_r0n1n_launch()). */
#define R0N1N_APP_ARCHIVE  "@archive"
#define R0N1N_APP_SETTINGS "@settings"
#define R0N1N_APP_ALL      "@all"

typedef struct {
    const char* name; // Loader name, .fap path, or one of the R0N1N_APP_* targets
    const char* label;
    const Icon* icon; // 10 px, for lists
    const Icon* tile_icon; // 14 px, for Quick Actions tiles
} R0n1nApp;

typedef enum {
    R0n1nSectionRadio,
    R0n1nSectionCards,
    R0n1nSectionIr,
    R0n1nSectionUsb,
    R0n1nSectionBluetooth,
    R0n1nSectionDev,
    R0n1nSectionTools,
    R0n1nSectionGames,
    R0n1nSectionMedia,
    R0n1nSectionOther,
    R0n1nSectionCount,
} R0n1nSection;

typedef struct {
    const char* short_label; // carousel tile
    const char* title; // list header / menu row
    const Icon* icon;
    const Icon* tile_icon;
    const R0n1nApp* apps; // built into the firmware or shipped with it
    uint8_t app_count;
    // Folders under /ext/apps whose apps also belong here (NULL-terminated);
    // the Other section takes the folders no section names.
    const char* const* sd_dirs;
} R0n1nSectionInfo;

typedef struct {
    const char* name;
    const char* description;
    const Icon* icon;
    uint8_t section_count;
    R0n1nSection sections[R0n1nSectionCount];
} R0n1nProfileInfo;

extern const R0n1nSectionInfo r0n1n_sections[R0n1nSectionCount];
extern const R0n1nProfileInfo r0n1n_profiles[R0n1nProfileCount];
extern const R0n1nApp r0n1n_system_apps[];
extern const size_t r0n1n_system_apps_count;

/** Known app by Loader name / target, or NULL. */
const R0n1nApp* r0n1n_catalog_find(const char* name);

/** Russian label for a stock settings app name, or the name itself. */
const char* r0n1n_catalog_settings_label(const char* name);

/** Icon for a stock settings app name. */
const Icon* r0n1n_catalog_settings_icon(const char* name);
