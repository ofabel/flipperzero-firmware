#include <stdio.h>
#include <py/repl.h>
#include <genhdr/mpversion.h>
#include <mp_flipper.h>
#include <cli/cli.h>
#include <furi.h>

#define TAG "py repl"

inline static int8_t handle_escape(char character, size_t cursor, FuriString* line) {
    // left arrow
    if(character == 'D' && cursor > 0) {
        printf("\e[D");

        fflush(stdout);

        return -1;
    }

    // right arrow
    if(character == 'C' && cursor != furi_string_size(line)) {
        printf("\e[C");

        fflush(stdout);

        return 1;
    }

    return 0;
}

void py_cli_repl_execute(Cli* cli, FuriString* args, void* context) {
    UNUSED(context);

    const size_t memory_size = memmgr_get_free_heap() * 0.3;
    const size_t stack_size = 2 * 1024;
    uint8_t* memory = malloc(memory_size * sizeof(uint8_t));

    printf("MicroPython (%s, %s) on Flipper Zero\r\n", MICROPY_GIT_TAG, MICROPY_BUILD_DATE);
    printf("Allocated memory is %zu bytes, stack size is %zu bytes\r\n", memory_size, stack_size);

    FuriString* code = furi_string_alloc();
    FuriString* line = furi_string_alloc();
    char* code_str = furi_string_get_cstr(code);

    mp_flipper_set_root_module_path("/ext");
    mp_flipper_init(memory + stack_size, memory_size - stack_size, memory);

    char character = '\0';

    bool exit = false;
    bool wait_for_lf = false;
    bool is_ps2 = false;
    size_t cursor = 0;

    do {
        do {
            cli_write(cli, is_ps2 ? "... " : ">>> ", 4);

            furi_string_reset(line);
            cursor = 0;

            do {
                character = cli_getc(cli);

                // ctrl + c
                if(character == CliSymbolAsciiETX) {
                    exit = true;

                    break;
                }

                // wait for line feed character
                if(character == CliSymbolAsciiCR) {
                    wait_for_lf = true;

                    continue;
                }

                // skip if we don't wait for line feed
                if(!wait_for_lf && character == CliSymbolAsciiLF) {
                    continue;
                }

                // handle line feed
                if(wait_for_lf && character == CliSymbolAsciiLF) {
                    furi_string_push_back(code, '\n');
                    furi_string_cat(code, line);
                    furi_string_trim(code);

                    cli_nl(cli);

                    break;
                }

                if(character >= 0x18 && character <= 0x1B) {
                    character = cli_getc(cli);
                    character = cli_getc(cli);

                    cursor += handle_escape(character, cursor, line);

                    continue;
                }

                // skip backspace at begin of line
                if(character == CliSymbolAsciiBackspace && cursor == 0) {
                    continue;
                }

                // handle backspace
                if(character == CliSymbolAsciiBackspace && cursor > 0) {
                    printf("\e[D\e[1P");

                    fflush(stdout);

                    furi_string_left(line, furi_string_size(line) - 1);

                    cursor--;

                    continue;
                }

                // append at end
                if(cursor == furi_string_size(line)) {
                    cli_putc(cli, character);

                    furi_string_push_back(line, character);

                    cursor++;

                    continue;
                }

                // insert between
                if(cursor < furi_string_size(line)) {
                    const char temp[2] = {character, 0};
                    furi_string_replace_at(line, cursor++, 0, temp);

                    printf("\e[4h%c\e[4l", character);
                    fflush(stdout);

                    continue;
                }
            } while(true);

            // ctrl + c
            if(exit) {
                break;
            }

            code_str = furi_string_get_cstr(code);

        } while(is_ps2 = !furi_string_empty(line) && mp_repl_continue_with_input(code_str));

        // ctrl + c
        if(exit) {
            break;
        }

        mp_flipper_exec_str(code_str);

        is_ps2 = false;

        furi_string_reset(code);
    } while(true);

    mp_flipper_deinit();

    furi_string_free(code);
    free(memory);
}

void py_app_repl_on_system_start(void) {
#ifdef SRV_CLI
    Cli* cli = furi_record_open(RECORD_CLI);
    cli_add_command(cli, "py-repl", CliCommandFlagDefault, py_cli_repl_execute, NULL);
    furi_record_close(RECORD_CLI);
#endif
}