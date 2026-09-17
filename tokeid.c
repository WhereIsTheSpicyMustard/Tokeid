#include "tokeid.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef NDEBUG
#define REPORT_ERROR ((void)0)
#else
#define REPORT_ERROR fprintf(stderr, "Tokeid Error -> %s | %d\n", __FILE__, __LINE__)
#endif

#include <assert.h>

#define HARDLIMIT (0xFFFF)

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
static int parse_hex(const int c);
static int num_from_dec(const char* restrict buf, int64_t* restrict out);
static int num_from_bin(const char* restrict buf, int64_t* restrict out);
static int num_from_hex(const char* restrict buf, int64_t* restrict out);

int tokeid_init(const size_t new_capacity)
{
    if (new_capacity > HARDLIMIT) {
        REPORT_ERROR;
        return 1;
    }
    commands = malloc(new_capacity * sizeof(*commands));
    if (commands == NULL) {
        REPORT_ERROR;
        return 1;
    }

    commands_capacity = new_capacity;

    assert(commands_size == 0);
    assert(commands_capacity >= 2);

    tokeid_command_create_ex("quit", tokeid_close, "Exits the program");
    tokeid_command_create_ex("help", commands_print, "Shows this help");

    return 0;
}

void tokeid_cleanup(void)
{
    free(commands);
    commands = NULL;
    commands_size = 0;
    commands_capacity = 0;
    should_close = 0;
}

int tokeid_should_close(void)
{
    return should_close;
}

// This function must take in unused arguments and return 0 unconditionally so that this function
// can be set as a default command
int tokeid_close(int argc, char argv[MAX_ARGS][MAX_INPUT_LENGTH])
{
    (void)argc;
    (void)argv;
    should_close = 1;
    return 0;
}

int tokeid_command_create(char* keyword, TokeidFunc func)
{
    if (commands_size >= commands_capacity || keyword == NULL || func == NULL) {
        REPORT_ERROR;
        return 1;
    }
    if (command_exists(keyword)) {
        REPORT_ERROR;
        return 1;
    }

    assert(commands_capacity >= 2);
    assert(commands_capacity > commands_size);

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
    if (commands_size >= commands_capacity || keyword == NULL || help_text == NULL || func == NULL) {
        REPORT_ERROR;
        return 1;
    }
    if (command_exists(keyword)) {
        REPORT_ERROR;
        return 1;
    }

    assert(commands_capacity >= 2);
    assert(commands_capacity > commands_size);

    commands[commands_size] = (TokeidCommand) {
        .func = func,
        .keyword = keyword,
        .help_text = help_text,
    };

    ++commands_size;
    return 0;
}

int tokeid_prompt_string(char out[MAX_INPUT_LENGTH], const char* const prompt)
{
    assert(NULL != out);
    if (NULL != prompt)
        printf("%s", prompt);

    if (read_line(out, MAX_INPUT_LENGTH)) {
        REPORT_ERROR;
        return TOKEID_IO_ERR;
    }

    return 0;
}

int tokeid_prompt_char(char out[MAX_INPUT_LENGTH], const char* const prompt)
{
    assert(NULL != out);
    while (1) {
        if (NULL != prompt)
            printf("%s", prompt);

        if (read_line(out, MAX_INPUT_LENGTH)) {
            REPORT_ERROR;
            return TOKEID_IO_ERR;
        }

        if (out[1] != '\0' || out[0] == '\0')
            continue;

        return 0;
    }
}

int tokeid_prompt_int(int64_t* restrict out, const char* const restrict prompt)
{
    assert(NULL != out);
    while (1) {
        *out = 0;

        if (NULL != prompt)
            printf("%s", prompt);

        char buf[MAX_INPUT_LENGTH] = {0};
        if (read_line(buf, MAX_INPUT_LENGTH)) {
            REPORT_ERROR;
            return TOKEID_IO_ERR;
        }

        const size_t buf_len = strnlen(buf, (size_t)MAX_INPUT_LENGTH);
        if (buf_len == 0)
            continue;
        if (-1 == parse_hex(buf[buf_len - 1]))
            continue;
        if (buf_len == 3 && buf[0] == '-' && buf[1] == '0') // prevents incomplete cases like "-0x" or "-0b"
            continue;
        if (buf_len == 2 && buf[0] == '0') // prevents incomplete cases like "0x" or "0b"
            continue;

        if ((buf[0] == '0' && buf[1] == 'x') || (buf[0] == '-' && buf[1] == '0' && buf[2] == 'x')) {
            if (num_from_hex(buf, out))
                continue;
        } else if ((buf[0] == '0' && buf[1] == 'b') || (buf[0] == '-' && buf[1] == '0' && buf[2] == 'b')) {
            if (num_from_bin(buf, out))
                continue;
        } else {
            if (num_from_dec(buf, out))
                continue;
        }

        return 0;
    }
}

