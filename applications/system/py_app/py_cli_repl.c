#include <stdio.h>
#include <py/repl.h>
#include <genhdr/mpversion.h>
#include <mp_flipper.h>
#include <cli/cli.h>
#include <furi.h>

#define TAG "py repl"
#define PSX(is_ps2) (is_ps2 ? "... " : ">>> ")
#define AUTOCOMPLETE_MANY_MATCHES (size_t)(-1)
#define HISTORY_SIZE 16

typedef struct {
    FuriString** stack;
    size_t pointer;
    size_t size;
} py_repl_history_t;

static py_repl_history_t* py_repl_history_alloc() {
    py_repl_history_t* history = malloc(sizeof(py_repl_history_t));

    history->stack = malloc(HISTORY_SIZE * sizeof(FuriString*));
    history->pointer = 0;
    history->size = 1;

    for(size_t i = 0; i < HISTORY_SIZE; i++) {
        history->stack[i] = furi_string_alloc();
    }

    return history;
}

static void py_repl_history_free(py_repl_history_t* history) {
    for(size_t i = 0; i < HISTORY_SIZE; i++) {
        furi_string_free(history->stack[i]);
    }

    free(history);
}

static void mp_flipper_print(FuriString* data, const char* str, size_t len) {
    for(size_t i = 0; i < len; i++) {
        furi_string_push_back(data, str[i]);
    }
}

inline static int32_t handle_arrow_keys(
    char character,
    size_t cursor,
    bool is_ps2,
    FuriString* line,
    py_repl_history_t* history) {
    // up arrow
    if(character == 'A' && history->pointer == 0) {
        furi_string_set(history->stack[0], line);
    }

    if(character == 'A' && history->pointer < history->size) {
        history->pointer += (history->pointer + 1) == history->size ? 0 : 1;

        furi_string_set(line, history->stack[history->pointer]);

        printf("\33[2K\r");
        printf(PSX(is_ps2));
        printf(furi_string_get_cstr(line));

        fflush(stdout);

        return furi_string_size(line);
    }

    // down arrow
    if(character == 'B' && history->pointer > 0) {
        history->pointer--;

        furi_string_set(line, history->stack[history->pointer]);

        printf("\33[2K\r");
        printf(PSX(is_ps2));
        printf(furi_string_get_cstr(line));

        fflush(stdout);

        return furi_string_size(line);
    }

    // right arrow
    if(character == 'C' && cursor != furi_string_size(line)) {
        printf("\e[C");

        fflush(stdout);

        return cursor + 1;
    }

    // left arrow
    if(character == 'D' && cursor > 0) {
        printf("\e[D");

        fflush(stdout);

        return cursor - 1;
    }

    return cursor;
}

inline static int32_t handle_backspace(size_t cursor, FuriString* line) {
    // skip backspace at begin of line
    if(cursor == 0) {
        return 0;
    }

    printf("\e[D\e[1P");

    fflush(stdout);

    furi_string_left(line, furi_string_size(line) - 1);

    return -1;
}

inline static int32_t handle_autocomplete(Cli* cli, bool is_ps2, size_t cursor, FuriString* line) {
    const char* line_str = furi_string_get_cstr(line);

    mp_print_t* print = malloc(sizeof(mp_print_t));

    char* compl_str = malloc(128 * sizeof(char));

    print->data = furi_string_alloc();
    print->print_strn = mp_flipper_print;

    size_t len = mp_repl_autocomplete(line_str, furi_string_size(line), print, &compl_str);

    if(len != AUTOCOMPLETE_MANY_MATCHES) {
        for(size_t i = 0; i < len; i++) {
            cli_putc(cli, compl_str[i]);

            furi_string_push_back(line, compl_str[i]);
        }
    } else {
        printf("%s", furi_string_get_cstr(print->data));
        printf(PSX(is_ps2));
        printf("%s", line_str);

        fflush(stdout);
    }

    furi_string_free(print->data);
    free(compl_str);
    free(print);

    return len == AUTOCOMPLETE_MANY_MATCHES ? 0 : len;
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

    py_repl_history_t* history = py_repl_history_alloc();

    char* line_str = furi_string_get_cstr(line);
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
            cli_write(cli, PSX(is_ps2), 4);

            furi_string_reset(line);
            cursor = 0;

            do {
                line_str = furi_string_get_cstr(line);

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

                // handle arrow keys
                if(character >= 0x18 && character <= 0x1B) {
                    character = cli_getc(cli);
                    character = cli_getc(cli);

                    cursor = handle_arrow_keys(character, cursor, is_ps2, line, history);

                    continue;
                }

                // handle tab, do autocompletion
                if(character == CliSymbolAsciiTab) {
                    cursor += handle_autocomplete(cli, is_ps2, cursor, line);

                    continue;
                }

                // handle backspace
                if(character == CliSymbolAsciiBackspace) {
                    cursor += handle_backspace(cursor, line);

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

            if(furi_string_size(line) > 0) {
                history->size += history->size == HISTORY_SIZE ? 0 : 1;

                for(size_t i = history->size - 1; i > 1; i--) {
                    furi_string_set(history->stack[i], history->stack[i - 1]);
                }

                furi_string_set(history->stack[1], line);
                furi_string_reset(history->stack[0]);
            }

            history->pointer = 0;

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

    py_repl_history_free(history);

    furi_string_free(line);
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