#include "shell.h"
#include "uart_printf.h"
#include "stdlib.h"
#include "string.h"
#include "command.h"
static void shell_put_history(shell_control_block *shell);
static void shell_handle_history(shell_control_block *shell);
shell_control_block shell;
void shell_init()
{

    shell.line_curpos = 0;
    shell.line_position = 0;
    shell.line_position = 0;
    shell.current_history = 0;
    shell.history_count = 0;
    shell.state = WAIT_NORMAL;
    memset(shell.line, 0, sizeof(shell.line));
    memset(shell.history_cmd, 0, sizeof(uint8_t) * SHELL_CMD_SIZE * SHELL_HISTORY_LENS);
    uart_printf(system_log, "shell start\r\n", NULL, 0);

    uart_printf(system_log, shell_prompt, NULL, 0);
}

void shell_enter(uint8_t data)
{
    uint8_t outData[2] = {'\0', '\0'};
    // uint8_t data = 0;

    // uart2_communicate_rx();
    // uart_get(&data);

    /* handle control key
     * up key  : 0x1b 0x5b 0x41
     * down key: 0x1b 0x5b 0x42
     * right key:0x1b 0x5b 0x43
     * left key: 0x1b 0x5b 0x44
     */

    if (data == 0x1b)
    {
        shell.state = WAIT_SPEC_KEY;
        return;
    }
    else if (shell.state == WAIT_SPEC_KEY)
    {
        if (data == 0x5b)
        {
            shell.state = WAIT_FUNC_KEY;
            return;
        }
        shell.state = WAIT_NORMAL;
    }
    else if (shell.state == WAIT_FUNC_KEY)
    {
        shell.state = WAIT_NORMAL;

        if (data == 0x41) // up
        {
            if (shell.current_history > 0)
            {
                shell.current_history--;
            }
            else // 没有历史
            {
                shell.current_history = 0;
                return;
            }

            // 复制到line中
            memcpy(shell.line, shell.history_cmd[shell.current_history], SHELL_CMD_SIZE);
            // 更新shell line 参数

            shell.line_curpos = shell.line_position = strlen(shell.history_cmd[shell.current_history]);
            shell_handle_history(&shell);

            return;
        }
        else if (data == 0x42) // down
        {
            // 更新current
            if (shell.current_history < shell.history_count - 1)
            {
                shell.current_history++;
            }
            else
            {
                return;
            }

            // 复制到line
            memcpy(shell.line, shell.history_cmd[shell.current_history], SHELL_CMD_SIZE);
            shell.line_curpos = shell.line_position = strlen(shell.line);
            // 输出新的
            shell_handle_history(&shell);
            return;
        }
        else if (data == 0x43) // right
        {
            // uart_printf("right key :", &data, 1);
            if (shell.line_curpos < shell.line_position)
            {
                uint8_t temp = (uint8_t)shell.line[shell.line_curpos];
                uart_printf(system_log, &temp, NULL, 0);
                shell.line_curpos++;
            }
        }
        else if (data == 0x44) // left
        {
            if (shell.line_curpos > 0)
            {
                uint8_t out_data[2] = {'\b', '\0'};
                // data = '\b';

                uart_printf(system_log, &out_data, NULL, 0);
                shell.line_curpos--;
            }
        }
        return;
    }

    // 换行符号 "/r"(13) "/n"(10)
    if (data == '\r' || data == '\n')
    {

        uart_printf(system_log, "\r\n", NULL, 0);

        // 放入命令到历史命令
        shell_put_history(&shell);

        // 执行命令 会更改主动替换命令中的空格为'/0',shell_put_history是根据'/0'判断结束没有
        exec_command(shell.line, shell.line_position);
        // 执行命令完成
        uart_printf(system_log, shell_prompt, NULL, 0);

        shell.line_curpos = 0;
        shell.line_position = 0;
        // 重置当前line为'\0'
        memset(&shell.line[0], '\0', sizeof(shell.line));
        return;
    }

    // 删除符号 delete(0x7f)和 backspace(0x08)
    if (data == 0x7f || data == 0x08)
    {

        if (shell.line_curpos > 0)
        {
            shell.line_curpos--;
            shell.line_position--;
            // 删除末尾
            if (shell.line_curpos == shell.line_position)
            {
                uart_printf(system_log, "\b \b", NULL, 0);
                shell.line[shell.line_position] = 0;
            }
            else // 删除中间
            {
                // 将需要删除的后一位以后的数据，复制到需要删除的那一位以后
                memcpy(&shell.line[shell.line_curpos],
                       &shell.line[shell.line_curpos + 1],
                       (shell.line_position - shell.line_curpos));
                // 删除了一位，最后一位设置为0
                shell.line[shell.line_position] = 0;

                // 退格覆盖需要删除那一位
                uint8_t temp = '\b';
                uart_printf(system_log, &temp, NULL, 0);
                // 输出新的字符串
                uart_printf(system_log, &shell.line[shell.line_curpos], NULL, 0);
                // 覆盖末尾的那一个字符
                temp = ' ';
                uart_printf(system_log, &temp, NULL, 0);
                // 退格12
                temp = '\b';
                for (int i = shell.line_curpos; i <= shell.line_position; i++)
                {
                    uart_printf(system_log, &temp, NULL, 0);
                }
            }

            return;
        }
        else
        {
            return;
        }
    }

    // 普通key：
    if (shell.line_curpos < shell.line_position)
    {
        char temp[SHELL_CMD_SIZE];
        // 更新buff
        memcpy(temp, shell.line, shell.line_position);
        memcpy(&shell.line[shell.line_curpos + 1], &temp[shell.line_curpos], shell.line_position - shell.line_curpos);
        shell.line[shell.line_curpos] = data;

        // 打印从line_curpos到line_position的字符
        uart_printf(system_log, &shell.line[shell.line_curpos], NULL, 0);
        // 左移符号
        uint8_t temp_data[shell.line_position - shell.line_curpos + 1];
        for (int i = shell.line_curpos; i < shell.line_position; i++)
        {
            temp_data[i] = '\b';
        }
        memset(&temp_data, '\b', shell.line_position - shell.line_curpos);
        temp_data[shell.line_position - shell.line_curpos] = '\0';
        uart_printf(system_log, &temp_data, NULL, 0);
    }
    else // line_curpos = line_position
    {
        shell.line[shell.line_curpos] = data;
        outData[0] = data;
        uart_printf(system_log, &outData, NULL, 0);
    }

    // 更新index
    shell.line_position++;
    shell.line_curpos++;
    if (shell.line_position >= SHELL_CMD_SIZE) // 输入的buff满了，从0开始记录
    {
        shell.line_curpos = 0;
        shell.line_position = 0; // 在line_curpos 小于 line_position 的情况下 有bug
        memset(&shell, '\0', SHELL_CMD_SIZE);
    }
}

