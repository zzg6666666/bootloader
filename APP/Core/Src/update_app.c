#include "typedef_conf.h"
#include "uart_printf.h"
#include "update_app.h"
#include "stdio.h" //NULL
#include "string.h"
#include "uds.h"
#include "crc.h"

UPDATE_APP_CONF update_app_conf;

// 上次命令
UPDATE_APP_FUNCTION update_app_functions[] = {
    {0x3101ff00, (app_function)erase_app_start},
    {0x3103ff00, (app_function)check_erase_app_result},
    {0x31010203, (app_function)start_install_app},
    {0x31010202, (app_function)download_app_crc_check},
    {0x31010205, (app_function)download_finish_CRC_Validate},
    {0x34000000, (app_function)pre_download_app},
    {0x36000000, (app_function)download_app},
    {0x37000000, (app_function)finish_download_app},

};

void init_app_conf()
{
    update_app_conf.erase_app_count = 0,
    update_app_conf.erase_app_start_addr = 0x8002800UL,
    update_app_conf.erase_app_end_addr = 0x00UL,
    update_app_conf.download_addr = 0x8002800UL,
    update_app_conf.erase_app_toal_count = 0,
    update_app_conf.last_command = 0,
    update_app_conf.next_command = 0,
    update_app_conf.command_result = false,
    update_app_conf.single_package_size = 0X80,
    update_app_conf.erase_result = 2; // waitchcek
    update_app_conf.download_finish = false;
}
void update_app_start()
{
    uart_printf(info_log, "start update app, get app size ", NULL, 0);
    init_app_conf();
    calcutate_app_size();
}

static void execute_update_APP_fucntion(uint32_t command)
{
    uint8_t find_function = false;
    for (int i = 0; i < sizeof(update_app_functions) / sizeof(UPDATE_APP_FUNCTION); i++)
    {
        if (command == (update_app_functions[i].update_app_command))
        {
            update_app_functions[i].update_app_function();
            find_function = true;
            break;
        }
    }
    if (command == 0x66666666)
    {
        uart_printf(error_log, "\r\n\r\ndownload app suceess", NULL, 0);
    }
    else if (find_function == false)
    {
        uint8_t data_to_uint8_t[4] = {0};
        convert_uint32_t_to_uint8_t(command, data_to_uint8_t);
        uart_printf(error_log, "not found update_app function", data_to_uint8_t, 4);
    }
}

void update_app_entry(uint8_t *data, uint16_t dataLen)
{

    for (int i = 0; i < 72000; i++)
    { // 延时，
    }
    uart_printf(system_log, "update_app_entry ", data, dataLen);

    uint8_t temp_data = (update_app_conf.last_command >> 24);

    // 检查服务是不是正反馈
    if (data[0] != (temp_data + 0x40))
    {
        // 检查是不是30 、3e 80、7f xx 78
        if (data[0] == 0x30 || data[0] == 0x3e || ((data[0] == 0x7f) && (data[2] == 0x78)))
        {
            // wait
        }
        else
        {
            uart_printf(error_log, "retry", &update_app_conf.retry_count, 1);
            update_app_conf.retry_count++;
            if (update_app_conf.retry_count >= 3)
            {

                uart_printf(error_log, "retry  failed ......\r\n", NULL, 0);
                return;
            }
            update_app_conf.command_result = false;
            execute_update_APP_fucntion(update_app_conf.last_command);
        }
    }
    else // 下一步命令
    {
        if (data[0] == 0x74)
        {
            update_app_conf.response_package_size = data[2];
        }
        execute_update_APP_fucntion(update_app_conf.next_command);
        update_app_conf.command_result = true;
    }
}

void calcutate_app_size(void)
{
    uint32_t *APP_start_address = (uint32_t *)0x8002800UL;
    uint32_t APPSize = 0;

    uint8_t date_to_uint8_t[4] = {0};
    // 计算app所占的字节数
    while (*APP_start_address != 0xFFFFFFFF)
    {
        APPSize += 4;
        APP_start_address++;
    }

    convert_uint32_t_to_uint8_t(APPSize, date_to_uint8_t);
    uart_printf(info_log, "the app size is: ", date_to_uint8_t, 4);
    update_app_conf.erase_app_end_addr = 0x8002800UL + APPSize;
    // 计算需要擦写的block数量
    uint8_t bootloader_erase_block_count = APPSize / 1024;

    if (APPSize % 1024 != 0)
    {
        bootloader_erase_block_count += 1;
    }
    uart_printf(info_log, "bootloader need erase block count is: ", &bootloader_erase_block_count, 1);

    update_app_conf.erase_app_toal_count = bootloader_erase_block_count;

    erase_app_start();
}

