#include "slix_render.h"

void nfc_render_slix_info(const SlixData* data, NfcProtocolFormatType format_type, FuriString* str) {
    nfc_render_iso15693_3_brief(slix_get_base_data(data), str);

    if(format_type != NfcProtocolFormatTypeFull) return;
    const SlixType slix_type = slix_get_type(data);

    furi_string_cat(str, "\n:::::::::::::::::::[Пароли]:::::::::::::::::::\n");

    static const char* slix_password_names[] = {
        "Чтение",
        "Записать",
        "Приватность",
        "Уничтожить",
        "EAS/AFI",
    };

    for(uint32_t i = 0; i < SlixPasswordTypeCount; ++i) {
        if(slix_type_supports_password(slix_type, i)) {
            furi_string_cat_printf(
                str, "%s :  %08lX\n", slix_password_names[i], data->passwords[i]);
        }
    }

    furi_string_cat(str, ":::::::::::::::::[Биты блок.]:::::::::::::::::\n");

    if(slix_type_has_features(slix_type, SLIX_TYPE_FEATURE_EAS)) {
        furi_string_cat_printf(
            str, "EAS: %sзаблок.\n", data->system_info.lock_bits.eas ? "" : "не ");
    }

    if(slix_type_has_features(slix_type, SLIX_TYPE_FEATURE_PROTECTION)) {
        furi_string_cat_printf(
            str, "PPL: %sзаблок.\n", data->system_info.lock_bits.ppl ? "" : "не ");

        const SlixProtection protection = data->system_info.protection;

        furi_string_cat(str, "::::::::::::[Защита страниц]::::::::::::\n");
        furi_string_cat_printf(str, "Указатель: В >= %02X\n", protection.pointer);

        const char* rh = (protection.condition & SLIX_PP_CONDITION_RH) ? "" : "не ";
        const char* rl = (protection.condition & SLIX_PP_CONDITION_RL) ? "" : "не ";

        const char* wh = (protection.condition & SLIX_PP_CONDITION_WH) ? "" : "не ";
        const char* wl = (protection.condition & SLIX_PP_CONDITION_WL) ? "" : "не ";

        furi_string_cat_printf(str, "Чт: В %sзащ. Н %sзащ.\n", rh, rl);
        furi_string_cat_printf(str, "Зп: В %sзащ. Н %sзащ.\n", wh, wl);
    }

    if(slix_type_has_features(slix_type, SLIX_TYPE_FEATURE_PRIVACY)) {
        furi_string_cat(str, ":::::::::::::::::[Приватность]:::::::::::::::::\n");
        furi_string_cat_printf(str, "Приватность: %s\n", data->privacy ? "вкл" : "выкл");
    }

    if(slix_type_has_features(slix_type, SLIX_TYPE_FEATURE_SIGNATURE)) {
        furi_string_cat(str, ":::::::::::::::::::[Подпись]:::::::::::::::::::\n");
        for(uint32_t i = 0; i < 4; ++i) {
            furi_string_cat_printf(str, "%02X ", data->signature[i]);
        }

        furi_string_cat(str, "[ ... ]");

        for(uint32_t i = 0; i < 3; ++i) {
            furi_string_cat_printf(str, " %02X", data->signature[sizeof(SlixSignature) - i - 1]);
        }
    }

    furi_string_cat(str, "\n\e#Данные ISO15693-3");
    nfc_render_iso15693_3_extra(slix_get_base_data(data), str);
}
