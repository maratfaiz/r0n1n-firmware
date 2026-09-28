#include "r0n1n_boot.h"

#include <furi.h>
#include <gui/r0n1n_ui.h>
#include <gui/canvas.h>
#include <assets_icons.h>

#define R0N1N_BOOT_FRAME_MS 33

#define LOGO_X  ((128 - R0N1N_UI_LOGO_WIDTH) / 2)
#define LOGO_Y  6
#define BLADE_Y 30
#define MOTTO_Y 44
#define BAR_Y   51

// Timeline, ms
#define T_POWER  300 // a scanline opens from the center
#define T_GLITCH 1000 // the wordmark settles out of shifted slices
#define T_FLASH  1066 // two inverted frames as the blade strikes
#define T_BLADE  1200 // the blade reaches the right edge
#define T_MOTTO  1850 // the motto has typed out
#define T_BAR    2700 // the bar is full

static const char* const r0n1n_boot_motto = "ПУТЬ БЕЗ ХОЗЯИНА";

static uint32_t r0n1n_boot_hash(uint32_t a, uint32_t b) {
    uint32_t h = a * 0x9E3779B1u ^ (b + 0x7F4A7C15u) * 0x85EBCA77u;
    h ^= h >> 15;
    h *= 0x2C1B3C6Du;
    h ^= h >> 12;
    return h;
}

static void r0n1n_boot_draw_logo(Canvas* canvas, uint32_t t) {
    if(t >= T_BLADE) {
        r0n1n_ui_logo(canvas, LOGO_X, LOGO_Y, NULL);
        return;
    }
    if(t >= T_GLITCH) {
        // Cut in two by the blade: the halves slide apart, then close
        int8_t shift[R0N1N_UI_LOGO_HEIGHT];
        int8_t gap = (t < T_FLASH + (T_BLADE - T_FLASH) / 2) ? 3 : 1;
        for(size_t row = 0; row < R0N1N_UI_LOGO_HEIGHT; row++) {
            shift[row] = row < R0N1N_UI_LOGO_HEIGHT / 2 ? gap : -gap;
        }
        r0n1n_ui_logo(canvas, LOGO_X, LOGO_Y, shift);
        return;
    }
    // Slices of 3 rows jump sideways; the jumps shrink until the logo is still
    const uint32_t frame = t / (R0N1N_BOOT_FRAME_MS * 2);
    const int32_t amp = 1 + 22 * (int32_t)(T_GLITCH - t) / (T_GLITCH - T_POWER);
    int8_t shift[R0N1N_UI_LOGO_HEIGHT];
    for(size_t row = 0; row < R0N1N_UI_LOGO_HEIGHT; row++) {
        uint32_t h = r0n1n_boot_hash(row / 3, frame);
        shift[row] = (h % 3) ? (int8_t)((int32_t)(h >> 8) % (2 * amp + 1) - amp) : 0;
    }
    // Dropped scanlines early on: a 127 px shift pushes the row off the screen
    for(size_t row = 0; row < R0N1N_UI_LOGO_HEIGHT; row++) {
        if(t < 800 && r0n1n_boot_hash(row, frame + 99) % 4 == 0) shift[row] = 127;
    }
    r0n1n_ui_logo(canvas, LOGO_X, LOGO_Y, shift);

    // Stray noise dashes around the logo
    for(uint32_t i = 0; i < 4; i++) {
        uint32_t h = r0n1n_boot_hash(i + 50, frame);
        int32_t y = (int32_t)(h % 40);
        int32_t x = (int32_t)((h >> 8) % 110);
        canvas_draw_line(canvas, x, y, x + 4 + (int32_t)((h >> 16) % 14), y);
    }
}