void erase_app_start(void)
{
    uint8_t data_to_uint8_t[4] = {0};
    uint8_t uds_data[8] = {0x31, 0x01, 0xff, 0x00};
    uint32_t erase_address = 0;

    // 设置上次执行命令
    update_app_conf.last_command = 0x3101ff00;

    // 继续擦写
    update_app_conf.next_command = 0x3101ff00;

    // 上次擦除失败
    if (update_app_conf.command_result == false)
    {
        // 尝试再次擦写
        erase_address = update_app_conf.erase_app_start_addr + update_app_conf.erase_app_count * 1024;

        // retry times
    }
    else // 上次执行擦除成功
    {
        // 已经擦除完所有block 执行下一步 检查擦写结果
        if (update_app_conf.erase_app_count == update_app_conf.erase_app_toal_count)
        {
            check_erase_app_result();
            return;
        }
        else // 继续擦写
        {
            // 擦写的块计数
            update_app_conf.erase_app_count++;
            // 本次擦写地址
            erase_address = update_app_conf.erase_app_start_addr + update_app_conf.erase_app_count * 1024;
        }
    }
    uart_printf(info_log, "erase count ", &update_app_conf.erase_app_count, 1);
    convert_uint32_t_to_uint8_t(erase_address, data_to_uint8_t);
    uart_printf(info_log, "erase address ", data_to_uint8_t, 4);
    // 擦写的地址插入到uds命令
    memcpy(uds_data + 4, data_to_uint8_t, 4);
    // 调用tx 发送到bootloader
    can_service_response(uds_data, 8);
}

void check_erase_app_result(void)
{
    uint8_t uds_data[12] = {0x31, 0x03, 0xff, 0x00};
    // 下次执行检查擦写结果
    update_app_conf.next_command = 0x3103ff00;
    // 检查失败需要交给 check_erase_app_result处理
    update_app_conf.last_command = 0x3103ff00;

    if (update_app_conf.erase_result == 2) // 需要发送bootloader检查请求
    {
        convert_uint32_t_to_uint8_t(update_app_conf.erase_app_start_addr, uds_data + 4);
        convert_uint32_t_to_uint8_t(update_app_conf.erase_app_end_addr, uds_data + 8);
        update_app_conf.erase_result = 1; // 等待bootloader 反馈
        can_service_response(uds_data, 12);
    }
    else if (update_app_conf.erase_result == 1) // 检查vip 反馈结果
    {
        if (update_app_conf.command_result == true)
        {
            // 执行34服务
            uart_printf(info_log, "erase app success\r\n", NULL, 0);
            update_app_conf.command_result = false; // 34服务要用到
            pre_download_app();
        }
        else
        {
            //  do nothing
            uart_printf(info_log, "erase app fail\r\n", NULL, 0);
        }
    }
}

