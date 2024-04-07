#include <furi.h>

#include "py_cli_i.h"

void py_cli_execute(Cli* cli, FuriString* args, void* context) {
    if(furi_string_empty(args)) {
        py_cli_repl_execute(cli, args, context);
    } else {
        py_cli_file_execute(cli, args, context);
    }
}
void py_cli_on_system_start(void) {
#ifdef SRV_CLI
    Cli* cli = furi_record_open(RECORD_CLI);
    cli_add_command(cli, "py", CliCommandFlagDefault, py_cli_execute, NULL);
    furi_record_close(RECORD_CLI);
#endif
}
