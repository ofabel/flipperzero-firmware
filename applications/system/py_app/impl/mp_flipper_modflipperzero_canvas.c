#include <gui/gui.h>

#include <mp_flipper_modflipperzero.h>
#include <mp_flipper_runtime.h>

#include "mp_flipper_context.h"

inline void mp_flipper_canvas_draw_dot(uint8_t x, uint8_t y) {
    mp_flipper_context_t* ctx = mp_flipper_context;

    canvas_draw_dot(ctx->canvas, x, y);
}

inline void mp_flipper_canvas_set_color(uint8_t color) {
    mp_flipper_context_t* ctx = mp_flipper_context;

    canvas_set_color(ctx->canvas, color == MP_FLIPPER_COLOR_BLACK ? ColorBlack : ColorWhite);
}

inline void mp_flipper_canvas_set_text(uint8_t x, uint8_t y, const char* text) {
    mp_flipper_context_t* ctx = mp_flipper_context;

    canvas_draw_str(ctx->canvas, x, y, text);
}

inline void mp_flipper_canvas_update() {
    mp_flipper_context_t* ctx = mp_flipper_context;

    canvas_commit(ctx->canvas);
}
