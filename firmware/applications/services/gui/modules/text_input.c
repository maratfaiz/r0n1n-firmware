#include "text_input.h"
#include <gui/elements.h>
#include <gui/utf8_i.h>
#include "text_input_i.h"
#include <assets_icons.h>
#include <furi.h>

struct TextInput {
    View* view;
    FuriTimer* timer;
};

// R0N1N keyboard: three layouts cycled with one key (Russian, Latin,
// digits/symbols). Letters take the left part of each row; the right
// column holds Backspace, the layout key and Save. Text is UTF-8 and file
// names may be Russian (the SD card works in code page 866, storage_ext.c);
// a caller that needs ASCII can have Cyrillic transliterated to Latin on
// Save with text_input_set_allow_unicode(false).
typedef struct {
    const uint16_t code;
    const uint8_t x;
    const uint8_t y;
} TextInputKey;

typedef enum {
    TextInputLayoutRu,
    TextInputLayoutEn,
    TextInputLayoutNum,
    TextInputLayoutCount,
} TextInputLayout;

typedef struct {
    const char* header;
    char* text_buffer;
    size_t text_buffer_size;
    size_t minimum_length;
    bool clear_default_text;
    bool allow_unicode;
    TextInputLayout layout;

    TextInputCallback callback;
    void* callback_context;

    uint8_t selected_row;
    uint8_t selected_column;

    TextInputValidatorCallback validator_callback;
    void* validator_callback_context;
    FuriString* validator_text;
    bool validator_message_visible;
} TextInputModel;

static const uint8_t keyboard_origin_x = 1;
static const uint8_t keyboard_origin_y = 29;
static const uint8_t keyboard_row_count = 3;

#define ENTER_KEY     '\r'
#define BACKSPACE_KEY '\b'
#define LAYOUT_KEY    '\t'

// The layout new keyboards open with: the one used last
static TextInputLayout text_input_last_layout = TextInputLayoutRu;

#define KEY_RU(c, i, row)  {c, 1 + 9 * (i), 8 + 12 * (row)}
#define KEY_EN(c, i, row)  {c, 1 + 10 * (i), 8 + 12 * (row)}
#define KEYS_SPECIAL_ROW_1 {BACKSPACE_KEY, 106, 0}
#define KEYS_SPECIAL_ROW_2 {LAYOUT_KEY, 103, 20}
#define KEYS_SPECIAL_ROW_3 {ENTER_KEY, 103, 23}

static const TextInputKey keys_ru_1[] = {
    KEY_RU(0x439, 0, 0),
    KEY_RU(0x446, 1, 0),
    KEY_RU(0x443, 2, 0),
    KEY_RU(0x43A, 3, 0),
    KEY_RU(0x435, 4, 0),
    KEY_RU(0x43D, 5, 0),
    KEY_RU(0x433, 6, 0),
    KEY_RU(0x448, 7, 0),
    KEY_RU(0x449, 8, 0),
    KEY_RU(0x437, 9, 0),
    KEY_RU(0x445, 10, 0),
    KEYS_SPECIAL_ROW_1,
}; // й ц у к е н г ш щ з х
static const TextInputKey keys_ru_2[] = {
    KEY_RU(0x444, 0, 1),
    KEY_RU(0x44B, 1, 1),
    KEY_RU(0x432, 2, 1),
    KEY_RU(0x430, 3, 1),
    KEY_RU(0x43F, 4, 1),
    KEY_RU(0x440, 5, 1),
    KEY_RU(0x43E, 6, 1),
    KEY_RU(0x43B, 7, 1),
    KEY_RU(0x434, 8, 1),
    KEY_RU(0x436, 9, 1),
    KEY_RU(0x44D, 10, 1),
    KEYS_SPECIAL_ROW_2,
}; // ф ы в а п р о л д ж э
static const TextInputKey keys_ru_3[] = {
    KEY_RU(0x44F, 0, 2),
    KEY_RU(0x447, 1, 2),
    KEY_RU(0x441, 2, 2),
    KEY_RU(0x43C, 3, 2),
    KEY_RU(0x438, 4, 2),
    KEY_RU(0x442, 5, 2),
    KEY_RU(0x44C, 6, 2),
    KEY_RU(0x431, 7, 2),
    KEY_RU(0x44E, 8, 2),
    KEY_RU(0x44A, 9, 2),
    KEY_RU('_', 10, 2),
    KEYS_SPECIAL_ROW_3,
}; // я ч с м и т ь б ю ъ _

