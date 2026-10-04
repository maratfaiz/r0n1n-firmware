#include "r0n1n_ui.h"

#include <furi.h>

void r0n1n_ui_fit_width(Canvas* canvas, FuriString* text, size_t width) {
    // Like elements_string_fit_width(), but never cuts a UTF-8 sequence in
    // half: Cyrillic letters are two bytes, and a stray lead byte renders as
    // garbage instead of the "...".
    if(canvas_string_width(canvas, furi_string_get_cstr(text)) <= width) return;
    const size_t ellipsis = canvas_string_width(canvas, "...");
    const size_t target = width > ellipsis ? width - ellipsis : 0;
    size_t len = furi_string_size(text);
    do {
        if(!len) break;
        len--;
        while(len && (furi_string_get_char(text, len) & 0xC0) == 0x80) {
            len--;
        }
        furi_string_left(text, len);
    } while(canvas_string_width(canvas, furi_string_get_cstr(text)) > target);
    furi_string_cat_str(text, "...");
}

void r0n1n_ui_multiline_centered(Canvas* canvas, int32_t y_center, const char* text) {
    canvas_set_font(canvas, FontSecondary);
    size_t lines = 1;
    for(const char* p = text; *p; p++) {
        if(*p == '\n') lines++;
    }
    const int32_t line_height = 10;
    int32_t y = y_center - (int32_t)(lines * line_height) / 2 + 7;
    FuriString* line = furi_string_alloc();
    const char* start = text;
    while(true) {
        const char* end = strchr(start, '\n');
        furi_string_set_strn(line, start, end ? (size_t)(end - start) : strlen(start));
        canvas_draw_str_aligned(
            canvas, 64, y, AlignCenter, AlignBottom, furi_string_get_cstr(line));
        if(!end) break;
        start = end + 1;
        y += line_height;
    }
    furi_string_free(line);
}

void r0n1n_ui_icon_centered(
    Canvas* canvas,
    int32_t x,
    int32_t y,
    uint8_t width,
    uint8_t height,
    const Icon* icon) {
    if(!icon) return;
    canvas_draw_icon(
        canvas,
        x + (width - icon_get_width(icon)) / 2,
        y + (height - icon_get_height(icon)) / 2,
        icon);
}

void r0n1n_ui_battery(Canvas* canvas, int32_t x_right, int32_t y, uint8_t pct) {
    int32_t bx = x_right - 13;
    canvas_draw_frame(canvas, bx, y, 12, 7);
    canvas_draw_box(canvas, bx + 12, y + 2, 1, 3);
    canvas_draw_box(canvas, bx + 2, y + 2, (8 * MIN(pct, 100) + 50) / 100, 3);

    char text[5];
    snprintf(text, sizeof(text), "%u%%", MIN(pct, 100));
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(canvas, bx - 2, y + 7, AlignRight, AlignBottom, text);
}

void r0n1n_ui_header(
    Canvas* canvas,
    const Icon* icon,
    const char* title,
    const char* right,
    int8_t battery_pct) {
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_box(canvas, 0, 0, canvas_width(canvas), R0N1N_UI_HEADER_HEIGHT);
    canvas_set_color(canvas, ColorWhite);
    canvas_set_font(canvas, FontSecondary);

    int32_t x = 2;
    if(icon) {
        canvas_draw_icon(canvas, 2, (R0N1N_UI_HEADER_HEIGHT - icon_get_height(icon)) / 2, icon);
        x += icon_get_width(icon) + 2;
    }

    int32_t right_x = canvas_width(canvas) - 2;
    if(battery_pct >= 0) {
        r0n1n_ui_battery(canvas, right_x, 1, battery_pct);
        right_x -= 34;
    }
    if(right) {
        canvas_draw_str_aligned(canvas, right_x, 9, AlignRight, AlignBottom, right);
        right_x -= canvas_string_width(canvas, right) + 4;
    }

    if(title) {
        FuriString* text = furi_string_alloc_set(title);
        r0n1n_ui_fit_width(canvas, text, right_x - x);
        canvas_draw_str(canvas, x, 8, furi_string_get_cstr(text));
        furi_string_free(text);
    }
    canvas_set_color(canvas, ColorBlack);
}

