#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>
#include <gui/modules/popup.h>
#include <storage/storage.h>

#include "academy_lessons.h"

// R0N1N Academy: short lessons on using the Flipper, each ending with one
// question. Passed lessons are remembered on the SD card.

#define ACADEMY_PROGRESS_PATH APP_DATA_PATH("progress")

typedef enum {
    AcademyViewList,
    AcademyViewLesson,
    AcademyViewQuiz,
    AcademyViewResult,
} AcademyView;

typedef struct {
    uint32_t lesson;
    uint32_t page;
} AcademyLessonModel;

typedef struct {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    Submenu* list;
    View* lesson_view;
    Submenu* quiz;
    Popup* result;
    uint32_t lesson;
    uint32_t passed; // bit per lesson
    bool last_answer_correct;
    FuriString* labels[ACADEMY_LESSON_COUNT];
    FuriString* header;
} Academy;

static uint32_t academy_page_count(uint32_t lesson) {
    uint32_t n = 0;
    while(n < ACADEMY_MAX_PAGES && academy_lessons[lesson].pages[n])
        n++;
    return n;
}

static void academy_load_progress(Academy* app) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    if(storage_file_open(file, ACADEMY_PROGRESS_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        if(storage_file_read(file, &app->passed, sizeof(app->passed)) != sizeof(app->passed)) {
            app->passed = 0;
        }
    }
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
}

static void academy_save_progress(Academy* app) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    if(storage_file_open(file, ACADEMY_PROGRESS_PATH, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        storage_file_write(file, &app->passed, sizeof(app->passed));
    }
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
}

// Lesson list ---------------------------------------------------------------

static void academy_list_callback(void* context, uint32_t index);

static void academy_build_list(Academy* app) {
    submenu_reset(app->list);
    uint32_t passed = 0;
    for(uint32_t i = 0; i < ACADEMY_LESSON_COUNT; i++) {
        const bool done = app->passed & (1UL << i);
        if(done) passed++;
        furi_string_printf(
            app->labels[i], "%s%lu. %s", done ? "+ " : "", i + 1, academy_lessons[i].title);
        submenu_add_item(
            app->list, furi_string_get_cstr(app->labels[i]), i, academy_list_callback, app);
    }
    furi_string_printf(app->header, "Академия: %lu из %u", passed, (unsigned)ACADEMY_LESSON_COUNT);
    submenu_set_header(app->list, furi_string_get_cstr(app->header));
    submenu_set_selected_item(app->list, app->lesson);
}

static void academy_list_callback(void* context, uint32_t index) {
    Academy* app = context;
    app->lesson = index;
    with_view_model(
        app->lesson_view,
        AcademyLessonModel * model,
        {
            model->lesson = index;
            model->page = 0;
        },
        true);
    view_dispatcher_switch_to_view(app->view_dispatcher, AcademyViewLesson);
}

// Lesson pages --------------------------------------------------------------

static void academy_lesson_draw(Canvas* canvas, void* model) {
    AcademyLessonModel* m = model;
    const AcademyLesson* lesson = &academy_lessons[m->lesson];
    const uint32_t pages = academy_page_count(m->lesson);

    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_box(canvas, 0, 0, 128, 11);
    canvas_set_color(canvas, ColorWhite);
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 2, 9, lesson->title);
    char page[12];
    snprintf(page, sizeof(page), "%lu/%lu", m->page + 1, pages);
    canvas_draw_str_aligned(canvas, 126, 9, AlignRight, AlignBottom, page);
    canvas_set_color(canvas, ColorBlack);

    // Up to 4 lines, 10 px apart, above a hint row
    const char* text = lesson->pages[m->page];
    char line[64];
    int32_t y = 21;
    while(*text && y <= 51) {
        const char* end = strchr(text, '\n');
        size_t len = end ? (size_t)(end - text) : strlen(text);
        if(len >= sizeof(line)) len = sizeof(line) - 1;
        memcpy(line, text, len);
        line[len] = 0;
        canvas_draw_str(canvas, 2, y, line);
        y += 10;
        text += len + (end ? 1 : 0);
    }

    canvas_draw_line(canvas, 0, 53, 127, 53);
    if(m->page > 0) canvas_draw_str(canvas, 2, 63, "< назад");
    canvas_draw_str_aligned(
        canvas, 126, 63, AlignRight, AlignBottom, m->page + 1 < pages ? "далее >" : "OK: вопрос");
}

static void academy_start_quiz(Academy* app);

static bool academy_lesson_input(InputEvent* event, void* context) {
    Academy* app = context;
    if(event->type != InputTypeShort) return false;
    if(event->key != InputKeyRight && event->key != InputKeyOk && event->key != InputKeyLeft)
        return false;

    bool quiz = false;
    with_view_model(
        app->lesson_view,
        AcademyLessonModel * m,
        {
            const uint32_t pages = academy_page_count(m->lesson);
            if(event->key == InputKeyLeft) {
                if(m->page > 0) m->page--;
            } else if(m->page + 1 < pages) {
                m->page++;
            } else {
                quiz = true;
            }
        },
        true);
    if(quiz) academy_start_quiz(app);
    return true;
}