// 在换号符号行处理调用
static void shell_put_history(shell_control_block *shell)
{

    // 确保当前的cmd不为空
    if (shell->line_position != 0)
    {

        // shell未满
        if (shell->history_count < SHELL_HISTORY_LENS)
        {
            // 和上一条不一样或者这是第一条
            if (shell->history_count == 0 || memcmp(shell->line, shell->history_cmd[shell->history_count - 1], SHELL_CMD_SIZE))
            {
                // 当前历史
                // 当前使用的清0
                memset(shell->history_cmd[shell->history_count], 0, SHELL_CMD_SIZE);
                memcpy(shell->history_cmd[shell->history_count], shell->line, shell->line_position);
                // 记录的命令行数
                shell->history_count++;
            }
        }
        else // shell已满
        {
            // 检查和上一行是否一致
            if (memcmp(shell->line, &shell->history_cmd[SHELL_HISTORY_LENS - 1], SHELL_CMD_SIZE))
            {

                // 更新cmd
                for (int i = 0; i < SHELL_HISTORY_LENS - 1; i++)
                {
                    memcpy(&shell->history_cmd[i], &shell->history_cmd[i + 1], SHELL_CMD_SIZE);
                }

                // 最后一行更新数据
                memset(shell->history_cmd[SHELL_HISTORY_LENS - 1], 0, SHELL_CMD_SIZE);
                memcpy(&shell->history_cmd[SHELL_HISTORY_LENS - 1], shell->line, SHELL_CMD_SIZE);
            }
        }
    }
    // 更新current ，换行后可以继续查看cmd历史
    shell->current_history = shell->history_count;
}

// 在up down 中调用，用于更新当前输出的内容
static void shell_handle_history(shell_control_block *shell)
{
    uint8_t tempData[SHELL_CMD_SIZE + 1];
    tempData[0] = '\r';
    tempData[1] = '\0';
    // 回到行首
    uart_printf(system_log, tempData, NULL, 0);
    memset(tempData, ' ', SHELL_CMD_SIZE);
    // 清行
    uart_printf(system_log, tempData, NULL, 0);
    // 回到行首
    tempData[0] = '\r';
    tempData[1] = '\0';
    uart_printf(system_log, tempData, NULL, 0);
    uart_printf(system_log, &shell_prompt, NULL, 0);
    uart_printf(system_log, &shell->line, NULL, 0);
}
