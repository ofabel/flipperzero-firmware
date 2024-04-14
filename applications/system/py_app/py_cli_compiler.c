#include <furi.h>

#include <mp_flipper_compiler.h>

#include "py_cli_i.h"

void py_cli_file_compile(Cli* cli, FuriString* args, void* context) {
    size_t stack;
    UNUSED(cli);
    UNUSED(context);

    PyApp* app = py_app_alloc();
    const char* path = furi_string_get_cstr(args);
    FuriString* file_path = furi_string_alloc_printf("%s", path);
    FuriString* mpy_file_path = furi_string_alloc_printf("%s.mpy", path);

    do {
        const size_t heap_size = memmgr_get_free_heap() * app->heap_size;
        const size_t stack_size = app->stack_size;
        uint8_t* heap = malloc(heap_size * sizeof(uint8_t));

        FURI_LOG_D(TAG, "allocated memory is %zu bytes", heap_size);
        FURI_LOG_D(TAG, "stack size is %zu bytes", stack_size);

        size_t index = furi_string_search_rchar(file_path, '/');

        furi_check(index != FURI_STRING_FAILURE);

        bool is_py_file = furi_string_end_with_str(file_path, ".py");

        furi_string_left(file_path, index);

        mp_flipper_set_root_module_path(furi_string_get_cstr(file_path));

        mp_flipper_init(heap, heap_size, stack_size, &stack);

        if(is_py_file) {
            printf("compiling script %s\r\n", path);

            mp_flipper_compile_and_save_file(path, furi_string_get_cstr(mpy_file_path));
        } else {
            printf("can only compile .py files\r\n");
        }

        mp_flipper_deinit();

        free(heap);
    } while(false);

    furi_string_free(mpy_file_path);
    furi_string_free(file_path);

    py_app_free(app);
}