static const TextInputKey keys_en_1[] = {
    KEY_EN('q', 0, 0),
    KEY_EN('w', 1, 0),
    KEY_EN('e', 2, 0),
    KEY_EN('r', 3, 0),
    KEY_EN('t', 4, 0),
    KEY_EN('y', 5, 0),
    KEY_EN('u', 6, 0),
    KEY_EN('i', 7, 0),
    KEY_EN('o', 8, 0),
    KEY_EN('p', 9, 0),
    KEYS_SPECIAL_ROW_1,
};
static const TextInputKey keys_en_2[] = {
    KEY_EN('a', 0, 1),
    KEY_EN('s', 1, 1),
    KEY_EN('d', 2, 1),
    KEY_EN('f', 3, 1),
    KEY_EN('g', 4, 1),
    KEY_EN('h', 5, 1),
    KEY_EN('j', 6, 1),
    KEY_EN('k', 7, 1),
    KEY_EN('l', 8, 1),
    KEYS_SPECIAL_ROW_2,
};
static const TextInputKey keys_en_3[] = {
    KEY_EN('z', 0, 2),
    KEY_EN('x', 1, 2),
    KEY_EN('c', 2, 2),
    KEY_EN('v', 3, 2),
    KEY_EN('b', 4, 2),
    KEY_EN('n', 5, 2),
    KEY_EN('m', 6, 2),
    KEY_EN('_', 7, 2),
    KEYS_SPECIAL_ROW_3,
};

static const TextInputKey keys_num_1[] = {
    KEY_EN('1', 0, 0),
    KEY_EN('2', 1, 0),
    KEY_EN('3', 2, 0),
    KEY_EN('4', 3, 0),
    KEY_EN('5', 4, 0),
    KEY_EN('6', 5, 0),
    KEY_EN('7', 6, 0),
    KEY_EN('8', 7, 0),
    KEY_EN('9', 8, 0),
    KEY_EN('0', 9, 0),
    KEYS_SPECIAL_ROW_1,
};
static const TextInputKey keys_num_2[] = {
    KEY_EN('-', 0, 1),
    KEY_EN('+', 1, 1),
    KEY_EN('=', 2, 1),
    KEY_EN('!', 3, 1),
    KEY_EN('#', 4, 1),
    KEY_EN('$', 5, 1),
    KEY_EN('%', 6, 1),
    KEY_EN('&', 7, 1),
    KEY_EN('@', 8, 1),
    KEYS_SPECIAL_ROW_2,
};
static const TextInputKey keys_num_3[] = {
    KEY_EN('(', 0, 2),
    KEY_EN(')', 1, 2),
    KEY_EN('\'', 2, 2),
    KEY_EN('_', 3, 2),
    KEYS_SPECIAL_ROW_3,
};

typedef struct {
    const TextInputKey* rows[3];
    uint8_t sizes[3];
    const char* next_label; // shown on the layout key: where it leads
} TextInputLayoutInfo;

static const TextInputLayoutInfo text_input_layouts[TextInputLayoutCount] = {
    [TextInputLayoutRu] =
        {{keys_ru_1, keys_ru_2, keys_ru_3},
         {COUNT_OF(keys_ru_1), COUNT_OF(keys_ru_2), COUNT_OF(keys_ru_3)},
         "ENG"},
    [TextInputLayoutEn] =
        {{keys_en_1, keys_en_2, keys_en_3},
         {COUNT_OF(keys_en_1), COUNT_OF(keys_en_2), COUNT_OF(keys_en_3)},
         "123"},
    [TextInputLayoutNum] =
        {{keys_num_1, keys_num_2, keys_num_3},
         {COUNT_OF(keys_num_1), COUNT_OF(keys_num_2), COUNT_OF(keys_num_3)},
         "РУС"},
};

static uint8_t get_row_size(const TextInputModel* model, uint8_t row_index) {
    furi_check(row_index < keyboard_row_count);
    return text_input_layouts[model->layout].sizes[row_index];
}

static const TextInputKey* get_row(const TextInputModel* model, uint8_t row_index) {
    furi_check(row_index < keyboard_row_count);
    return text_input_layouts[model->layout].rows[row_index];
}

