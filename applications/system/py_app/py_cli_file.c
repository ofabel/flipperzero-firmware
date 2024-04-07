#include <mp_flipper.h>
#include <storage/storage.h>

#include "py_cli_i.h"

bool load_python_file(const char* file_path, FuriString* code) {
    Storage* storage = furi_record_open(RECORD_STORAGE);

    File* file = storage_file_alloc(storage);

    bool result = storage_file_open(file, file_path, FSAM_READ, FSOM_OPEN_EXISTING);

    if(result) {
        size_t read = 0;

        do {
            uint8_t buffer[64] = {'\0'};

            read = storage_file_read(file, buffer, sizeof(buffer) - 1);

            for(size_t i = 0; i < read; i++) {
                furi_string_push_back(code, buffer[i]);
            }
        } while(read > 0);

        furi_string_trim(code);
    }

    storage_file_free(file);

    furi_record_close(RECORD_STORAGE);

    return result;
}

void py_cli_file_execute(Cli* cli, FuriString* args, void* context) {
    UNUSED(context);

    const char* path = furi_string_get_cstr(args);
    Storage* storage = furi_record_open(RECORD_STORAGE);
    FuriString* code = furi_string_alloc();

    do {
        if(!storage_file_exists(storage, path)) {
            printf("File not found: %s\r\n", path);

            break;
        }

        furi_record_close(RECORD_STORAGE);

        FuriString* file_path = args;

        if(!load_python_file(path, code)) {
            printf("Cannot open file: %s\r\n", path);

            break;
        }

        printf("Running script %s, press CTRL+C to stop\r\n", path);

        const size_t memory_size = memmgr_get_free_heap() * 0.5;
        const size_t stack_size = 2 * 1024;
        uint8_t* memory = malloc(memory_size * sizeof(uint8_t));

        printf("allocated memory is %zu bytes\r\n", memory_size);
        printf("stack size is %zu bytes\r\n", stack_size);

        size_t index = furi_string_search_rchar(file_path, '/');

        furi_check(index != FURI_STRING_FAILURE);

        furi_string_left(file_path, index);

        mp_flipper_set_root_module_path(furi_string_get_cstr(file_path));

        mp_flipper_init(memory + stack_size, memory_size - stack_size, memory);
        mp_flipper_exec_str(furi_string_get_cstr(code));
        mp_flipper_deinit();

        free(memory);
    } while(false);

    furi_string_free(code);
}