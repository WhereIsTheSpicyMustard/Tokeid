#include "tokeid.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REPORT_ERROR(X) fprintf(stderr, "Tokeid Error -> %s | %s | %d\n", X, __FILE__, __LINE__)

typedef struct TokeidCommand {
    TokeidFunc func;
    char* keyword;
    char* help_text;
} TokeidCommand;


static TokeidCommand* commands = NULL;
static size_t commands_size = 0;
static size_t commands_capacity = 0;

static int read_line(char* buf, const int size);

int tokeid_init(const size_t new_capacity)
{
    commands = malloc(new_capacity * sizeof(*commands));
    if (commands == NULL)
        return 1;

    commands_capacity = new_capacity;

    return 0;
}

void tokeid_cleanup(void)
{
    free(commands);
}


int tokeid_get_input(const char* const prompt)
{
    char buf[MAX_INPUT_LENGTH];

    if (prompt != NULL)
        printf("%s", prompt);

    if (read_line(buf, MAX_INPUT_LENGTH))
        return 1;

    // tokenize


    // TODO: change to hashmap in future update
    for (int i = 0; i < commands_size; ++i) {
        if (strncmp(buf, commands[i].keyword, strlen(commands[i].keyword)))
            continue;

        commands[i].func();
        break;
    }

    return 0;
}

static int read_line(char* buf, const int size)
{
    if (NULL == fgets(buf, size, stdin)) return 1;
    buf[strcspn(buf, "\n")] = '\0';
    return 0;
}