static uint16_t get_selected_char(TextInputModel* model) {
    return get_row(model, model->selected_row)[model->selected_column].code;
}

static bool char_is_lowercase(uint16_t letter) {
    return (letter >= 0x61 && letter <= 0x7A) || (letter >= 0x430 && letter <= 0x44F);
}

static uint16_t char_to_uppercase(const uint16_t letter) {
    if(letter == '_') {
        return 0x20;
    } else if(letter >= 0x61 && letter <= 0x7A) {
        return letter - 0x20;
    } else if(letter >= 0x430 && letter <= 0x44F) {
        return letter - 0x20;
    } else {
        return letter;
    }
}

// Byte offset where the last UTF-8 character of `text` starts
static size_t text_input_last_char_start(const char* text, size_t length) {
    return length ? gui_utf8_char_start(text, length - 1) : 0;
}

static void text_input_backspace_cb(TextInputModel* model) {
    if(model->clear_default_text) {
        model->text_buffer[0] = 0;
        return;
    }
    size_t text_length = strlen(model->text_buffer);
    model->text_buffer[text_input_last_char_start(model->text_buffer, text_length)] = 0;
}

// Transliteration of the Russian alphabet (а..я), for names that must stay ASCII
static const char* const text_input_translit[32] = {
    "a", "b", "v", "g", "d", "e",  "zh", "z",  "i",  "y",    "k", "l", "m", "n", "o",  "p",
    "r", "s", "t", "u", "f", "kh", "ts", "ch", "sh", "shch", "",  "y", "",  "e", "yu", "ya",
};

static void text_input_transliterate(char* text, size_t size) {
    char* out = malloc(size);
    size_t o = 0;
    for(const char* p = text; *p && o + 1 < size;) {
        uint16_t code;
        p += gui_utf8_char(p, &code);
        if(code < 0x80) {
            out[o++] = (char)code;
            continue;
        }
        bool upper = code >= 0x410 && code <= 0x42F;
        if(upper) code += 0x20;
        if(code == 0x451) code = 0x435; // ё -> е
        if(code < 0x430 || code > 0x44F) continue; // not representable: dropped
        const char* latin = text_input_translit[code - 0x430];
        for(size_t i = 0; latin[i] && o + 1 < size; i++) {
            out[o++] = (upper && i == 0) ? (char)toupper((unsigned char)latin[i]) : latin[i];
        }
    }
    out[o] = 0;
    strlcpy(text, out, size);
    free(out);
}

