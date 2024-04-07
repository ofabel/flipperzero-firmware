#include <stdio.h>

#include <furi.h>
#include <storage/storage.h>

#include <mp_flipper.h>
#include <mp_flipper_halport.h>

inline void mp_flipper_stdout_tx_str(const char* str) {
    printf("%s", str);
}

inline void mp_flipper_stdout_tx_strn_cooked(const char* str, size_t len) {
    printf("%.*s", len, str);
}

inline mp_flipper_import_stat_t mp_flipper_import_stat(const char* path) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    FuriString* file_path = furi_string_alloc();
    mp_flipper_import_stat_t stat = MP_FLIPPER_IMPORT_STAT_NO_EXIST;

    furi_string_printf(file_path, "%s/%s", mp_flipper_root_module_path, path);

    do {
        FileInfo* info = NULL;

        if(storage_common_stat(storage, furi_string_get_cstr(file_path), info) != FSE_OK) {
            break;
        }

        if((info->flags & FSF_DIRECTORY) == FSF_DIRECTORY) {
            stat = MP_FLIPPER_IMPORT_STAT_DIR;
            break;
        }

        stat = MP_FLIPPER_IMPORT_STAT_FILE;
    } while(false);

    furi_string_free(file_path);
    furi_record_close(RECORD_STORAGE);

    return stat;
}