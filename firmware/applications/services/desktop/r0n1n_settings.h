/**
 * @file r0n1n_settings.h
 * R0N1N shell settings: the active profile and the six Quick Actions slots.
 * Stored separately from DesktopSettings so the stock settings file format
 * (and the Desktop settings app that edits it) stays untouched.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#define R0N1N_QUICK_SLOTS    6
#define R0N1N_SLOT_NAME_SIZE 128

typedef enum {
    R0n1nProfileEveryday,
    R0n1nProfilePentest,
    R0n1nProfileDev,
    R0n1nProfileCtf,
    R0n1nProfileCount,
} R0n1nProfile;

typedef struct {
    uint8_t profile;
    char quick[R0N1N_QUICK_SLOTS][R0N1N_SLOT_NAME_SIZE];
    bool simple_mode; // big-icon Home and menu with the basic functions only
    bool feedback; // click/vibro on OK and on success/error, default on
    uint8_t wallpaper; // index into r0n1n_wallpapers (Home picture)
} R0n1nSettings;

void r0n1n_settings_load(R0n1nSettings* settings);
void r0n1n_settings_save(const R0n1nSettings* settings);
