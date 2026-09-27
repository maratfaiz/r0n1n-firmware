#include "r0n1n_catalog.h"

#include <assets_icons.h>
#include <furi.h>
#include <storage/storage.h>

static const R0n1nApp r0n1n_apps_radio[] = {
    {"Sub-GHz", "Sub-GHz", &I_sub1_10px, &A_Sub1ghz_14},
};

static const R0n1nApp r0n1n_apps_cards[] = {
    {"NFC", "NFC", &I_Nfc_10px, &A_NFC_14},
    {"125 kHz RFID", "RFID 125 кГц", &I_125_10px, &A_125khz_14},
    {"iButton", "iButton", &I_ibutt_10px, &A_iButton_14},
};

static const R0n1nApp r0n1n_apps_ir[] = {
    {"Infrared", "ИК-пульт", &I_ir_10px, &A_Infrared_14},
};

static const R0n1nApp r0n1n_apps_usb[] = {
    {"Bad USB", "BadUSB", &I_badusb_10px, &A_BadUsb_14},
    {"U2F", "U2F", &I_u2f_10px, &A_U2F_14},
    {EXT_PATH("apps/USB/hid_usb.fap"), "Пульт USB", &I_badusb_10px, &A_BadUsb_14},
};

static const R0n1nApp r0n1n_apps_bluetooth[] = {
    {EXT_PATH("apps/Bluetooth/hid_ble.fap"),
     "Пульт Bluetooth",
     &I_R_Bluetooth_7x9,
     &I_R_TileBt_14x14},
};

static const R0n1nApp r0n1n_apps_dev[] = {
    {"GPIO", "GPIO", &I_R_Chip_9x7, &A_GPIO_14},
};

static const R0n1nApp r0n1n_apps_tools[] = {
    {"JS Runner", "Скрипты JS", &I_js_script_10px, &A_Plugins_14},
    {EXT_PATH("apps/Tools/academy.fap"), "Академия", &I_R_Cap_10x7, &I_R_TileTools_14x14},
};

const R0n1nApp r0n1n_system_apps[] = {
    {R0N1N_APP_ARCHIVE, "Файлы", &I_dir_10px, &A_FileManager_14},
    {R0N1N_APP_SETTINGS, "Настройки", &I_settings_10px, &A_Settings_14},
    {R0N1N_APP_ALL, "Все приложения", &I_R_Star_9x7, &A_Plugins_14},
    {"academy.fap", "Академия", &I_R_Cap_10x7, &A_Plugins_14},
};
const size_t r0n1n_system_apps_count = COUNT_OF(r0n1n_system_apps);

static const char* const r0n1n_dirs_radio[] = {"Sub-GHz", NULL};
static const char* const r0n1n_dirs_cards[] = {"NFC", "RFID", "iButton", NULL};
static const char* const r0n1n_dirs_ir[] = {"Infrared", NULL};
static const char* const r0n1n_dirs_usb[] = {"USB", NULL};
static const char* const r0n1n_dirs_bluetooth[] = {"Bluetooth", NULL};
static const char* const r0n1n_dirs_dev[] = {"GPIO", NULL};
static const char* const r0n1n_dirs_tools[] = {"Tools", "Scripts", NULL};
static const char* const r0n1n_dirs_games[] = {"Games", NULL};
static const char* const r0n1n_dirs_media[] = {"Media", NULL};

#define R0N1N_SECTION_APPS(apps) apps, COUNT_OF(apps)

const R0n1nSectionInfo r0n1n_sections[R0n1nSectionCount] = {
    [R0n1nSectionRadio] =
        {"Эфир",
         "Радио",
         &I_sub1_10px,
         &A_Sub1ghz_14,
         R0N1N_SECTION_APPS(r0n1n_apps_radio),
         r0n1n_dirs_radio},
    [R0n1nSectionCards] =
        {"NFC",
         "Карты: NFC / RFID",
         &I_Nfc_10px,
         &A_NFC_14,
         R0N1N_SECTION_APPS(r0n1n_apps_cards),
         r0n1n_dirs_cards},
    [R0n1nSectionIr] =
        {"ИК",
         "ИК-порт",
         &I_ir_10px,
         &A_Infrared_14,
         R0N1N_SECTION_APPS(r0n1n_apps_ir),
         r0n1n_dirs_ir},
    [R0n1nSectionUsb] =
        {"USB",
         "USB",
         &I_badusb_10px,
         &A_BadUsb_14,
         R0N1N_SECTION_APPS(r0n1n_apps_usb),
         r0n1n_dirs_usb},
    [R0n1nSectionBluetooth] =
        {"BT",
         "Bluetooth",
         &I_R_Bluetooth_7x9,
         &I_R_TileBt_14x14,
         R0N1N_SECTION_APPS(r0n1n_apps_bluetooth),
         r0n1n_dirs_bluetooth},
    [R0n1nSectionDev] =
        {"GPIO",
         "GPIO и модули",
         &I_R_Chip_9x7,
         &A_GPIO_14,
         R0N1N_SECTION_APPS(r0n1n_apps_dev),
         r0n1n_dirs_dev},
    [R0n1nSectionTools] =
        {"Инстр.",
         "Инструменты",
         &I_R_Gear_9x7,
         &I_R_TileTools_14x14,
         R0N1N_SECTION_APPS(r0n1n_apps_tools),
         r0n1n_dirs_tools},
    [R0n1nSectionGames] =
        {"Игры", "Игры", &I_R_App_9x7, &I_R_TileGames_14x14, NULL, 0, r0n1n_dirs_games},
    [R0n1nSectionMedia] =
        {"Медиа", "Медиа", &I_R_Sound_9x8, &I_R_TileMedia_14x14, NULL, 0, r0n1n_dirs_media},
    [R0n1nSectionOther] = {"Другое", "Другое", &I_dir_10px, &I_R_TileOther_14x14, NULL, 0, NULL},
};

