#include <mp_flipper.h>
#include <storage/storage.h>

#include "py_cli_i.h"

void py_cli_file_execute(Cli* cli, FuriString* args, void* context) {
    size_t stack;
    UNUSED(cli);
    UNUSED(context);

    const char* path = furi_string_get_cstr(args);
    FuriString* code = furi_string_alloc();
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

        furi_string_left(file_path, index);

        mp_flipper_set_root_module_path(furi_string_get_cstr(file_path));

        mp_flipper_init(memory, memory_size, stack_size, &stack);
        mp_flipper_exec_file(path);
        mp_flipper_deinit();

        free(memory);
    } while(false);

    furi_string_free(file_path);
    furi_string_free(code);
}
