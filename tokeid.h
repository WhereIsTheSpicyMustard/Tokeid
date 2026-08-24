#ifndef TOKEID_H
#define TOKEID_H

#include <stddef.h>

#define MAX_INPUT_LENGTH 32
#define MAX_ARGS 8


typedef int (*TokeidFunc)(int argc, char argv[MAX_ARGS][MAX_INPUT_LENGTH]);
typedef struct TokeidCommand TokeidCommand;

int  tokeid_init(const size_t new_capacity); // Allocates memory for new_capacity number of commands.  Returns 1 on failure, 0 on success.
void tokeid_cleanup(void); // Frees all memory allocated from init
int  tokeid_should_close(void); // returns 1 when the program should close, 0 otherwise
int tokeid_close(int argc, char argv[MAX_ARGS][MAX_INPUT_LENGTH]); // Causes tokeid_should_close to return 1.  Always returns 0.
// int  tokeid_commands_capacity_resize(const size_t new_capacity);

int  tokeid_command_create(char* keyword, TokeidFunc func);
int  tokeid_command_create_ex(char* keyword, TokeidFunc func, char* help_text);

int  tokeid_prompt(const char out[], const size_t size, const char* const prompt);
int  tokeid_get_input(const char* const prompt);


#endif
