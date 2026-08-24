#include "tokeid.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOKEID_DEBUG

#ifdef TOKEID_DEBUG
#define REPORT_ERROR fprintf(stderr, "Tokeid Error -> %s | %d\n", __FILE__, __LINE__)
#else
#define REPORT_ERROR NULL
#define NDEBUG
#endif

#include <assert.h>

#define HARDLIMIT INT32_MAX

typedef struct TokeidCommand {
    TokeidFunc func;
    char* keyword;
    char* help_text;
} TokeidCommand;

static TokeidCommand* commands = NULL;
static size_t commands_size = 0;
static size_t commands_capacity = 0;
static int should_close = 0;

static int tokenize(char out[MAX_ARGS][MAX_INPUT_LENGTH], const char* const in, int* argc);
static int read_line(char* buf, const int size);
static int commands_print(int argc, char argv[MAX_ARGS][MAX_INPUT_LENGTH]);
static int command_exists(const char* const keyword);

int tokeid_init(const size_t new_capacity)
{
    if (new_capacity > (HARDLIMIT - 2)) {
        REPORT_ERROR;
        return 1;
    }
    commands = malloc((new_capacity + 2) * sizeof(*commands));
    if (commands == NULL) {
        REPORT_ERROR;
        return 1;
    }

    commands_capacity = new_capacity;

    tokeid_command_create_ex("quit", tokeid_close, "Quits the program");
    tokeid_command_create_ex("help", commands_print, "Shows this help");

    return 0;
}

void tokeid_cleanup(void)
{
    free(commands);
}

int tokeid_should_close(void)
{
    return should_close;
}

int tokeid_close(int argc, char argv[MAX_ARGS][MAX_INPUT_LENGTH])
{
    (void)argc;
    (void)argv;
    should_close = 1;
    return 0;
}

int tokeid_command_create(char* keyword, TokeidFunc func)
{
    if (commands_size >= commands_capacity || keyword == NULL) {
        REPORT_ERROR;
        return 1;
    }
    if (command_exists(keyword)) {
        REPORT_ERROR;
        return 1;
    }

    commands[commands_size] = (TokeidCommand) {
        .func = func,
        .keyword = keyword,
        .help_text = "",
    };

    ++commands_size;
    return 0;
}

int tokeid_command_create_ex(char* keyword, TokeidFunc func, char* help_text)
{
    if (commands_size >= commands_capacity || keyword == NULL || help_text == NULL) {
        REPORT_ERROR;
        return 1;
    }
    if (command_exists(keyword)) {
        REPORT_ERROR;
        return 1;
    }

    commands[commands_size] = (TokeidCommand) {
        .func = func,
        .keyword = keyword,
        .help_text = help_text,
    };
    ++commands_size;
    return 0;
}

int tokeid_get_input(const char* const prompt)
{
    char token[MAX_INPUT_LENGTH];
    char args[MAX_ARGS][MAX_INPUT_LENGTH];
    int argc;

    if (prompt != NULL)
        printf("%s", prompt);

    if (read_line(token, MAX_INPUT_LENGTH)) {
        REPORT_ERROR;
        return 1;
    }

    if (tokenize(args, token, &argc)) {
        REPORT_ERROR;
        return 1;
    }

    // TODO: change to hashmap in future update
    for (size_t i = 0; i < commands_size; ++i) {
        if (strcmp(args[0], commands[i].keyword))
            continue;
        commands[i].func(argc, args);
        return 0;
    }

    return 1;
}

/******************************************************************************************/
// static internals

static int tokenize(char out[MAX_ARGS][MAX_INPUT_LENGTH], const char* const in, int* argc)
{
    *argc = 0;
    int out_idx = 0;

    for (int i = 0 ;; ++i) {
        if (*argc >= MAX_ARGS || out_idx >= MAX_INPUT_LENGTH) {
            REPORT_ERROR;
            return 1;
        }

        if (in[i] == ' ' || in[i] == '\0') {
            out[*argc][out_idx] = '\0';
            ++(*argc);
            out_idx = 0;
        } else {
            out[*argc][out_idx++] = in[i];
        }

        if (in[i] == '\0') {
            return 0;
        }
    }
}

static int read_line(char* buf, const int size)
{
    if (NULL == fgets(buf, size, stdin)) {
        REPORT_ERROR;
        return 1;
    }
    buf[strcspn(buf, "\n")] = '\0';
    return 0;
}

static int commands_print(int argc, char argv[MAX_ARGS][MAX_INPUT_LENGTH])
{
    (void)argc;
    (void)argv;

    for (size_t i = 0; i < commands_size; ++i)
        printf("%s | %s\n", commands[i].keyword, commands[i].help_text);
    return 0;
}

// returns 1 if the command already exists
static int command_exists(const char* const keyword)
{
    if (keyword == NULL) return 0;

    for (size_t i = 0; i < commands_size; ++i) {
        if (0 == strcmp(keyword, commands[i].keyword))
            return 1;
    } 

    return 0;
}







