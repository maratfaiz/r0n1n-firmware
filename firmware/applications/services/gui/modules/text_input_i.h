/**
 * @file text_input_i.h
 * GUI: TextInput, firmware-internal additions (R0N1N)
 */
#pragma once

#include "text_input.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Whether Cyrillic stays in the result (the default) or is transliterated
 * to Latin on Save, for text that must be ASCII. Reset by text_input_reset().
 */
void text_input_set_allow_unicode(TextInput* text_input, bool allow);

#ifdef __cplusplus
}
#endif