static void text_input_view_draw_callback(Canvas* canvas, void* _model) {
    TextInputModel* model = _model;
    uint8_t text_length = model->text_buffer ? strlen(model->text_buffer) : 0;
    uint8_t needed_string_width = canvas_width(canvas) - 8;
    uint8_t start_pos = 4;

    const char* text = model->text_buffer;

    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);

    canvas_draw_str(canvas, 2, 8, model->header);
    elements_slightly_rounded_frame(canvas, 1, 12, 126, 15);

    if(canvas_string_width(canvas, text) > needed_string_width) {
        canvas_draw_str(canvas, start_pos, 22, "...");
        start_pos += 6;
        needed_string_width -= 8;
    }

    // Drop whole characters from the front until the tail fits
    while(text && *text && canvas_string_width(canvas, text) > needed_string_width) {
        uint16_t code;
        text += gui_utf8_char(text, &code);
    }

    if(model->clear_default_text) {
        elements_slightly_rounded_box(
            canvas, start_pos - 1, 14, canvas_string_width(canvas, text) + 2, 10);
        canvas_set_color(canvas, ColorWhite);
    } else {
        canvas_draw_str(canvas, start_pos + canvas_string_width(canvas, text) + 1, 22, "|");
        canvas_draw_str(canvas, start_pos + canvas_string_width(canvas, text) + 2, 22, "|");
    }
    canvas_draw_str(canvas, start_pos, 22, text);

    // Latin keys in the stock keyboard font, Cyrillic in the one that has it
    const bool cyrillic = model->layout == TextInputLayoutRu;
    const Font key_font = cyrillic ? FontSecondary : FontKeyboard;
    const uint8_t key_box_width = cyrillic ? 8 : 7;

    for(uint8_t row = 0; row < keyboard_row_count; row++) {
        const uint8_t column_count = get_row_size(model, row);
        const TextInputKey* keys = get_row(model, row);

        for(size_t column = 0; column < column_count; column++) {
            const bool selected = model->selected_row == row && model->selected_column == column;
            const int32_t key_x = keyboard_origin_x + keys[column].x;
            const int32_t key_y = keyboard_origin_y + keys[column].y;
            canvas_set_color(canvas, ColorBlack);

            if(keys[column].code == ENTER_KEY || keys[column].code == LAYOUT_KEY) {
                // Key-shaped boxes: Save reads "ОК", the layout key shows
                // the layout it switches to
                const bool enter = keys[column].code == ENTER_KEY;
                const int32_t box_y = enter ? key_y : key_y - 9;
                canvas_set_font(canvas, FontSecondary);
                if(selected) {
                    canvas_draw_rbox(canvas, key_x, box_y, 24, 11, 2);
                    canvas_set_color(canvas, ColorWhite);
                } else {
                    canvas_draw_rframe(canvas, key_x, box_y, 24, 11, 2);
                }
                canvas_draw_str_aligned(
                    canvas,
                    key_x + 12,
                    box_y + 9,
                    AlignCenter,
                    AlignBottom,
                    enter ? "ОК" : text_input_layouts[model->layout].next_label);
            } else if(keys[column].code == BACKSPACE_KEY) {
                canvas_draw_icon(
                    canvas,
                    key_x,
                    key_y,
                    selected ? &I_KeyBackspaceSelected_16x9 : &I_KeyBackspace_16x9);
            } else {
                canvas_set_font(canvas, key_font);
                if(selected) {
                    canvas_draw_box(canvas, key_x - 1, key_y - 8, key_box_width, 10);
                    canvas_set_color(canvas, ColorWhite);
                }
                uint16_t code = keys[column].code;
                if(model->clear_default_text || (text_length == 0 && char_is_lowercase(code))) {
                    code = char_to_uppercase(code);
                }
                canvas_draw_glyph(canvas, key_x, key_y, code);
            }
        }
    }
    canvas_set_color(canvas, ColorBlack);
    if(model->validator_message_visible) {
        canvas_set_font(canvas, FontSecondary);
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_box(canvas, 8, 10, 110, 48);
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_icon(canvas, 10, 14, &I_WarningDolphin_45x42);
        canvas_draw_rframe(canvas, 8, 8, 112, 50, 3);
        canvas_draw_rframe(canvas, 9, 9, 110, 48, 2);
        elements_multiline_text(canvas, 62, 20, furi_string_get_cstr(model->validator_text));
    }
    canvas_set_font(canvas, FontSecondary);
}

// Moving between rows of different length: the special keys in the right
// column stay in it, letters keep their column as far as the row allows.
static void text_input_move_row(TextInputModel* model, uint8_t new_row) {
    const uint8_t old_size = get_row_size(model, model->selected_row);
    const uint8_t new_size = get_row_size(model, new_row);
    if(model->selected_column == old_size - 1) {
        model->selected_column = new_size - 1;
    } else if(model->selected_column > new_size - 2) {
        model->selected_column = new_size - 2;
    }
    model->selected_row = new_row;
}

static void text_input_handle_up(TextInput* text_input, TextInputModel* model) {
    UNUSED(text_input);
    if(model->selected_row > 0) {
        text_input_move_row(model, model->selected_row - 1);
    }
}

static void text_input_handle_down(TextInput* text_input, TextInputModel* model) {
    UNUSED(text_input);
    if(model->selected_row < keyboard_row_count - 1) {
        text_input_move_row(model, model->selected_row + 1);
    }
}

static void text_input_handle_left(TextInput* text_input, TextInputModel* model) {
    UNUSED(text_input);
    if(model->selected_column > 0) {
        model->selected_column--;
    } else {
        model->selected_column = get_row_size(model, model->selected_row) - 1;
    }
}

static void text_input_handle_right(TextInput* text_input, TextInputModel* model) {
    UNUSED(text_input);
    if(model->selected_column < get_row_size(model, model->selected_row) - 1) {
        model->selected_column++;
    } else {
        model->selected_column = 0;
    }
}

static void text_input_switch_layout(TextInputModel* model) {
    const uint8_t old_size = get_row_size(model, model->selected_row);
    const bool on_special = model->selected_column == old_size - 1;
    model->layout = (model->layout + 1) % TextInputLayoutCount;
    text_input_last_layout = model->layout;
    const uint8_t new_size = get_row_size(model, model->selected_row);
    if(on_special || model->selected_column > new_size - 1) {
        model->selected_column = new_size - 1;
    }
}

