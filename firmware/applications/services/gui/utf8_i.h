/**
 * @file utf8_i.h
 * GUI: UTF-8 helpers for text layout (R0N1N's Russian UI).
 *
 * Stock layout code walks text byte by byte, which only works for ASCII:
 * a Cyrillic letter is two bytes. These let it step and measure whole
 * characters instead.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

/** Decode the character at `s` into `code`; returns its length in bytes
 * (1 for ASCII, for the terminating zero and for malformed bytes). */
static inline size_t gui_utf8_char(const char* s, uint16_t* code) {
    const uint8_t* b = (const uint8_t*)s;
    if((b[0] & 0xE0) == 0xC0 && (b[1] & 0xC0) == 0x80) {
        *code = ((b[0] & 0x1F) << 6) | (b[1] & 0x3F);
        return 2;
    }
    if((b[0] & 0xF0) == 0xE0 && (b[1] & 0xC0) == 0x80 && (b[2] & 0xC0) == 0x80) {
        *code = ((b[0] & 0x0F) << 12) | ((b[1] & 0x3F) << 6) | (b[2] & 0x3F);
        return 3;
    }
    *code = b[0];
    return 1;
}

/** Move byte position `pos` back to the start of the character it is in. */
static inline size_t gui_utf8_char_start(const char* s, size_t pos) {
    while(pos && (((uint8_t)s[pos]) & 0xC0) == 0x80) {
        pos--;
    }
    return pos;
}
