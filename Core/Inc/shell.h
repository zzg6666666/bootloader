#ifndef _SHELL_H_
#define _SHELL_H_

#define SHELL_CMD_SIZE 80
#define SHELL_HISTORY_LENS 5
#define shell_prompt "shell >"
typedef unsigned char uint8_t;

typedef struct
{
    char line[SHELL_CMD_SIZE + 1];
    // 输入了的命令的长度
    uint8_t line_position;
    // 当前命令在那个位置
    uint8_t line_curpos;

    // 等待特殊按键的状态
    uint8_t state;
    // 当前是在历史的那一条命令中
    uint8_t current_history;
    // 已经输入的cmd长度
    uint8_t history_count;

    char history_cmd[SHELL_HISTORY_LENS][SHELL_CMD_SIZE];
} shell_control_block;

enum SHELL_STATE
{
    WAIT_NORMAL = 0,
    WAIT_SPEC_KEY,
    WAIT_FUNC_KEY
};

void shell(void);

#endif