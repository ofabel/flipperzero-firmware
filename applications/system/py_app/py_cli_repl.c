#include <stdio.h>
#include <py/repl.h>
#include <genhdr/mpversion.h>
#include <mp_flipper.h>
#include <cli/cli.h>
#include <furi.h>

#include "py_cli_i.h"

#define AUTOCOMPLETE_MANY_MATCHES (size_t)(-1)
#define HISTORY_SIZE 16

typedef struct {
    FuriString** stack;
    size_t pointer;
    size_t size;
} py_repl_history_t;

typedef struct {
    py_repl_history_t* history;
    FuriString* line;
    FuriString* code;
    size_t cursor;
    bool is_ps2;
} py_repl_context_t;

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

static py_repl_context_t* py_repl_context_alloc() {
    py_repl_context_t* context = malloc(sizeof(py_repl_context_t));

    context->history = py_repl_history_alloc();
    context->code = furi_string_alloc();
    context->line = furi_string_alloc();
    context->cursor = 0;
    context->is_ps2 = false;

    return context;
}

static void py_repl_context_free(py_repl_context_t* context) {
    py_repl_history_free(context->history);

    furi_string_free(context->code);
    furi_string_free(context->line);

    free(context);
}

static void print_full_psx(py_repl_context_t* context) {
    const char* psx = context->is_ps2 ? mp_repl_get_ps2() : mp_repl_get_ps1();

    printf("\33[2K\r%s%s", psx, furi_string_get_cstr(context->line));

    for(size_t i = context->cursor; i < furi_string_size(context->line); i++) {
        printf("\e[D");

        fflush(stdout);
    }

    fflush(stdout);
}

static void mp_flipper_print(FuriString* data, const char* str, size_t len) {
    for(size_t i = 0; i < len; i++) {
        furi_string_push_back(data, str[i]);
    }
}

inline static void handle_arrow_keys(char character, py_repl_context_t* context) {
    py_repl_history_t* history = context->history;

    // up arrow
    if(character == 'A' && history->pointer == 0) {
        furi_string_set(history->stack[0], context->line);
    }

    if(character == 'A' && history->pointer < history->size) {
        history->pointer += (history->pointer + 1) == history->size ? 0 : 1;

        furi_string_set(context->line, history->stack[history->pointer]);

        context->cursor = furi_string_size(context->line);

        print_full_psx(context);

        return;
    }

    // down arrow
    if(character == 'B' && history->pointer > 0) {
        history->pointer--;

        furi_string_set(context->line, history->stack[history->pointer]);

        context->cursor = furi_string_size(context->line);

        print_full_psx(context);

        return;
    }

    // right arrow
    if(character == 'C' && context->cursor != furi_string_size(context->line)) {
        printf("\e[C");

        fflush(stdout);

        context->cursor++;

        return;
    }

    // left arrow
    if(character == 'D' && context->cursor > 0) {
        printf("\e[D");

        fflush(stdout);

        context->cursor--;

        return;
    }
}

inline static void handle_backspace(py_repl_context_t* context) {
    // skip backspace at begin of line
    if(context->cursor == 0) {
        return;
    }

    printf("\e[D\e[1P");

    fflush(stdout);

    furi_string_left(context->line, furi_string_size(context->line) - 1);

    context->cursor--;
}

inline static void handle_autocomplete(Cli* cli, py_repl_context_t* context) {
    const char* new_line = furi_string_get_cstr(context->line);
    FuriString* orig_line = furi_string_alloc_printf("%s", new_line);
    const char* orig_line_str = furi_string_get_cstr(orig_line);
    const char* completion = malloc(128 * sizeof(char));

    mp_print_t* print = malloc(sizeof(mp_print_t));

    print->data = furi_string_alloc();
    print->print_strn = mp_flipper_print;

    size_t length = mp_repl_autocomplete(new_line, context->cursor, print, &completion);

    do {
        if(length == 0) {
            break;
        }

        if(length != AUTOCOMPLETE_MANY_MATCHES) {
            furi_string_printf(
                context->line,
                "%.*s%.*s%s",
                context->cursor,
                orig_line_str,
                length,
                completion,
                orig_line_str + context->cursor);

            context->cursor += length;

            print_full_psx(context);

            break;
        }

        if(length == AUTOCOMPLETE_MANY_MATCHES) {
            printf("%s", furi_string_get_cstr(print->data));

            print_full_psx(context);

            break;
        }
    } while(false);

    furi_string_free(print->data);
    furi_string_free(orig_line);
    free(completion);
    free(print);
}

