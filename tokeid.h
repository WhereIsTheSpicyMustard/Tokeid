#ifndef TOKEID_H
#define TOKEID_H

#include <stddef.h>

#define MAX_INPUT_LENGTH 32


typedef int (*TokeidFunc)(int argc, char* argv[]);
typedef struct TokeidCommand TokeidCommand;

int  tokeid_init(const size_t new_capacity); // Allocates memory for new_capacity number of commands.  Returns 1 on failure, 0 on success.
void tokeid_cleanup(void); // Frees all memory allocated from init
// int  tokeid_should_close(void); // returns 1 when the program should close, 0 otherwise
// int  tokeid_commands_capacity_resize(const size_t new_capacity);

int  tokeid_command_create(const char* const keyword, const TokeidFunc func);
int  tokeid_command_create_ex(const char* const keyword, TokeidFunc func, const char* const help_text);

int  tokeid_prompt(const char out[], const size_t size, const char* const prompt);
int  tokeid_get_input(const char* const prompt);


#endif
