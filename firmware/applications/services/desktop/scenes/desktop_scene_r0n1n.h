/**
 * @file desktop_scene_r0n1n.h
 * Shared plumbing for the R0N1N desktop scenes.
 *
 * The shared views report actions as custom events with the kind in the high
 * bits and the item index / tab / value in the low 16 bits. Keeping them out
 * of the low range matters: desktop_custom_event_callback() matches the global
 * DesktopEvent values (app started, auto-lock...) first, so a raw list index
 * could otherwise be mistaken for one of them.
 */
#pragma once

#include "../desktop_i.h"

#define R0N1N_EVT_OK     0x10000UL
#define R0N1N_EVT_HOLD   0x20000UL
#define R0N1N_EVT_TAB    0x30000UL
#define R0N1N_EVT_SLIDER 0x40000UL
#define R0N1N_EVT_KIND   0xFFFF0000UL
#define R0N1N_EVT_VALUE  0x0000FFFFUL

/** Reset the shared list / grid / carousel and route their callbacks to custom events. */
void desktop_r0n1n_prepare_list(Desktop* desktop);
void desktop_r0n1n_prepare_grid(Desktop* desktop);
void desktop_r0n1n_prepare_carousel(Desktop* desktop);

/** Show `text` in the info popup scene (text must be static). */
void desktop_r0n1n_show_info(Desktop* desktop, const char* text);