inline static void update_history(py_repl_context_t* context) {
    py_repl_history_t* history = context->history;

    if(furi_string_size(context->line) > 0) {
        history->size += history->size == HISTORY_SIZE ? 0 : 1;

        for(size_t i = history->size - 1; i > 1; i--) {
            furi_string_set(history->stack[i], history->stack[i - 1]);
        }

        furi_string_set(history->stack[1], context->line);
        furi_string_reset(history->stack[0]);
    }

    history->pointer = 0;
}

inline static bool continue_with_input(py_repl_context_t* context) {
    if(furi_string_empty(context->line)) {
        return false;
    }

    if(!mp_repl_continue_with_input(furi_string_get_cstr(context->code))) {
        return false;
    }

    return true;
}

void py_cli_repl_execute(Cli* cli, FuriString* args, void* ctx) {
    UNUSED(args);
    UNUSED(ctx);

    const size_t memory_size = memmgr_get_free_heap() * 0.3;
    const size_t stack_size = 2 * 1024;
    uint8_t* memory = malloc(memory_size * sizeof(uint8_t));

    printf("MicroPython (%s, %s) on Flipper Zero\r\n", MICROPY_GIT_TAG, MICROPY_BUILD_DATE);
    printf("Allocated memory is %zu bytes, stack size is %zu bytes\r\n", memory_size, stack_size);

    py_repl_context_t* context = py_repl_context_alloc();

    mp_flipper_set_root_module_path("/ext");
    mp_flipper_init(memory + stack_size, memory_size - stack_size, memory);

    char character = '\0';

    bool exit = false;
    bool wait_for_lf = false;

    // REPL loop
    do {
        furi_string_reset(context->code);

        context->is_ps2 = false;

        // scan line loop
        do {
            furi_string_reset(context->line);

            context->cursor = 0;

            print_full_psx(context);

            // scan character loop
            do {
                character = cli_getc(cli);

                // Ctrl + C
                if(character == CliSymbolAsciiETX) {
                    context->cursor = 0;

                    furi_string_reset(context->line);
                    furi_string_reset(context->code);

                    printf("\r\nKeyboardInterrupt\r\n");

                    break;
                }

                // Ctrl + D
                if(character == CliSymbolAsciiEOT) {
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
                    furi_string_push_back(context->code, '\n');
                    furi_string_cat(context->code, context->line);
                    furi_string_trim(context->code);

                    cli_nl(cli);

                    break;
                }

                // handle arrow keys
                if(character >= 0x18 && character <= 0x1B) {
                    character = cli_getc(cli);
                    character = cli_getc(cli);

                    handle_arrow_keys(character, context);

                    continue;
                }

                // handle tab, do autocompletion
                if(character == CliSymbolAsciiTab) {
                    handle_autocomplete(cli, context);

                    continue;
                }

                // handle backspace
                if(character == CliSymbolAsciiBackspace) {
                    handle_backspace(context);

                    continue;
                }

                // append at end
                if(context->cursor == furi_string_size(context->line)) {
                    cli_putc(cli, character);

                    furi_string_push_back(context->line, character);

                    context->cursor++;

                    continue;
                }

                // insert between
                if(context->cursor < furi_string_size(context->line)) {
                    const char temp[2] = {character, 0};
                    furi_string_replace_at(context->line, context->cursor++, 0, temp);

                    printf("\e[4h%c\e[4l", character);
                    fflush(stdout);

                    continue;
                }
            } while(true);

            // Ctrl + D
            if(exit) {
                break;
            }

            update_history(context);
        } while(context->is_ps2 = continue_with_input(context));

        // Ctrl + D
        if(exit) {
            break;
        }

        mp_flipper_exec_str(furi_string_get_cstr(context->code));
    } while(true);

    mp_flipper_deinit();

    py_repl_context_free(context);

    free(memory);
}