static void r0n1n_boot_draw_blade(Canvas* canvas, uint32_t t) {
    const int32_t left = 8, right = 119;
    int32_t tip = right;
    if(t < T_BLADE) tip = left + (right - left) * (int32_t)(t - T_GLITCH) / (T_BLADE - T_GLITCH);
    canvas_draw_line(canvas, left, BLADE_Y, tip, BLADE_Y);
    if(t < T_BLADE) {
        // Glint at the tip
        int32_t glint = MAX(left, tip - 10);
        canvas_draw_line(canvas, glint, BLADE_Y - 1, tip, BLADE_Y - 1);
        canvas_draw_line(canvas, glint, BLADE_Y + 1, tip, BLADE_Y + 1);
        canvas_draw_line(canvas, tip + 2, BLADE_Y, tip + 5, BLADE_Y);
    } else {
        // Tsuba: a small guard a third of the way in
        canvas_draw_box(canvas, left + 30, BLADE_Y - 2, 2, 5);
        canvas_draw_line(canvas, left, BLADE_Y + 1, left + 29, BLADE_Y + 1);
    }
}

void r0n1n_boot_draw(Canvas* canvas, uint32_t t) {
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);

    if(t < T_POWER) {
        // A CRT-like line opening from the center
        int32_t half = 2 + 62 * (int32_t)t / T_POWER;
        int32_t y = LOGO_Y + R0N1N_UI_LOGO_HEIGHT / 2;
        canvas_draw_line(canvas, 64 - half, y, 63 + half, y);
        if(t > T_POWER / 2) {
            canvas_draw_line(canvas, 64 - half / 2, y - 1, 63 + half / 2, y - 1);
            canvas_draw_line(canvas, 64 - half / 2, y + 1, 63 + half / 2, y + 1);
        }
        return;
    }

    r0n1n_boot_draw_logo(canvas, t);
    if(t < T_GLITCH) return;

    r0n1n_boot_draw_blade(canvas, t);

    if(t >= T_BLADE) {
        // The mascot settles in beside the wordmark once the blade lands.
        canvas_draw_icon(canvas, 1, LOGO_Y + 1, &I_R_MascotHead_24x22);

        canvas_set_font(canvas, FontSecondary);
        size_t chars = SIZE_MAX;
        if(t < T_MOTTO) chars = 1 + 16 * (t - T_BLADE) / (T_MOTTO - T_BLADE);
        r0n1n_ui_spaced_text(canvas, 64, MOTTO_Y, r0n1n_boot_motto, 1, chars);

        // Ease-out fill
        uint32_t p = 100;
        if(t < T_BAR) {
            uint32_t k = 100 - 100 * (t - T_BLADE) / (T_BAR - T_BLADE);
            p = 100 - k * k / 100;
        }
        r0n1n_ui_progress(canvas, 24, BAR_Y, 80, 7, (uint8_t)p);
    }

    if(t < T_FLASH) {
        canvas_set_color(canvas, ColorXOR);
        canvas_draw_box(canvas, 0, 0, 128, 64);
        canvas_set_color(canvas, ColorBlack);
    }
}

typedef struct {
    ViewPort* view_port;
    uint32_t start;
    volatile bool skip;
} R0n1nBoot;

static uint32_t r0n1n_boot_elapsed_ms(R0n1nBoot* boot) {
    return (furi_get_tick() - boot->start) * 1000 / furi_kernel_get_tick_frequency();
}

static void r0n1n_boot_draw_callback(Canvas* canvas, void* context) {
    R0n1nBoot* boot = context;
    r0n1n_boot_draw(canvas, r0n1n_boot_elapsed_ms(boot));
}

static void r0n1n_boot_input_callback(InputEvent* event, void* context) {
    UNUSED(event);
    R0n1nBoot* boot = context;
    boot->skip = true;
}

void r0n1n_boot_run(Gui* gui) {
    R0n1nBoot boot = {.view_port = view_port_alloc(), .start = furi_get_tick(), .skip = false};
    view_port_draw_callback_set(boot.view_port, r0n1n_boot_draw_callback, &boot);
    view_port_input_callback_set(boot.view_port, r0n1n_boot_input_callback, &boot);
    gui_add_view_port(gui, boot.view_port, GuiLayerFullscreen);

    while(!boot.skip && r0n1n_boot_elapsed_ms(&boot) < R0N1N_BOOT_DURATION_MS) {
        view_port_update(boot.view_port);
        furi_delay_ms(R0N1N_BOOT_FRAME_MS);
    }

    gui_remove_view_port(gui, boot.view_port);
    view_port_free(boot.view_port);
}
