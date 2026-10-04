#include "r0n1n_carousel.h"

#include <gui/r0n1n_ui.h>
#include <assets_icons.h>
#include <furi.h>

#define R0N1N_CAROUSEL_MAX     8
#define R0N1N_CAROUSEL_VISIBLE 4

typedef struct {
    const Icon* icon;
    const char* label;
    uint32_t index;
} R0n1nCarouselItem;

typedef struct {
    R0n1nCarouselItem items[R0N1N_CAROUSEL_MAX];
    size_t count;
    size_t position;
    size_t window;
    FuriString* title;
} R0n1nCarouselModel;

struct R0n1nCarousel {
    View* view;
    R0n1nCarouselCallback callback;
    void* context;
};

static void r0n1n_carousel_draw_callback(Canvas* canvas, void* _model) {
    R0n1nCarouselModel* model = _model;
    canvas_clear(canvas);

    char counter[24] = {0};
    if(model->count)
        snprintf(
            counter,
            sizeof(counter),
            "%u/%u",
            (unsigned)(model->position + 1),
            (unsigned)model->count);
    r0n1n_ui_header(canvas, NULL, furi_string_get_cstr(model->title), counter, -1);

    if(model->count > R0N1N_CAROUSEL_VISIBLE) {
        canvas_draw_icon(canvas, 1, 27, &I_R_ArrowLeft_3x5);
        canvas_draw_icon(canvas, 124, 27, &I_R_ArrowRight_3x5);
    }

    canvas_set_font(canvas, FontSecondary);
    for(size_t i = 0; i < R0N1N_CAROUSEL_VISIBLE && model->window + i < model->count; i++) {
        const size_t pos = model->window + i;
        const R0n1nCarouselItem* item = &model->items[pos];
        const int32_t x = 6 + i * 30;
        if(pos == model->position) {
            canvas_draw_rbox(canvas, x, 14, 27, 33, 2);
            canvas_set_color(canvas, ColorWhite);
        } else {
            canvas_draw_rframe(canvas, x, 14, 27, 33, 2);
        }
        r0n1n_ui_icon_centered(canvas, x, 16, 27, 18, item->icon);
        canvas_draw_str_aligned(canvas, x + 13, 43, AlignCenter, AlignBottom, item->label);
        canvas_set_color(canvas, ColorBlack);
    }

    const int32_t dots_x = 64 - (int32_t)(model->count * 6 - 3) / 2;
    for(size_t i = 0; i < model->count; i++) {
        if(i == model->position) {
            canvas_draw_box(canvas, dots_x + i * 6, 54, 3, 3);
        } else {
            canvas_draw_frame(canvas, dots_x + i * 6, 54, 3, 3);
        }
    }
}

static void r0n1n_carousel_fix_window(R0n1nCarouselModel* model) {
    if(model->position < model->window) {
        model->window = model->position;
    } else if(model->position >= model->window + R0N1N_CAROUSEL_VISIBLE) {
        model->window = model->position - R0N1N_CAROUSEL_VISIBLE + 1;
    }
}

static bool r0n1n_carousel_input_callback(InputEvent* event, void* context) {
    R0n1nCarousel* carousel = context;
    bool consumed = false;
    bool fire = false;
    uint32_t index = 0;

    if(event->type == InputTypeShort || event->type == InputTypeRepeat) {
        with_view_model(
            carousel->view,
            R0n1nCarouselModel * model,
            {
                if(model->count && (event->key == InputKeyLeft || event->key == InputKeyRight)) {
                    const int32_t delta = event->key == InputKeyLeft ? -1 : 1;
                    model->position = (model->position + model->count + delta) % model->count;
                    r0n1n_carousel_fix_window(model);
                    consumed = true;
                } else if(model->count && event->key == InputKeyOk && event->type == InputTypeShort) {
                    index = model->items[model->position].index;
                    fire = true;
                    consumed = true;
                }
            },
            true);
    }

    if(fire && carousel->callback) carousel->callback(carousel->context, index);
    return consumed;
}

R0n1nCarousel* r0n1n_carousel_alloc(void) {
    R0n1nCarousel* carousel = malloc(sizeof(R0n1nCarousel));
    carousel->view = view_alloc();
    view_set_context(carousel->view, carousel);
    view_allocate_model(carousel->view, ViewModelTypeLocking, sizeof(R0n1nCarouselModel));
    view_set_draw_callback(carousel->view, r0n1n_carousel_draw_callback);
    view_set_input_callback(carousel->view, r0n1n_carousel_input_callback);
    with_view_model(
        carousel->view,
        R0n1nCarouselModel * model,
        {
            model->count = 0;
            model->position = 0;
            model->window = 0;
            model->title = furi_string_alloc();
        },
        true);
    return carousel;
}

void r0n1n_carousel_free(R0n1nCarousel* carousel) {
    furi_check(carousel);
    with_view_model(
        carousel->view, R0n1nCarouselModel * model, { furi_string_free(model->title); }, false);
    view_free(carousel->view);
    free(carousel);
}

View* r0n1n_carousel_get_view(R0n1nCarousel* carousel) {
    furi_check(carousel);
    return carousel->view;
}

void r0n1n_carousel_reset(R0n1nCarousel* carousel) {
    furi_check(carousel);
    with_view_model(
        carousel->view,
        R0n1nCarouselModel * model,
        {
            model->count = 0;
            model->position = 0;
            model->window = 0;
            furi_string_reset(model->title);
        },
        true);
}

void r0n1n_carousel_set_title(R0n1nCarousel* carousel, const char* title) {
    furi_check(carousel);
    with_view_model(
        carousel->view,
        R0n1nCarouselModel * model,
        { furi_string_set(model->title, title); },
        true);
}

void r0n1n_carousel_add_item(
    R0n1nCarousel* carousel,
    const Icon* icon,
    const char* label,
    uint32_t index) {
    furi_check(carousel);
    with_view_model(
        carousel->view,
        R0n1nCarouselModel * model,
        {
            furi_check(model->count < R0N1N_CAROUSEL_MAX);
            R0n1nCarouselItem* item = &model->items[model->count++];
            item->icon = icon;
            item->label = label;
            item->index = index;
        },
        true);
}

void r0n1n_carousel_set_callback(
    R0n1nCarousel* carousel,
    R0n1nCarouselCallback callback,
    void* context) {
    furi_check(carousel);
    carousel->callback = callback;
    carousel->context = context;
}

void r0n1n_carousel_set_position(R0n1nCarousel* carousel, size_t position) {
    furi_check(carousel);
    with_view_model(
        carousel->view,
        R0n1nCarouselModel * model,
        {
            if(model->count) {
                model->position = MIN(position, model->count - 1);
                r0n1n_carousel_fix_window(model);
            }
        },
        true);
}

size_t r0n1n_carousel_get_position(R0n1nCarousel* carousel) {
    furi_check(carousel);
    size_t position = 0;
    with_view_model(
        carousel->view, R0n1nCarouselModel * model, { position = model->position; }, false);
    return position;
}
