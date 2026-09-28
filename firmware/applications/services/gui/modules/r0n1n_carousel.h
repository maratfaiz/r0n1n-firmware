/**
 * @file r0n1n_carousel.h
 * R0N1N sections carousel: labelled tiles paged with Left/Right, four visible,
 * page dots underneath.
 */
#pragma once

#include <gui/view.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct R0n1nCarousel R0n1nCarousel;

typedef void (*R0n1nCarouselCallback)(void* context, uint32_t index);

R0n1nCarousel* r0n1n_carousel_alloc(void);
void r0n1n_carousel_free(R0n1nCarousel* carousel);
View* r0n1n_carousel_get_view(R0n1nCarousel* carousel);

void r0n1n_carousel_reset(R0n1nCarousel* carousel);
void r0n1n_carousel_set_title(R0n1nCarousel* carousel, const char* title);

/** `label` must outlive the carousel (static strings). */
void r0n1n_carousel_add_item(
    R0n1nCarousel* carousel,
    const Icon* icon,
    const char* label,
    uint32_t index);

void r0n1n_carousel_set_callback(
    R0n1nCarousel* carousel,
    R0n1nCarouselCallback callback,
    void* context);

/** Select by position (not index); out-of-range selects the last item. */
void r0n1n_carousel_set_position(R0n1nCarousel* carousel, size_t position);
size_t r0n1n_carousel_get_position(R0n1nCarousel* carousel);

#ifdef __cplusplus
}
#endif