void pre_download_app() // 34服务告诉bootloader下载参数
{

    update_app_conf.last_command = 0x34000000;
    uint8_t uds_data[8] = {0x34, 0x00, 0x14};    // memory size 1字节 memory address 4字节
    update_app_conf.package_count = 0;           // 包计数清零
    if (update_app_conf.command_result == false) // 上次执行fail
    {
        // retry
        convert_uint32_t_to_uint8_t(update_app_conf.download_addr, &uds_data[4]); // 下载地址
    }
    else
    {
        if (update_app_conf.download_finish == true) // 下载完成所有包
        {
            // update_app_conf.download_finish = true; // 标记为下载完成， 进行整体CRC
            uart_printf(info_log, "download app finish,start crc\r\n", NULL, 0);
            download_finish_CRC_Validate();
            return;
        }
        update_app_conf.download_addr += update_app_conf.single_package_size; // 下载地址后移一包大小
        // 检查是不是下载到边界
        if (update_app_conf.download_addr + update_app_conf.single_package_size > update_app_conf.erase_app_end_addr)
        {
            // 设置最后一包的大小的下载大小
            update_app_conf.single_package_size = update_app_conf.erase_app_end_addr - update_app_conf.download_addr;
            // update_app_conf.download_addr = update_app_conf.erase_app_end_addr;
            update_app_conf.download_finish = true; // 标记为下载完成， 进行整体CRC
        }
        convert_uint32_t_to_uint8_t(update_app_conf.download_addr, &uds_data[4]);
    }
    // 发送长度
    uds_data[3] = update_app_conf.single_package_size;
    uart_printf(info_log, "pre download address ", &uds_data[3], 4);
    // 36请求下载
    update_app_conf.next_command = 0x36000000;

    uint32_t rest_size = update_app_conf.erase_app_end_addr - update_app_conf.download_addr;

    uint8_t temp[4];
    convert_uint32_t_to_uint8_t(rest_size, temp);
    uart_printf(system_log, "----------------------download rest size-------------------------: ", temp, 4);
    can_service_response(uds_data, 8);
}

// 36服务
void download_app()
{
    // 36服务失败 交给34?
    update_app_conf.last_command = 0x36000000;
    // update_app_conf.next_command = 0x37000000;
    if (update_app_conf.last_command == false)
    {
        pre_download_app();
    }

    /* 一次发完*/
    // uint8_t uds_data[update_app_conf.single_package_size + 2];
    // uds_data[0] = 0x36;
    // uds_data[1] = 0x01;
    // uint32_t *flash_data = (uint32_t *)update_app_conf.download_addr;

    // for (uint8_t i = 0; i < update_app_conf.single_package_size / 4; i++)
    // {
    //     convert_uint32_t_to_uint8_t(*flash_data, &uds_data[2 + i * 4]);
    //     flash_data++;
    // }
    // uart_printf(info_log, "download data\r\n", NULL, 0);

    // can_service_response(uds_data, update_app_conf.single_package_size + 2);

    /*根据34服务反馈的一包大小进行发送*/
    uint8_t uds_data[update_app_conf.response_package_size + 2];
    uint8_t package_total_count; // 传包总数
    // uint8_t package_count = 0x01; // 包计数

    // 计算36服务传包是整包数
    package_total_count = update_app_conf.single_package_size / update_app_conf.response_package_size;
    uds_data[0] = 0x36;
    uds_data[1] = update_app_conf.package_count + 1;
    // 当前传包是满足34返回的整包
    if (update_app_conf.package_count < package_total_count)
    {
        uint32_t *flash_data = (uint32_t *)(update_app_conf.download_addr + update_app_conf.package_count * update_app_conf.response_package_size); // 复制的数据

        for (uint8_t i = 0; i < update_app_conf.response_package_size / 4; i++)
        {
            convert_uint32_t_to_uint8_t(*flash_data, &uds_data[2 + i * 4]);
            flash_data++;
        }
        update_app_conf.package_count++;

        // 36一包没有发送完毕，下一包继续由36发送
        update_app_conf.next_command = 0x36000000;
        // 发送到bootloader
        can_service_response(uds_data, update_app_conf.response_package_size + 2);
    }
    // 当前包不满足34反馈包大小
    else if (update_app_conf.single_package_size % update_app_conf.response_package_size != 0)
    {
        uint32_t *flash_data = (uint32_t *)(update_app_conf.download_addr + update_app_conf.package_count * update_app_conf.response_package_size); // 复制的数据

        uint8_t rest_data_size = update_app_conf.single_package_size - update_app_conf.response_package_size * package_total_count; // 剩下的数据长度

        for (uint8_t i = 0; i < rest_data_size / 4; i++)
        {
            convert_uint32_t_to_uint8_t(*flash_data, &uds_data[2 + i * 4]);
            flash_data++;
        }
        // 36一包发送完毕，
        update_app_conf.next_command = 0x37000000;
        // 发送到bootloader
        can_service_response(uds_data, rest_data_size + 2);
    }

    // // 一包大小满足34反馈大小
    // for (uint8_t i = 0; i < package_total_count; i++)
    // {
    //     uds_data[0] = 0x36;
    //     uds_data[1] = package_count;
    //     for (uint8_t j = 0; j < update_app_conf.response_package_size / 4; j++)
    //     {
    //         convert_uint32_t_to_uint8_t(*flash_data, &uds_data[2 + j * 4]);
    //         flash_data++;
    //     }
    //     // 发送到bootloader
    //     package_count++;
    //     can_service_response(uds_data, update_app_conf.response_package_size + 2);
    // }
    // // 一包大小小于34反馈大小
    // if (update_app_conf.single_package_size % update_app_conf.response_package_size != 0)
    // {
    //     // package_total_count++;
    //     uds_data[0] = 0x36;
    //     uds_data[1] = package_count;

    //     uint8_t rest_data_size = update_app_conf.single_package_size -
    //                              update_app_conf.response_package_size * package_total_count;

    //     for (size_t i = 0; i < rest_data_size / 4; i++)
    //     {
    //         convert_uint32_t_to_uint8_t(*flash_data, &uds_data[2 + i * 4]);
    //         flash_data++;
    //     }
    //     can_service_response(uds_data, rest_data_size + 2);
    // }
}

