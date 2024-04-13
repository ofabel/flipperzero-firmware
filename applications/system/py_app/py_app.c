#include <furi.h>

#include "py_app.h"

int32_t py_app(void* arg) {
    size_t stack;

    if(arg == NULL || strlen(arg) == 0) {
        return 0;
    }

    const char* path = arg;
    FuriString* file_path = furi_string_alloc_printf("%s", path);

    do {
        printf("Running script %s, press CTRL+C to stop\r\n", path);

        const size_t memory_size = memmgr_get_free_heap() * 0.3;
        const size_t stack_size = 2 * 1024;
        uint8_t* memory = malloc(memory_size * sizeof(uint8_t));

        printf("allocated memory is %zu bytes\r\n", memory_size);
        printf("stack size is %zu bytes\r\n", stack_size);

        size_t index = furi_string_search_rchar(file_path, '/');

        furi_check(index != FURI_STRING_FAILURE);

        bool is_py_file = furi_string_end_with_str(file_path, ".py");

        furi_string_left(file_path, index);

        mp_flipper_set_root_module_path(furi_string_get_cstr(file_path));

        mp_flipper_init(memory, memory_size, stack_size, &stack);

        if(is_py_file) {
            mp_flipper_exec_py_file(path);
        } else {
            mp_flipper_exec_mpy_file(path);
        }

        mp_flipper_deinit();

        free(memory);
    } while(false);

    furi_string_free(file_path);

    return 0;
}