static void text_input_handle_ok(TextInput* text_input, TextInputModel* model, bool shift) {
    uint16_t selected = get_selected_char(model);
    size_t text_length = strlen(model->text_buffer);

    bool toggle_case = text_length == 0 || model->clear_default_text;
    if(shift) toggle_case = !toggle_case;
    if(toggle_case) {
        selected = char_to_uppercase(selected);
    }

    if(selected == LAYOUT_KEY) {
        text_input_switch_layout(model);
        return;
    } else if(selected == ENTER_KEY) {
        if(!model->allow_unicode) {
            text_input_transliterate(model->text_buffer, model->text_buffer_size);
            text_length = strlen(model->text_buffer);
        }
        if(model->validator_callback &&
           (!model->validator_callback(
               model->text_buffer, model->validator_text, model->validator_callback_context))) {
            model->validator_message_visible = true;
            furi_timer_start(text_input->timer, furi_kernel_get_tick_frequency() * 4);
        } else if(model->callback != 0 && text_length >= model->minimum_length) {
            model->callback(model->callback_context);
        }
    } else if(selected == BACKSPACE_KEY) {
        text_input_backspace_cb(model);
    } else {
        if(model->clear_default_text) {
            text_length = 0;
        }
        // UTF-8: one byte for ASCII, two for Cyrillic
        const size_t char_size = selected < 0x80 ? 1 : 2;
        if(text_length + char_size < model->text_buffer_size) {
            if(char_size == 1) {
                model->text_buffer[text_length] = (char)selected;
            } else {
                model->text_buffer[text_length] = (char)(0xC0 | (selected >> 6));
                model->text_buffer[text_length + 1] = (char)(0x80 | (selected & 0x3F));
            }
            model->text_buffer[text_length + char_size] = 0;
        }
    }
    model->clear_default_text = false;
}

static bool text_input_view_input_callback(InputEvent* event, void* context) {
    TextInput* text_input = context;
    furi_assert(text_input);

    bool consumed = false;

    // Acquire model
    TextInputModel* model = view_get_model(text_input->view);

    if((!(event->type == InputTypePress) && !(event->type == InputTypeRelease)) &&
       model->validator_message_visible) {
        model->validator_message_visible = false;
        consumed = true;
    } else if(event->type == InputTypeShort) {
        consumed = true;
        switch(event->key) {
        case InputKeyUp:
            text_input_handle_up(text_input, model);
            break;
        case InputKeyDown:
            text_input_handle_down(text_input, model);
            break;
        case InputKeyLeft:
            text_input_handle_left(text_input, model);
            break;
        case InputKeyRight:
            text_input_handle_right(text_input, model);
            break;
        case InputKeyOk:
            text_input_handle_ok(text_input, model, false);
            break;
        default:
            consumed = false;
            break;
        }
    } else if(event->type == InputTypeLong) {
        consumed = true;
        switch(event->key) {
        case InputKeyUp:
            text_input_handle_up(text_input, model);
            break;
        case InputKeyDown:
            text_input_handle_down(text_input, model);
            break;
        case InputKeyLeft:
            text_input_handle_left(text_input, model);
            break;
        case InputKeyRight:
            text_input_handle_right(text_input, model);
            break;
        case InputKeyOk:
            text_input_handle_ok(text_input, model, true);
            break;
        case InputKeyBack:
            text_input_backspace_cb(model);
            break;
        default:
            consumed = false;
            break;
        }
    } else if(event->type == InputTypeRepeat) {
        consumed = true;
        switch(event->key) {
        case InputKeyUp:
            text_input_handle_up(text_input, model);
            break;
        case InputKeyDown:
            text_input_handle_down(text_input, model);
            break;
        case InputKeyLeft:
            text_input_handle_left(text_input, model);
            break;
        case InputKeyRight:
            text_input_handle_right(text_input, model);
            break;
        case InputKeyBack:
            text_input_backspace_cb(model);
            break;
        default:
            consumed = false;
            break;
        }
    }

    // Commit model
    view_commit_model(text_input->view, consumed);

    return consumed;
}

