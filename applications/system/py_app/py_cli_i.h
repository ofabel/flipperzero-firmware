#include <cli/cli.h>
#include <furi.h>

#include "py_app.h"

void py_cli_repl_execute(Cli* cli, FuriString* args, void* ctx);
void py_cli_file_execute(Cli* cli, FuriString* args, void* ctx);
void py_cli_file_compile(Cli* cli, FuriString* args, void* ctx);
