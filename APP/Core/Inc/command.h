#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "stdio.h"
#include "typedef_conf.h"

void exec_command(char *command, unsigned char str_len);
uint8_t check_flash_address(uint32_t flashAddress);
// 擦写flash
uint8_t flash_erase(uint32_t flash_erase_address);
void flash_lock(uint8_t lock_state);
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


typedef struct
{
    volatile uint32_t ACR;
    volatile uint32_t KEYR;
    volatile uint32_t OPTKEYR;
    volatile uint32_t SR;
    volatile uint32_t CR;
    volatile uint32_t AR;
    volatile uint32_t RESERVED;
    volatile uint32_t OBR;
    volatile uint32_t WRPR;
} FLASH_TypeDef;

#endif