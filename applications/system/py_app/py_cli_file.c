#include <furi.h>

#include <mp_flipper_compiler.h>

#include "py_cli_i.h"

void py_cli_file_execute(Cli* cli, FuriString* args, void* context) {
    UNUSED(cli);
    UNUSED(context);

    py_app_file_execute(args);
}
