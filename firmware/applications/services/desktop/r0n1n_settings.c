#include "r0n1n_settings.h"
#include "r0n1n_catalog.h"

#include <furi.h>
#include <saved_struct.h>
#include <storage/storage.h>

#define TAG "R0n1nSettings"

#define R0N1N_SETTINGS_PATH  INT_PATH(".r0n1n.settings")
#define R0N1N_SETTINGS_MAGIC (0x52)
#define R0N1N_SETTINGS_VER   (3)

static const char* const r0n1n_quick_defaults[R0N1N_QUICK_SLOTS] = {
    "NFC",
    "Sub-GHz",
    "Infrared",
    "Bad USB",
    R0N1N_APP_ARCHIVE,
    R0N1N_APP_SETTINGS,
};

// Version 1 had no simple_mode; its profile and slots are kept on upgrade
typedef struct {
    uint8_t profile;
    char quick[R0N1N_QUICK_SLOTS][R0N1N_SLOT_NAME_SIZE];
} R0n1nSettingsV1;

static bool r0n1n_settings_load_v1(R0n1nSettings* settings) {
    R0n1nSettingsV1* v1 = malloc(sizeof(R0n1nSettingsV1));
    bool loaded = saved_struct_load(
        R0N1N_SETTINGS_PATH, v1, sizeof(R0n1nSettingsV1), R0N1N_SETTINGS_MAGIC, 1);
    if(loaded) {
        memset(settings, 0, sizeof(R0n1nSettings));
        settings->profile = v1->profile;
        memcpy(settings->quick, v1->quick, sizeof(settings->quick));
        settings->feedback = true;
    }
    free(v1);
    return loaded;
}

// Version 2 had no feedback flag; the rest is kept on upgrade
typedef struct {
    uint8_t profile;
    char quick[R0N1N_QUICK_SLOTS][R0N1N_SLOT_NAME_SIZE];
    bool simple_mode;
} R0n1nSettingsV2;

static bool r0n1n_settings_load_v2(R0n1nSettings* settings) {
    R0n1nSettingsV2* v2 = malloc(sizeof(R0n1nSettingsV2));
    bool loaded = saved_struct_load(
        R0N1N_SETTINGS_PATH, v2, sizeof(R0n1nSettingsV2), R0N1N_SETTINGS_MAGIC, 2);
    if(loaded) {
        memset(settings, 0, sizeof(R0n1nSettings));
        settings->profile = v2->profile;
        memcpy(settings->quick, v2->quick, sizeof(settings->quick));
        settings->simple_mode = v2->simple_mode;
        settings->feedback = true;
    }
    free(v2);
    return loaded;
}

void r0n1n_settings_load(R0n1nSettings* settings) {
    furi_assert(settings);
    if(!saved_struct_load(
           R0N1N_SETTINGS_PATH,
           settings,
           sizeof(R0n1nSettings),
           R0N1N_SETTINGS_MAGIC,
           R0N1N_SETTINGS_VER) &&
       !r0n1n_settings_load_v2(settings) && !r0n1n_settings_load_v1(settings)) {
        FURI_LOG_I(TAG, "No settings, using defaults");
        memset(settings, 0, sizeof(R0n1nSettings));
        settings->profile = R0n1nProfileEveryday;
        settings->feedback = true;
        for(size_t i = 0; i < R0N1N_QUICK_SLOTS; i++) {
            strlcpy(settings->quick[i], r0n1n_quick_defaults[i], R0N1N_SLOT_NAME_SIZE);
        }
    }
    if(settings->profile >= R0n1nProfileCount) settings->profile = R0n1nProfileEveryday;
}

void r0n1n_settings_save(const R0n1nSettings* settings) {
    furi_assert(settings);
    if(!saved_struct_save(
           R0N1N_SETTINGS_PATH,
           settings,
           sizeof(R0n1nSettings),
           R0N1N_SETTINGS_MAGIC,
           R0N1N_SETTINGS_VER)) {
        FURI_LOG_E(TAG, "Failed to save settings");
    }
}