// Question ------------------------------------------------------------------

static void academy_result_callback(void* context) {
    Academy* app = context;
    if(app->last_answer_correct) {
        academy_build_list(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, AcademyViewList);
    } else {
        view_dispatcher_switch_to_view(app->view_dispatcher, AcademyViewQuiz);
    }
}

static void academy_quiz_callback(void* context, uint32_t index) {
    Academy* app = context;
    const AcademyLesson* lesson = &academy_lessons[app->lesson];
    app->last_answer_correct = index == lesson->correct;

    popup_reset(app->result);
    if(app->last_answer_correct) {
        app->passed |= 1UL << app->lesson;
        academy_save_progress(app);
        popup_set_header(app->result, "Верно!", 64, 20, AlignCenter, AlignCenter);
        popup_set_text(app->result, "Урок пройден", 64, 40, AlignCenter, AlignCenter);
    } else {
        popup_set_header(app->result, "Не совсем", 64, 20, AlignCenter, AlignCenter);
        popup_set_text(app->result, "Попробуйте еще раз", 64, 40, AlignCenter, AlignCenter);
    }
    popup_set_timeout(app->result, 1500);
    popup_set_context(app->result, app);
    popup_set_callback(app->result, academy_result_callback);
    popup_enable_timeout(app->result);
    view_dispatcher_switch_to_view(app->view_dispatcher, AcademyViewResult);
}

static void academy_start_quiz(Academy* app) {
    const AcademyLesson* lesson = &academy_lessons[app->lesson];
    submenu_reset(app->quiz);
    submenu_set_header(app->quiz, lesson->question);
    for(uint32_t i = 0; i < ACADEMY_MAX_ANSWERS; i++) {
        if(lesson->answers[i]) {
            submenu_add_item(app->quiz, lesson->answers[i], i, academy_quiz_callback, app);
        }
    }
    view_dispatcher_switch_to_view(app->view_dispatcher, AcademyViewQuiz);
}

// Navigation ----------------------------------------------------------------

static uint32_t academy_exit(void* context) {
    UNUSED(context);
    return VIEW_NONE;
}

static uint32_t academy_to_list(void* context) {
    UNUSED(context);
    return AcademyViewList;
}

static uint32_t academy_to_lesson(void* context) {
    UNUSED(context);
    return AcademyViewLesson;
}

int32_t academy_app(void* p) {
    UNUSED(p);
    Academy* app = malloc(sizeof(Academy));
    for(size_t i = 0; i < ACADEMY_LESSON_COUNT; i++)
        app->labels[i] = furi_string_alloc();
    app->header = furi_string_alloc();
    academy_load_progress(app);

    app->gui = furi_record_open(RECORD_GUI);
    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    app->list = submenu_alloc();
    view_set_previous_callback(submenu_get_view(app->list), academy_exit);
    view_dispatcher_add_view(app->view_dispatcher, AcademyViewList, submenu_get_view(app->list));

    app->lesson_view = view_alloc();
    view_allocate_model(app->lesson_view, ViewModelTypeLocking, sizeof(AcademyLessonModel));
    view_set_context(app->lesson_view, app);
    view_set_draw_callback(app->lesson_view, academy_lesson_draw);
    view_set_input_callback(app->lesson_view, academy_lesson_input);
    view_set_previous_callback(app->lesson_view, academy_to_list);
    view_dispatcher_add_view(app->view_dispatcher, AcademyViewLesson, app->lesson_view);

    app->quiz = submenu_alloc();
    view_set_previous_callback(submenu_get_view(app->quiz), academy_to_lesson);
    view_dispatcher_add_view(app->view_dispatcher, AcademyViewQuiz, submenu_get_view(app->quiz));

    app->result = popup_alloc();
    view_dispatcher_add_view(app->view_dispatcher, AcademyViewResult, popup_get_view(app->result));

    academy_build_list(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, AcademyViewList);
    view_dispatcher_run(app->view_dispatcher);

    view_dispatcher_remove_view(app->view_dispatcher, AcademyViewList);
    view_dispatcher_remove_view(app->view_dispatcher, AcademyViewLesson);
    view_dispatcher_remove_view(app->view_dispatcher, AcademyViewQuiz);
    view_dispatcher_remove_view(app->view_dispatcher, AcademyViewResult);
    submenu_free(app->list);
    submenu_free(app->quiz);
    popup_free(app->result);
    view_free(app->lesson_view);
    view_dispatcher_free(app->view_dispatcher);
    furi_record_close(RECORD_GUI);
    for(size_t i = 0; i < ACADEMY_LESSON_COUNT; i++)
        furi_string_free(app->labels[i]);
    furi_string_free(app->header);
    free(app);
    return 0;
}
