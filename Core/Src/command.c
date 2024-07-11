#include "command.h"
#include "uart_printf.h"
#include "string.h"
#include "shell.h"
void exec_command(char *command, uint8_t str_len);
void help(uint8_t argc, char *argv[]);
extern void jump_to_app();

void help(uint8_t argc, char *argv[])
{
    uart_printf("this is help\r\n", NULL, 0);
}

shell_function help_function = {
    "help",
    "short help",
    "long help",
    help};

shell_function jump_to_app_function = {
    "jump_app",
    "short help",
    "long help",
    (shell_func)jump_to_app};

static shell_function *shell_functions[] = {
    &help_function, &jump_to_app_function, NULL};

void exec_command(char *command, uint8_t str_len)
{
    uint8_t i = 0;
    uint8_t argc = 0;
    // 支持的命令条数 为3 条，过长的参数交给cmd命令自行处理
    char *argv[3] = {NULL, NULL, NULL};
    while (shell_functions[i] != NULL)
    {

        // 计算 输入字符前面的空格数量
        uint8_t j = 0;
        while ((command[j] == ' ' || command[j] == '\0') & (j < str_len))
        {
            j++;
        }

        // 是不是全都是空格 或者'\0'
        if (j == str_len)
        {
            return;
        }

        // 尝试找到命令
        if (memcmp(&command[j], shell_functions[i]->name, strlen(shell_functions[i]->name)) == 0)
        {

            // 分割参数
            for (j += strlen(shell_functions[i]->name); j < str_len; j++)
            {
                if (command[j] == ' ')
                {
                    // 将空格替换成结束
                    command[j] = '\0';
                    // 指向空格的后面一个
                    argv[argc] = &command[j + 1];
                    argc++;
                }
                // 已经分割出三条命令
                if (argc == 3)
                {
                    break;
                }
            }

            // 执行命令
            shell_functions[i]->function(argc, argv);

            break;
        }

        i++;
    }
    // 是否找到了命令
    if (i == (sizeof(shell_functions) / sizeof(shell_functions[0]) - 1))
    {
        uart_printf("command not found :", NULL, 0);
        uart_printf(command, NULL, 0);
        uart_printf("\r\n", NULL, 0);
    }
}
