#include <furi.h>

#include <mp_flipper.h>

inline void mp_flipper_nlr_jump_fail(void* val) {
    furi_crash();
}

inline void mp_flipper_assert(const char* file, int line, const char* func, const char* expr) {
}

inline void mp_flipper_fatal_error(const char* msg) {
    furi_crash(msg);
}