void r0n1n_ui_list_row(
    Canvas* canvas,
    int32_t y,
    uint8_t height,
    uint8_t width,
    const Icon* icon,
    const char* label,
    const char* right,
    const Icon* right_icon,
    bool selected) {
    canvas_set_font(canvas, FontSecondary);
    if(selected) {
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_rbox(canvas, 0, y, width, height, 2);
        canvas_set_color(canvas, ColorWhite);
    } else {
        canvas_set_color(canvas, ColorBlack);
    }

    const int32_t baseline = y + (height + 7) / 2;
    if(icon) r0n1n_ui_icon_centered(canvas, 2, y, 10, height, icon);

    int32_t right_edge = width - 3;
    if(right_icon) {
        right_edge -= icon_get_width(right_icon);
        canvas_draw_icon(
            canvas, right_edge, y + (height - icon_get_height(right_icon)) / 2, right_icon);
        right_edge -= 3;
    } else if(right) {
        canvas_draw_str_aligned(canvas, right_edge, baseline, AlignRight, AlignBottom, right);
        right_edge -= canvas_string_width(canvas, right) + 3;
    }

    if(label) {
        FuriString* text = furi_string_alloc_set(label);
        r0n1n_ui_fit_width(canvas, text, right_edge - 15);
        canvas_draw_str(canvas, 15, baseline, furi_string_get_cstr(text));
        furi_string_free(text);
    }
    canvas_set_color(canvas, ColorBlack);
}

void r0n1n_ui_scrollbar(Canvas* canvas, int32_t y, uint8_t height, size_t pos, size_t total) {
    if(total < 2) return;
    const int32_t x = canvas_width(canvas) - 2;
    for(int32_t i = 0; i < height; i += 2) {
        canvas_draw_dot(canvas, x, y + i);
    }
    uint8_t thumb = MAX((size_t)4, height / total);
    int32_t thumb_y = y + (int32_t)(height - thumb) * (int32_t)pos / (int32_t)(total - 1);
    canvas_draw_box(canvas, x - 1, thumb_y, 3, thumb);
}

void r0n1n_ui_tile(
    Canvas* canvas,
    int32_t x,
    int32_t y,
    uint8_t width,
    uint8_t height,
    const Icon* icon,
    bool selected,
    bool marked) {
    canvas_set_color(canvas, ColorBlack);
    if(selected) {
        canvas_draw_rbox(canvas, x, y, width, height, 2);
        canvas_set_color(canvas, ColorWhite);
    } else {
        canvas_draw_rframe(canvas, x, y, width, height, 2);
    }
    r0n1n_ui_icon_centered(canvas, x, y, width, height, icon);
    if(marked) canvas_draw_box(canvas, x + width - 5, y + 2, 3, 3);
    canvas_set_color(canvas, ColorBlack);
}

void r0n1n_ui_caption(Canvas* canvas, int32_t sep_y, const char* text) {
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_line(canvas, 0, sep_y, canvas_width(canvas) - 1, sep_y);
    if(!text) return;
    canvas_set_font(canvas, FontSecondary);
    FuriString* str = furi_string_alloc_set(text);
    r0n1n_ui_fit_width(canvas, str, canvas_width(canvas) - 4);
    canvas_draw_str_aligned(
        canvas, canvas_width(canvas) / 2, 63, AlignCenter, AlignBottom, furi_string_get_cstr(str));
    furi_string_free(str);
}

