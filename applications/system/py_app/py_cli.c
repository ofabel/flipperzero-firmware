#include <furi.h>

#include "py_cli_i.h"

void py_cli_execute(Cli* cli, FuriString* args, void* ctx) {
    if(furi_string_empty(args)) {
        printf("Usage: py <path>");
    } else {
        py_cli_file_execute(cli, args, ctx);
    }
}

void py_cli_compiler(Cli* cli, FuriString* args, void* ctx) {
    if(furi_string_empty(args)) {
        py_cli_repl_execute(cli, args, ctx);
    } else {
        py_cli_file_compile(cli, args, ctx);
    }
}

void py_cli_on_system_start(void) {
#ifdef SRV_CLI
    Cli* cli = furi_record_open(RECORD_CLI);

#ifdef MP_FLIPPER_RUNTIME
    cli_add_command(cli, "py", CliCommandFlagDefault, py_cli_execute, NULL);
#endif

#ifdef MP_FLIPPER_COMPILER
    cli_add_command(cli, "pyc", CliCommandFlagDefault, py_cli_compiler, NULL);
#endif

    furi_record_close(RECORD_CLI);
#endif
}