// TODO:
int tokeid_prompt_double(double* out, const char* const prompt)
{
    (void)out;
    (void)prompt;
    return 1;
}

int tokeid_get_input(const char* const prompt, int* command_result)
{
    char token[MAX_INPUT_LENGTH];
    char args[MAX_ARGS][MAX_INPUT_LENGTH] = {{0}};
    int argc = -1;
    *command_result = 0;

    if (prompt != NULL)
        printf("%s", prompt);

    if (read_line(token, MAX_INPUT_LENGTH)) {
        REPORT_ERROR;
        return TOKEID_IO_ERR;
    }

    if (tokenize(args, token, &argc)) {
        REPORT_ERROR;
        return TOKEID_TOKENIZE_ERR;
    }

    if (args[0][0] == '\0')
        return TOKEID_EMPTY_LINE;

    assert(argc > 0);
    assert(commands_size >= 2);
    assert(commands_capacity >= commands_size);

    // TODO: change to hashmap in future update
    for (size_t i = 0; i < commands_size; ++i) {
        if (strcmp(args[0], commands[i].keyword))
            continue;
        *command_result = commands[i].func(argc, args);
        return 0;
    }

    return TOKEID_UNKNOWN_COMMAND;
}

/******************************************************************************************/
// static internals

static int num_from_hex(const char* restrict buf, int64_t* restrict out)
{
    const int start = 2 + (buf[0] == '-');
    if (buf[start] == '0') return 1;
    for (int i = start; i < MAX_INPUT_LENGTH && buf[i] != '\0'; ++i) {
        const int c = parse_hex((int)buf[i]);
        if (c == -1) return 1;

        if ((*out) > ((INT64_MAX - c) >> 4))
            return 1;
        *out = ((*out) << 4) + c;
    }
    if (buf[0] == '-')
        *out = -(*out);
    return 0;
}

static int num_from_bin(const char* restrict buf, int64_t* restrict out)
{
    const int start = 2 + (buf[0] == '-');
    if (buf[start] == '0') return 1;
    for (int i = start; i < MAX_INPUT_LENGTH && buf[i] != '\0'; ++i) {
        if (buf[i] != '0' && buf[i] != '1')
            return 1;
        if ((*out) > ((INT64_MAX - (buf[i] - '0')) >> 1))
            return 1;
        *out = ((*out) << 1) + (buf[i] - '0');
    }
    if (buf[0] == '-')
        *out = -(*out);

    return 0;
}

static int num_from_dec(const char* restrict buf, int64_t* restrict out)
{
    const int start = buf[0] == '-';
    if (buf[start] == '0') return 1;
    for (int i = start; i < MAX_INPUT_LENGTH && buf[i] != '\0'; ++i) {
        if (buf[i] < '0' || buf[i] > '9')
            return 1;
        if ((*out) > ((INT64_MAX - (buf[i] - '0')) / 10))
            return 1;
        *out = ((*out) * 10) + (buf[i] - '0');
    }
    if (buf[0] == '-')
        *out = -(*out);

    return 0;
}

static int parse_hex(const int c)
{
    if (c >= '0' && c <= '9')
        return c - '0';

    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;

    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return -1;
}

static int tokenize(char out[MAX_ARGS][MAX_INPUT_LENGTH], const char* restrict const in, int* restrict argc)
{
    *argc = 0;
    int out_idx = 0;
    int leading_spaces = 1;

    for (int i = 0 ;; ++i) {
        if (leading_spaces && in[i] == ' ')
            continue;
        leading_spaces = 0;

        if (*argc >= MAX_ARGS || out_idx >= MAX_INPUT_LENGTH) {
            REPORT_ERROR;
            return 1;
        }

        if (in[i] == ' ' || in[i] == '\0') {
            if (i > 0)
                if (in[i - 1] == ' ')
                    continue;
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

static int read_line(char* restrict buf, const int size)
{
    assert(NULL != buf);
    if (NULL == fgets(buf, size, stdin)) {
        REPORT_ERROR;
        return 1;
    }

    size_t len = strcspn(buf, "\n");
    if (buf[len] == '\n') {
        buf[len] = '\0';
    } else {
        buf[len] = '\0';
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
    }
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


