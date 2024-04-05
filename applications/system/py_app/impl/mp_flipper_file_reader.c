#include <stdio.h>

#include <furi.h>
#include <storage/storage.h>

#include <mp_flipper.h>
#include <mp_flipper_file_reader.h>

typedef struct {
    size_t pointer;
    FuriString* content;
    size_t size;
} FileReaderContext;

inline void* mp_flipper_file_reader_context_alloc(const char* filename) {
    FuriString* file_path = furi_string_alloc();
    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    furi_string_printf(file_path, "%s/%s", mp_flipper_root_module_path, filename);

    if(!storage_file_open(file, furi_string_get_cstr(file_path), FSAM_READ, FSOM_OPEN_EXISTING)) {
        furi_crash("unable to open file");
    }

    FileReaderContext* ctx = malloc(sizeof(FileReaderContext));

    ctx->pointer = 0;
    ctx->content = furi_string_alloc();
    ctx->size = storage_file_size(file);

    char character = '\0';

    for(size_t i = 0; i < ctx->size; i++) {
        storage_file_read(file, &character, 1);

        furi_string_push_back(ctx->content, character);
    }

    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
    furi_string_free(file_path);
}

inline uint32_t mp_flipper_file_reader_read(void* data) {
    FileReaderContext* ctx = data;

    if(ctx->pointer >= ctx->size) {
        return MP_FLIPPER_FILE_READER_EOF;
    }

    return furi_string_get_char(ctx->content, ctx->pointer++);
}

void mp_flipper_file_reader_close(void* data) {
    FileReaderContext* ctx = data;

    furi_string_free(ctx->content);

    free(data);
}