#include "command.h"
#include "uart_printf.h"
#include "string.h"
#include "shell.h"
// 执行命令
void exec_command(char *command, uint8_t str_len);
void help(uint8_t argc, char *argv[]);
static void change_space_to_end(char *str, uint8_t strlen);

static void memory_read_write(uint8_t argc, char *argv[]);
static void memory_flash_read(uint8_t argc, char *argv[]);
static void memory_flash_write(uint8_t argc, char *argv[]);
extern void jump_to_app();

void help(uint8_t argc, char *argv[])
{
    uart_printf("this is help\r\n", NULL, 0);
}

// // 内存 flash 读写功能
// shell_function memory_function = {
//     "memory",
//     "memory short help",
//     "memory long help",
//     memory_flash_read,
//     NULL};

subfunc *memory_subfunctions[] = {&(subfunc){"read", memory_flash_read}, &(subfunc){"write", memory_flash_write}, NULL};

static shell_function *shell_functions[] = {
    &(shell_function){"help", help, {NULL}},
    &(shell_function){"jump_app", (shell_func)jump_to_app, {NULL}},
    &(shell_function){"memory", memory_read_write, {&(subfunc){"read", memory_flash_read}, &(subfunc){"write", memory_flash_write}, NULL}},
    NULL};

/**
 * @brief 分割传入的字符串为命令和命令参数，并尝试找到对应的命令去执行
 * @param command 传入的命令
 * @param str_len 传入的字符长度
 */
void exec_command(char *command, uint8_t str_len)
{
    // 迭代父命令
    uint8_t i = 0;
    uint8_t vaild_strlen = 0;
    uint8_t find_sub_function = 0;
    uint8_t argc = 0;
    // 支持的命令条数 为3 条，过长的参数交给cmd命令自行处理
    char *argv[3] = {NULL, NULL, NULL};
    while (shell_functions[i] != NULL) // 遍历支持的shell_function
    {

        // 将空格全部替换成'\0'
        change_space_to_end(command, str_len);
        // 计算 输入字符前面的'\0'数量
        uint8_t j = 0; // 迭代字符串
        vaild_strlen = str_len;
        while ((command[j] == '\0') & (j < str_len))
        {
            j++;
            // 有效字符长度减少
            vaild_strlen--;
        }

        // 有效字符串为0
        if (vaild_strlen == 0)
        {
            return;
        }

        // 尝试找到命令
        if (memcmp(&command[j], shell_functions[i]->name, strlen(shell_functions[i]->name)) == 0)
        {

            // 分割参数
            for (j += strlen(shell_functions[i]->name); j < vaild_strlen - 1; j++)
            {

                if ((command[j] == '\0') && (command[j + 1] != '\0'))
                {
                    // 指向空格的后面一个
                    argv[argc] = &command[j + 1];
                    argc++;
                    // 已经分割出三条命令
                    if (argc == 3)
                    {
                        break;
                    }
                }
            }

            // 分割出的命令条数不为0 尝试找到子命令
            if (argc != 0)
            {
                j = 0;
                while (shell_functions[i]->subFuns[j] != NULL)
                {
                    // 找到子命令
                    if (memcmp(shell_functions[i]->subFuns[j]->name, argv[0], strlen(shell_functions[i]->subFuns[j]->name)) == 0)
                    {
                        // 执行子命令
                        shell_functions[i]->subFuns[j]->subFunction(argc, argv);
                        find_sub_function = 1;
                        break;
                    }
                    j++;
                }
            }
            // 未找到子命令，或者没有分割出子命令

            if (find_sub_function == 0)
            {
                shell_functions[i]->function(argc, argv);
            }

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

static void change_space_to_end(char *str, uint8_t strlen)
{

    for (uint8_t i = 0; i < strlen; i++)
    {
        if (*str == ' ')
        {
            *str = '\0';
        }
        str++;
    }
}


static void memory_read_write(uint8_t argc, char *argv[])
{
    uart_printf("use \"memory read FFFFFFFF\" to read memory, support 2 address\r\n", NULL, 0);
}

// 读内存和flash
static void memory_flash_read(uint8_t argc, char *argv[])
{

    uint8_t data[4];

    //迭代参数
    for (uint8_t i = 1; i < argc; i++)
    {
        //设置第8位为0；防止溢出
        argv[i][8] = '\0';
        STR_TO_HEX(argv[i], data);
        uart_printf("addres : ", &data[0], 4);

        // address 必须为0
        uint32_t temp, address = 0;

        //将传回的16进制的数据转换成32位
        for (int j = 0; j < 4; j++)
        {
            temp = data[j];
            address |= temp << ((4 - j - 1) * 8);
        }
        //读取出数据
        uint32_t memory_data = *(uint32_t *)address;
        //将数据转成4个8位
        for (int j = 0; j < 4; j++)
        {
            data[j] = (uint8_t)(memory_data >> (8 * (3 - j)));
        }
        uart_printf("data : ", &data[0], 4);
    }
}
static void memory_flash_write(uint8_t argc, char *argv[])
{
    uart_printf("memory write\r\n", NULL, 0);
}