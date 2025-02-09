#include "command.h"
#include "uart_printf.h"
#include "string.h"
#include "shell.h"
#include "uds.h"
#include "typedef_conf.h"
// 执行命令
void exec_command(char *command, uint8_t str_len);
static void help(uint8_t argc, char *argv[]);
static void change_space_to_end(char *str, uint8_t strlen);

static void memory_read_write(uint8_t argc, char *argv[]);
static void memory_flash_read(uint8_t argc, char *argv[]);
static void memory_flash_write(uint8_t argc, char *argv[]);
extern void jump_to_app();

// static void flash_write(uint32_t address, uint32_t *data, uint16_t dataLen);

static void flash_write_half_world(uint32_t address, uint16_t data);
static void uds_port(uint8_t argc, char *argv[]);
// static void update_app(void);
static void log_set_cancel(uint8_t argc, char *argv[]);
static void log_set_commend(uint8_t argc, char *argv[]);
static void log_cancel_commend(uint8_t argc, char *argv[]);

FLASH_TypeDef *FLASH_BASS = ((FLASH_TypeDef *)0x40022000UL);

// subfunc *memory_subfunctions[] = {&(subfunc){"read", memory_flash_read}, &(subfunc){"write", memory_flash_write}, NULL};

static shell_function *shell_functions[] = {
    &(shell_function){"help", help, {NULL}},
    &(shell_function){"jump_app", (shell_func)jump_to_app, {NULL}},
    &(shell_function){"memory", memory_read_write, {&(subfunc){"read", memory_flash_read}, &(subfunc){"write", memory_flash_write}, NULL}},
    &(shell_function){"uds", uds_port, {NULL}},
    // &(shell_function){"update_app", (shell_func)update_app, {NULL}},
    &(shell_function){"log", log_set_cancel, {&(subfunc){"set", log_set_commend}, &(subfunc){"cancel", log_cancel_commend}, NULL}},
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

    while (shell_functions[i] != NULL) // 遍历支持的shell_function
    {

        // 尝试找到命令
        if (memcmp(&command[j], shell_functions[i]->name, strlen(shell_functions[i]->name)) == 0)
        {

            // 分割参数
            for (j += strlen(shell_functions[i]->name); j < vaild_strlen - 1; j++)
            {

                // 分割参数
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
                        // 执行子命令 给子命令传入，参数数量-1，和参数[1]
                        shell_functions[i]->subFuns[j]->subFunction((argc - 1), &argv[1]);
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
        uart_printf(error_log, "command not found :", NULL, 0);
        uart_printf(error_log, command, NULL, 0);
        uart_printf(error_log, "\r\n", NULL, 0);
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

static void help(uint8_t argc, char *argv[])
{
    uint8_t i = 0;
    uart_printf(system_log, "shell suppport commamnds:\r\n", NULL, 0);
    while (shell_functions[i] != NULL)
    {

        uart_printf(system_log, shell_functions[i]->name, NULL, 0);
        uart_printf(system_log, "    ", NULL, 0);
        uint8_t j = 0;

        while (shell_functions[i]->subFuns[j] != NULL)
        {
            if (j == 0)
            {
                uart_printf(system_log, "sub command :", NULL, 0);
            }
            uart_printf(system_log, shell_functions[i]->subFuns[j]->name, NULL, 0);
            j++;
            uart_printf(system_log, "    ", NULL, 0);
            /* code */
        }
        i++;
        uart_printf(system_log, "\r\n", NULL, 0);
    }
}

static void memory_read_write(uint8_t argc, char *argv[])
{
    uart_printf(system_log, "use \"memory read FFFFFFFF\" to read memory, support 2 address\r\n", NULL, 0);
}

// 读内存和flash
static void memory_flash_read(uint8_t argc, char *argv[])
{

    uint8_t data[4] = {};

    // 迭代参数
    for (uint8_t i = 0; i < argc; i++)
    {
        // 设置第8位为0；防止溢出
        argv[i][8] = '\0';
        STR_TO_HEX(argv[i], data);
        uart_printf(system_log, "addres : ", &data[0], 4);

        // address 必须为0
        uint32_t temp, address = 0;

        // 将传回的16进制的数据转换成32位
        for (int j = 0; j < 4; j++)
        {
            temp = data[j];
            address |= temp << ((4 - j - 1) * 8);
        }
        // 检查地址合法性
        if (((address <= 0x20005000) && (address >= 0x20000000)) ||
            ((address <= 0x0000FFFF) && (address >= 0x00000000)) ||
            ((address <= 0x0800FFFF) && (address >= 0x08000000)))
        {
            // 读取出数据
            uint32_t memory_data = *(uint32_t *)address;
            // 将数据转成4个8位
            for (int j = 0; j < 4; j++)
            {
                data[j] = (uint8_t)(memory_data >> (8 * (3 - j)));
            }
            uart_printf(system_log, "data : ", &data[0], 4);
        }
        else
        {
            uart_printf(system_log, "the address is not in flash or ram range ,try to use \"register read\" \r\n", NULL, 0);
        }
    }
}

static void memory_flash_write(uint8_t argc, char *argv[])
{

    if (argc != 3)
    {
        uart_printf(system_log, "write input is illegal\r\n", NULL, 0);
    }
    else
    {
        uint8_t return_value = true;
        uint8_t data[4];
        uint32_t address = 0, write_data = 0;
        for (uint8_t i = 1; i < argc; i++)
        {
            // 设置第8位为0；防止溢出
            argv[i][8] = '\0';
            STR_TO_HEX(argv[i], data);
            uint32_t temp = 0;
            // 将传回的16进制的数据转换成32位
            for (int j = 0; j < 4; j++)
            {
                temp = data[j];
                if (i == 1)
                {
                    address |= temp << ((4 - j - 1) * 8);
                }
                else
                {
                    write_data |= temp << ((4 - j - 1) * 8);
                }
            }
        }
        // 强制进行4字节对齐
        address &= ~((uint32_t)0x03);

        // 检查写入是memory 还是 flash
        return_value = check_flash_address(address);

        if (return_value != true)
        {
            uart_printf(system_log, "write address is illegal \r\n", NULL, 0);
            return;
        }
        else
        {
            // uart_printf("the write address is ", &address, 4);

            // 读取出数据

            // 解锁flash
            flash_lock(1);

            // 擦除
            return_value = flash_erase(address);
            if (return_value != true)
            {
                uart_printf(system_log, "flash_erase fail \r\n", NULL, 0);

                // 上锁flash
                flash_lock(0);
                return;
            }

            // 写数据

            uint32_t testdata[64] = {};
            for (int dd = 0; dd < 64; dd++)
            {
                // write_data += dd;
                testdata[dd] = 0x12345600 + dd;
                // flash_write(address, &write_data, 1);
            }

            flash_write(address, testdata, 64);
        }
    }
}

// 连续多次写入数据
void flash_write(uint32_t address, uint32_t *data, uint16_t dataLen)
{

    uint8_t waitCount = 255;
    for (uint16_t index = 0; index < dataLen * 2; index++)
    {
        // 等待flash不在忙碌状态
        while (((FLASH_BASS->SR) & (1UL)) != 0)
        {
            waitCount--;
        }
        if (waitCount == 0)
        {
            // do something
        }

        // 烧写数据的后16位
        if ((index % 2) == 0)
        {
            flash_write_half_world(address + 0x02 * index, (uint16_t)(data[index / 2]));
        }
        // 烧写数据的前16位
        else
        {
            flash_write_half_world(address + 0x02 * index, (uint16_t)((data[index / 2]) >> 16));
        }

        // 检查错误码

        if (((FLASH_BASS->CR & 1UL << 0x02) == (1UL << 0x02)) ||
            ((FLASH_BASS->CR & 1UL << 0x04) == (1UL << 0x04)))
        {
            // dosomthing
        }
    }
}

static void flash_write_half_world(uint32_t address, uint16_t data)
{
    // 设置为编程模式
    FLASH_BASS->CR |= 0x01UL;
    // flash 不在忙碌状态
    uint8_t waitCount = 255;
    while (((FLASH_BASS->SR) & (1UL)) != 0)
    {
        waitCount--;
    }
    // 写入数据
    *(volatile uint16_t *)address = data;
    // uart_printf("write address is ", &address, 4);
    // 关闭编程模式
    FLASH_BASS->CR &= ~0x01UL;
}

void flash_lock(uint8_t lock_state)
{
    // lock
    if (lock_state == 0)
    {
        // lock
        FLASH_BASS->CR |= 0x01UL << 7;
    }
    // unlock
    else if (lock_state == 1)
    {

        FLASH_BASS->KEYR = 0x45670123UL;

        FLASH_BASS->KEYR = 0xCDEF89ABUL;

        // 检查锁定
        while (((FLASH_BASS->CR) & (1U << 7)) != 0)
        {
        }
    }
}

// 擦写flash
uint8_t flash_erase(uint32_t flash_erase_address)
{

    // flash的真实地址
    flash_erase_address &= 0x0000FFFF;

    // 页地址
    flash_erase_address = flash_erase_address - (flash_erase_address % 1024);
    flash_erase_address |= 0x08000000;

    uint8_t temp[4];

    temp[0] = flash_erase_address >> 24;
    temp[1] = flash_erase_address >> 16;
    temp[2] = flash_erase_address >> 8;
    temp[3] = flash_erase_address >> 0;

    uart_printf(system_log, "the page start address is ", temp, 4);

    // 设置擦写页
    {
        // 选择页擦除
        FLASH_BASS->CR |= 1UL << 1;
        // 设置擦除寄存器 page地址
        FLASH_BASS->AR = flash_erase_address;
        // 开始页擦除
        FLASH_BASS->CR |= 1UL << 6;
    }

    // 完成擦除页
    {
        uint8_t temp = 255;
        // 检查擦除结束
        while (((FLASH_BASS->SR) & (1UL << 5)) != 1UL << 5)
        {
            temp--;
            if (temp == 0)
            {
                // 擦除失败
                return false;
            }
        }

        // 结束擦除
        FLASH_BASS->CR &= ~(1UL << 1);

        return true;
    }
}

// 检查flash地址合法
uint8_t check_flash_address(uint32_t flashAddress)
{

    // 系统映射地址
    if ((flashAddress >= 0x00000000) && (flashAddress <= 0x0000FFFF))
    {
        return true;
    }
    // 真实地址
    else if ((flashAddress >= 0x08000000) && (flashAddress <= 0x0800FFFF))
    {
        return true;
    }
    else
    {
        return false;
    }
}

// 让uds 支持shell
static void uds_port(uint8_t argc, char *argv[])
{

    // 解析参数 ,shell 只解析三条
    uint8_t data[8] = {};

    for (uint8_t index = 0; index < 8; index++)
    {
        STR_TO_HEX(&argv[0][index * 3], &data[index]);
    }
    can_service_entry(data);
}

static void log_set_cancel(uint8_t argc, char *argv[])
{
    uart_printf(system_log, "use \"log set/cancel xx\" to set log level\r\n", NULL, 0);
}

static void log_set_commend(uint8_t argc, char *argv[])
{
    uint8_t data;

    // 防止溢出
    argv[0][3] = '\0';
    STR_TO_HEX(argv[0], &data);
    set_log_level(data);
}
static void log_cancel_commend(uint8_t argc, char *argv[])
{
    uint8_t data;
    argv[0][3] = '\0';
    STR_TO_HEX(argv[0], &data);
    cancel_log_level(data);
}

// static void update_app(void)
// {
//     uart_printf(info_log, "bootloader dont support\r\n", NULL, 0);
// }