void text_input_timer_callback(void* context) {
    furi_assert(context);
    TextInput* text_input = context;

    with_view_model(
        text_input->view,
        TextInputModel * model,
        { model->validator_message_visible = false; },
        true);
}

TextInput* text_input_alloc(void) {
    TextInput* text_input = malloc(sizeof(TextInput));
    text_input->view = view_alloc();
    view_set_context(text_input->view, text_input);
    view_allocate_model(text_input->view, ViewModelTypeLocking, sizeof(TextInputModel));
    view_set_draw_callback(text_input->view, text_input_view_draw_callback);
    view_set_input_callback(text_input->view, text_input_view_input_callback);

    text_input->timer = furi_timer_alloc(text_input_timer_callback, FuriTimerTypeOnce, text_input);

    with_view_model(
        text_input->view,
        TextInputModel * model,
        { model->validator_text = furi_string_alloc(); },
        false);

    text_input_reset(text_input);

    return text_input;
}

void text_input_free(TextInput* text_input) {
    furi_check(text_input);
    with_view_model(
        text_input->view,
        TextInputModel * model,
        { furi_string_free(model->validator_text); },
        false);

    // Send stop command
    furi_timer_stop(text_input->timer);
    // Release allocated memory
    furi_timer_free(text_input->timer);

    view_free(text_input->view);

    free(text_input);
}

void text_input_reset(TextInput* text_input) {
    furi_check(text_input);
    with_view_model(
        text_input->view,
        TextInputModel * model,
        {
            model->header = "";
            model->selected_row = 0;
            model->selected_column = 0;
            model->minimum_length = 1;
            model->clear_default_text = false;
            model->allow_unicode = true;
            model->layout = text_input_last_layout;
            model->text_buffer = NULL;
            model->text_buffer_size = 0;
            model->callback = NULL;
            model->callback_context = NULL;
            model->validator_callback = NULL;
            model->validator_callback_context = NULL;
            furi_string_reset(model->validator_text);
            model->validator_message_visible = false;
        },
        true);
}

View* text_input_get_view(TextInput* text_input) {
    furi_check(text_input);
    return text_input->view;
}

void text_input_set_result_callback(
    TextInput* text_input,
    TextInputCallback callback,
    void* callback_context,
    char* text_buffer,
    size_t text_buffer_size,
    bool clear_default_text) {
    furi_check(text_input);
    with_view_model(
        text_input->view,
        TextInputModel * model,
        {
            model->callback = callback;
            model->callback_context = callback_context;
            model->text_buffer = text_buffer;
            model->text_buffer_size = text_buffer_size;
            model->clear_default_text = clear_default_text;
            if(text_buffer && text_buffer[0] != '\0') {
                // Set focus on Save
                model->selected_row = 2;
                model->selected_column = get_row_size(model, 2) - 1;
            }
        },
        true);
}

void text_input_set_minimum_length(TextInput* text_input, size_t minimum_length) {
    with_view_model(
        text_input->view,
        TextInputModel * model,
        { model->minimum_length = minimum_length; },
        true);
}

void text_input_set_validator(
    TextInput* text_input,
    TextInputValidatorCallback callback,
    void* callback_context) {
    furi_check(text_input);
    with_view_model(
        text_input->view,
        TextInputModel * model,
        {
            model->validator_callback = callback;
            model->validator_callback_context = callback_context;
        },
        true);
}

TextInputValidatorCallback text_input_get_validator_callback(TextInput* text_input) {
    furi_check(text_input);
    TextInputValidatorCallback validator_callback = NULL;
    with_view_model(
        text_input->view,
        TextInputModel * model,
        { validator_callback = model->validator_callback; },
        false);
    return validator_callback;
}

void* text_input_get_validator_callback_context(TextInput* text_input) {
    furi_check(text_input);
    void* validator_callback_context = NULL;
    with_view_model(
        text_input->view,
        TextInputModel * model,
        { validator_callback_context = model->validator_callback_context; },
        false);
    return validator_callback_context;
}

void text_input_set_header_text(TextInput* text_input, const char* text) {
    furi_check(text_input);
    with_view_model(text_input->view, TextInputModel * model, { model->header = text; }, true);
}

void text_input_set_allow_unicode(TextInput* text_input, bool allow) {
    furi_check(text_input);
    with_view_model(
        text_input->view, TextInputModel * model, { model->allow_unicode = allow; }, true);
}