// 77x20 XBM, rows of 10 bytes, generated from the wordmark drawn for R0N1N.
static const uint8_t r0n1n_ui_logo_xbm[] = {
    0xf8, 0x3f, 0xc0, 0xff, 0xe0, 0x00, 0x07, 0x8e, 0x03, 0x1c, 0xf8, 0x7f, 0xe0, 0xff, 0xe1, 0x01,
    0x07, 0x8f, 0x07, 0x1c, 0xf8, 0xff, 0xf0, 0xff, 0xe3, 0x01, 0x87, 0x8f, 0x07, 0x1c, 0x38, 0xe0,
    0x71, 0xe0, 0xe3, 0x03, 0xc7, 0x8f, 0x0f, 0x1c, 0x38, 0xc0, 0x71, 0xf0, 0xe3, 0x03, 0xc7, 0x8e,
    0x0f, 0x1c, 0x1c, 0xe0, 0x38, 0xf8, 0xf1, 0x83, 0x03, 0xc7, 0x0f, 0x0e, 0x1c, 0xe0, 0x38, 0xdc,
    0xf1, 0x83, 0x03, 0xc7, 0x0f, 0x0e, 0x1c, 0xf0, 0x38, 0xdc, 0x71, 0x87, 0x03, 0xc7, 0x1d, 0x0e,
    0xfc, 0x7f, 0x38, 0xce, 0x71, 0x87, 0x03, 0xc7, 0x1d, 0x0e, 0xfc, 0x3f, 0x38, 0xce, 0x71, 0x8e,
    0x03, 0xc7, 0x39, 0x0e, 0xfe, 0x0f, 0x9c, 0xe3, 0x38, 0xc7, 0x81, 0xe3, 0x1c, 0x07, 0x0e, 0x0f,
    0x9c, 0xe3, 0x38, 0xce, 0x81, 0xe3, 0x38, 0x07, 0x0e, 0x1e, 0xdc, 0xe1, 0x38, 0xce, 0x81, 0xe3,
    0x38, 0x07, 0x0e, 0x3c, 0xdc, 0xe1, 0x38, 0xdc, 0x81, 0xe3, 0x70, 0x07, 0x0e, 0x78, 0xfc, 0xe0,
    0x38, 0xdc, 0x81, 0xe3, 0x70, 0x07, 0x07, 0x38, 0x7e, 0x70, 0x1c, 0xfc, 0xc0, 0x71, 0xf0, 0x03,
    0x07, 0x38, 0x3e, 0x70, 0x1c, 0xfc, 0xc0, 0x71, 0xf0, 0x03, 0x07, 0x38, 0xfe, 0x7f, 0x1c, 0xf8,
    0xc0, 0x71, 0xe0, 0x03, 0x07, 0x38, 0xfc, 0x3f, 0x1c, 0xf8, 0xc0, 0x71, 0xe0, 0x03, 0x07, 0x38,
    0xf8, 0x1f, 0x1c, 0xf0, 0xc0, 0x71, 0xc0, 0x03,
};

void r0n1n_ui_logo(Canvas* canvas, int32_t x, int32_t y, const int8_t* row_shift) {
    const size_t stride = (R0N1N_UI_LOGO_WIDTH + 7) / 8;
    for(size_t row = 0; row < R0N1N_UI_LOGO_HEIGHT; row++) {
        int32_t shift = row_shift ? row_shift[row] : 0;
        canvas_draw_xbm(
            canvas,
            x + shift,
            y + (int32_t)row,
            R0N1N_UI_LOGO_WIDTH,
            1,
            &r0n1n_ui_logo_xbm[row * stride]);
    }
}

// Length in bytes of the UTF-8 character starting with `c`.
static size_t r0n1n_ui_utf8_len(char c) {
    uint8_t b = (uint8_t)c;
    if(b >= 0xF0) return 4;
    if(b >= 0xE0) return 3;
    if(b >= 0xC0) return 2;
    return 1;
}

void r0n1n_ui_spaced_text(
    Canvas* canvas,
    int32_t x,
    int32_t y,
    const char* text,
    uint8_t spacing,
    size_t chars) {
    char glyph[5];
    int32_t total = 0;
    for(const char* p = text; *p;) {
        size_t len = r0n1n_ui_utf8_len(*p);
        strlcpy(glyph, p, MIN(len + 1, sizeof(glyph)));
        total += canvas_string_width(canvas, glyph) + spacing;
        p += strlen(glyph);
    }
    if(total) total -= spacing;

    int32_t cx = x - total / 2;
    for(const char* p = text; *p && chars; chars--) {
        size_t len = r0n1n_ui_utf8_len(*p);
        strlcpy(glyph, p, MIN(len + 1, sizeof(glyph)));
        canvas_draw_str(canvas, cx, y, glyph);
        cx += canvas_string_width(canvas, glyph) + spacing;
        p += strlen(glyph);
    }
}

void r0n1n_ui_progress(
    Canvas* canvas,
    int32_t x,
    int32_t y,
    uint8_t width,
    uint8_t height,
    uint8_t percent) {
    canvas_draw_rframe(canvas, x, y, width, height, 2);
    int32_t fill = (int32_t)(width - 4) * MIN(percent, 100) / 100;
    if(fill <= 0) return;
    int32_t inner_h = height - 4;
    canvas_draw_box(canvas, x + 2, y + 2, fill, inner_h);
    // Diagonal notches cut into the fill every 4 px
    canvas_set_color(canvas, ColorWhite);
    for(int32_t i = 3; i < fill + inner_h; i += 4) {
        for(int32_t j = 0; j < inner_h; j++) {
            int32_t px = i - j;
            if(px >= 1 && px < fill - 1) canvas_draw_dot(canvas, x + 2 + px, y + 2 + j);
        }
    }
    canvas_set_color(canvas, ColorBlack);
}
