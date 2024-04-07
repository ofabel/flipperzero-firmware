#include <furi.h>

#include <mp_flipper.h>

inline void mp_flipper_nlr_jump_fail(void* val) {
    furi_crash();
}