const R0n1nProfileInfo r0n1n_profiles[R0n1nProfileCount] = {
    [R0n1nProfileEveryday] =
        {"Обычный",
         "Повседневные задачи",
         &I_R_Brightness_9x9,
         7,
         {R0n1nSectionRadio,
          R0n1nSectionCards,
          R0n1nSectionIr,
          R0n1nSectionUsb,
          R0n1nSectionTools,
          R0n1nSectionGames,
          R0n1nSectionMedia}},
    [R0n1nProfilePentest] =
        {"Пентест",
         "Инструменты безопасности",
         &I_R_Pentest_9x7,
         7,
         {R0n1nSectionRadio,
          R0n1nSectionCards,
          R0n1nSectionUsb,
          R0n1nSectionIr,
          R0n1nSectionBluetooth,
          R0n1nSectionDev,
          R0n1nSectionTools}},
    [R0n1nProfileDev] =
        {"Разраб.",
         "Разработка и отладка",
         &I_R_Chip_9x7,
         6,
         {R0n1nSectionDev,
          R0n1nSectionTools,
          R0n1nSectionUsb,
          R0n1nSectionBluetooth,
          R0n1nSectionRadio,
          R0n1nSectionCards}},
    [R0n1nProfileCtf] =
        {"CTF",
         "Лаборатории и практики",
         &I_R_Flag_7x7,
         8,
         {R0n1nSectionCards,
          R0n1nSectionRadio,
          R0n1nSectionIr,
          R0n1nSectionUsb,
          R0n1nSectionBluetooth,
          R0n1nSectionDev,
          R0n1nSectionTools,
          R0n1nSectionGames}},
};

const R0n1nApp* r0n1n_catalog_find(const char* name) {
    for(size_t s = 0; s < R0n1nSectionCount; s++) {
        for(size_t i = 0; i < r0n1n_sections[s].app_count; i++) {
            if(strcmp(r0n1n_sections[s].apps[i].name, name) == 0)
                return &r0n1n_sections[s].apps[i];
        }
    }
    for(size_t i = 0; i < r0n1n_system_apps_count; i++) {
        if(strcmp(r0n1n_system_apps[i].name, name) == 0) return &r0n1n_system_apps[i];
    }
    return NULL;
}

static const struct {
    const char* name;
    const char* label;
    const Icon* icon;
} r0n1n_settings_labels[] = {
    {"Bluetooth", "Bluetooth", &I_R_Bluetooth_7x9},
    {"LCD and Notifications", "Экран и звук", &I_R_Display_9x6},
    {"Storage", "Хранилище", &I_dir_10px},
    {"Power", "Питание", &I_R_Brightness_9x9},
    {"Desktop", "Рабочий стол", &I_R_Star_9x7},
    {"System", "Система", &I_settings_10px},
    {"Clock & Alarm", "Часы и будильник", &I_R_Clock_9x8},
    {"Expansion Modules", "Внешние модули", &I_R_Module_9x7},
    {"Passport", "Паспорт", &I_R_Profile_7x7},
    {"About", "Об устройстве", &I_R_Check_7x6},
};

const char* r0n1n_catalog_settings_label(const char* name) {
    for(size_t i = 0; i < COUNT_OF(r0n1n_settings_labels); i++) {
        if(strcmp(r0n1n_settings_labels[i].name, name) == 0) return r0n1n_settings_labels[i].label;
    }
    return name;
}

const Icon* r0n1n_catalog_settings_icon(const char* name) {
    for(size_t i = 0; i < COUNT_OF(r0n1n_settings_labels); i++) {
        if(strcmp(r0n1n_settings_labels[i].name, name) == 0) return r0n1n_settings_labels[i].icon;
    }
    return &I_settings_10px;
}
