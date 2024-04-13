#include <furi.h>
#include <gui/gui.h>

typedef struct {
    Gui* gui;
    ViewPort* view_port;
    Canvas* canvas;
} mp_flipper_context_t;
