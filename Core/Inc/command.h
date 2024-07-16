#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "stdio.h"

void exec_command(char *command, unsigned char str_len);
void help(unsigned char argc, char *argv[]);

typedef void (*shell_func)(unsigned char argc, char *argv[]);

typedef struct
{
    char *name;
    shell_func subFunction;
} subfunc;

typedef struct
{
    char *name;
    shell_func function;
    subfunc *subFuns[];
} shell_function;

#endif