// 通知bootloader本次传包完成

void finish_download_app()
{
    update_app_conf.last_command = 0x37000000;
    uint8_t uds_data = 0x37;
    if (update_app_conf.command_result == false)
    {
        uart_printf(error_log, "37 service finish download fail ", NULL, 0);
        // retry

        pre_download_app();
        return;
    }
    update_app_conf.next_command = 0x31010202; // 下一步crc校验
    // 发送到bootloader
    can_service_response(&uds_data, 1);
}

// 通知app 进行crc校验
void download_app_crc_check()
{
    update_app_conf.last_command = 0x31010202;
    if (update_app_conf.command_result == false)
    {
        uart_printf(error_log, "31 service crc check fail ", NULL, 0);
        // retry
        pre_download_app();
        return;
    }

    uint8_t uds_data[8] = {0x31, 0x01, 0x02, 0x02};
    // 对数据进行CRC计算
    uint8_t CRC_package_length = update_app_conf.single_package_size / 4;

    uint32_t CRC_data[CRC_package_length];
    uint32_t *flash_data = (uint32_t *)update_app_conf.download_addr;

    for (uint16_t i = 0; i < CRC_package_length; i++)
    {
        CRC_data[i] = *flash_data;
        flash_data++;
    }

    // 进行CRC校验
    crc_reset();
    convert_uint32_t_to_uint8_t(crc_calculate(CRC_data, CRC_package_length), &uds_data[4]);
    update_app_conf.next_command = 0x31010203; // 安装
    // 发送到bootloader
    can_service_response(uds_data, 8);
}

void download_finish_CRC_Validate()
{
    uint16_t flash_data_length = 0;
    uint32_t CRC_reult = 0;
    uint8_t uds_data[16] = {0x31, 0x01, 0x02, 0x05};

    uint32_t *temp = (uint32_t *)update_app_conf.erase_app_start_addr;
    update_app_conf.last_command = 0x31010205;
    flash_data_length = update_app_conf.erase_app_end_addr - update_app_conf.erase_app_start_addr;
    crc_reset();
    CRC_reult = crc_calculate(temp, flash_data_length / 4);
    convert_uint32_t_to_uint8_t(CRC_reult, &uds_data[4]);                            // crc结果
    convert_uint32_t_to_uint8_t(update_app_conf.erase_app_start_addr, &uds_data[8]); // app的开始地址
    convert_uint32_t_to_uint8_t(update_app_conf.erase_app_end_addr, &uds_data[12]);  // app的结束地址
    update_app_conf.next_command = 0x66666666;
    can_service_response(uds_data, 16);
}

// 通知bootloader安装本次包
void start_install_app()
{
    // 34服务执行失败，交给34?
    if (update_app_conf.command_result == false)
    {
        uart_printf(error_log, "bootloader install app fail ", NULL, 0);
        pre_download_app();
        return;
    }

    update_app_conf.last_command = 0x31010203;
    uint8_t uds_data[8] = {0x31, 0x01, 0x02, 0x03};
    update_app_conf.next_command = 0x34000000;
    can_service_response(uds_data, 4);
}