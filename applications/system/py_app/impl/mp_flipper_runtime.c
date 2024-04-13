#include <furi.h>
#include <storage/storage.h>

#include <mp_flipper_runtime.h>
#include <mp_flipper_modflipperzero.h>

#include "mp_flipper_context.h"

void mp_flipper_save_file(const char* file_path, const char* data, size_t size) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    do {
        if(!storage_file_open(file, file_path, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
            storage_file_free(file);

            mp_flipper_raise_os_error_with_filename(MP_ENOENT, file_path);

            break;
        }

        storage_file_write(file, data, size);
    } while(false);

    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
}

inline void mp_flipper_nlr_jump_fail(void* val) {
    furi_crash();
}

inline void mp_flipper_assert(const char* file, int line, const char* func, const char* expr) {
}

inline void mp_flipper_fatal_error(const char* msg) {
    furi_crash(msg);
}

const char* mp_flipper_print_get_data(void* data) {
    return furi_string_get_cstr(data);
}

size_t mp_flipper_print_get_data_length(void* data) {
    return furi_string_size(data);
}

void* mp_flipper_print_data_alloc() {
    return furi_string_alloc();
}

void mp_flipper_print_strn(void* data, const char* str, size_t length) {
    for(size_t i = 0; i < length; i++) {
        furi_string_push_back(data, str[i]);
    }
}

void mp_flipper_print_data_free(void* data) {
    furi_string_free(data);
}

void* mp_flipper_context_alloc() {
    mp_flipper_context_t* ctx = malloc(sizeof(mp_flipper_context_t));

    ctx->gui = furi_record_open(RECORD_GUI);
    ctx->view_port = view_port_alloc();
    ctx->canvas = gui_direct_draw_acquire(ctx->gui);

    gui_add_view_port(ctx->gui, ctx->view_port, GuiLayerFullscreen);

    return ctx;
}

void mp_flipper_context_free(void* context) {
    mp_flipper_context_t* ctx = context;

    gui_direct_draw_release(ctx->gui);

    gui_remove_view_port(ctx->gui, ctx->view_port);

    view_port_free(ctx->view_port);

    furi_record_close(RECORD_GUI);

    free(ctx);
}
