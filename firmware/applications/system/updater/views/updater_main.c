#include <gui/gui_i.h>
#include <gui/view.h>
#include <gui/elements.h>
#include <gui/r0n1n_ui.h>
#include <gui/canvas.h>
#include <assets_icons.h>
#include <furi.h>
#include <input/input.h>

#include "../updater_i.h"
#include "updater_main.h"

struct UpdaterMainView {
    View* view;
    ViewDispatcher* view_dispatcher;
    FuriPubSubSubscription* subscription;
    void* context;
};

static const uint8_t PROGRESS_RENDER_STEP = 1; /* percent, to limit rendering rate */

typedef struct {
    FuriString* status;
    uint8_t progress, rendered_progress;
    bool failed;
} UpdaterProgressModel;

void updater_main_model_set_state(
    UpdaterMainView* main_view,
    const char* message,
    uint8_t progress,
    bool failed) {
    bool update = false;
    with_view_model(
        main_view->view,
        UpdaterProgressModel * model,
        {
            model->failed = failed;
            model->progress = progress;
            if(furi_string_cmp_str(model->status, message)) {
                furi_string_set(model->status, message);
                model->rendered_progress = progress;
                update = true;
            } else if(
                (model->rendered_progress > progress) ||
                ((progress - model->rendered_progress) > PROGRESS_RENDER_STEP)) {
                model->rendered_progress = progress;
                update = true;
            }
        },
        update);
}

View* updater_main_get_view(UpdaterMainView* main_view) {
    furi_assert(main_view);
    return main_view->view;
}

bool updater_main_input(InputEvent* event, void* context) {
    furi_assert(event);
    furi_assert(context);

    UpdaterMainView* main_view = context;
    if(!main_view->view_dispatcher) {
        return true;
    }

    if((event->type == InputTypeShort) && (event->key == InputKeyOk)) {
        view_dispatcher_send_custom_event(
            main_view->view_dispatcher, UpdaterCustomEventRetryUpdate);
    } else if((event->type == InputTypeLong) && (event->key == InputKeyBack)) {
        view_dispatcher_send_custom_event(
            main_view->view_dispatcher, UpdaterCustomEventCancelUpdate);
    }

    return true;
}

static void updater_main_draw_callback(Canvas* canvas, void* _model) {
    UpdaterProgressModel* model = _model;

    canvas_set_font(canvas, FontSecondary);

    if(model->failed) {
        r0n1n_ui_header(canvas, NULL, "Сбой обновления", NULL, -1);
        canvas_draw_icon(canvas, 3, 18, &I_Warning_30x23);
        elements_multiline_text_aligned(
            canvas, 81, 16, AlignCenter, AlignTop, furi_string_get_cstr(model->status));

        canvas_draw_line(canvas, 0, 51, 127, 51);
        canvas_draw_icon(canvas, 2, 54, &I_Ok_btn_9x9);
        canvas_draw_str(canvas, 14, 62, "повтор");
        canvas_draw_str_aligned(canvas, 83, 62, AlignRight, AlignBottom, "держи");
        canvas_draw_icon(canvas, 86, 55, &I_Pin_back_arrow_10x8);
        canvas_draw_str_aligned(canvas, 126, 62, AlignRight, AlignBottom, "выход");
    } else {
        r0n1n_ui_logo(canvas, (128 - R0N1N_UI_LOGO_WIDTH) / 2, 3, NULL);

        // "── ОБНОВЛЕНИЕ ──"
        r0n1n_ui_spaced_text(canvas, 64, 34, "ОБНОВЛЕНИЕ", 2, SIZE_MAX);
        canvas_draw_line(canvas, 4, 30, 28, 30);
        canvas_draw_line(canvas, 99, 30, 123, 30);

        char percent[8];
        snprintf(percent, sizeof(percent), "%u%%", (unsigned)MIN(model->progress, 100));
        r0n1n_ui_progress(canvas, 4, 38, 92, 9, model->progress);
        canvas_draw_str_aligned(canvas, 126, 46, AlignRight, AlignBottom, percent);

        FuriString* status = furi_string_alloc_set(model->status);
        r0n1n_ui_fit_width(canvas, status, 124);
        canvas_draw_str_aligned(
            canvas, 64, 61, AlignCenter, AlignBottom, furi_string_get_cstr(status));
        furi_string_free(status);
    }
}

UpdaterMainView* updater_main_alloc(void) {
    UpdaterMainView* main_view = malloc(sizeof(UpdaterMainView));

    main_view->view = view_alloc();
    view_allocate_model(main_view->view, ViewModelTypeLocking, sizeof(UpdaterProgressModel));

    with_view_model(
        main_view->view,
        UpdaterProgressModel * model,
        { model->status = furi_string_alloc_set("Ожидание SD-карты"); },
        true);

    view_set_context(main_view->view, main_view);
    view_set_input_callback(main_view->view, updater_main_input);
    view_set_draw_callback(main_view->view, updater_main_draw_callback);

    return main_view;
}

void updater_main_free(UpdaterMainView* main_view) {
    furi_assert(main_view);
    with_view_model(
        main_view->view, UpdaterProgressModel * model, { furi_string_free(model->status); }, false);
    view_free(main_view->view);
    free(main_view);
}

void updater_main_set_storage_pubsub(UpdaterMainView* main_view, FuriPubSubSubscription* sub) {
    main_view->subscription = sub;
}

FuriPubSubSubscription* updater_main_get_storage_pubsub(UpdaterMainView* main_view) {
    return main_view->subscription;
}

void updater_main_set_view_dispatcher(UpdaterMainView* main_view, ViewDispatcher* view_dispatcher) {
    main_view->view_dispatcher = view_dispatcher;
}
