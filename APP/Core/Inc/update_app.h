#ifndef _UPDATE_APP_H_
#define _UPDATE_APP_H_

typedef struct
{
    uint32_t erase_app_start_addr; // 擦写APP起始地址
    uint32_t download_addr;        // 下次下载地址
    uint32_t erase_app_end_addr;   // 擦写APP结束地址
    uint8_t erase_app_toal_count;  // 擦写APP总次数
    uint8_t erase_app_count;       // 擦写APP当前次数
    uint32_t last_command;         // 上次的命令
    uint32_t next_command;         // 下次的命令
    uint8_t command_result;        // 上次命令结果
    uint8_t erase_result;          // 擦写结果
    uint8_t download_finish;       // 下载完成
    uint8_t single_package_size;   // 36下载一个包大小
    uint8_t package_count;         // 36传包的次数
    uint8_t response_package_size; // 34服务反馈，36单次传输的大小
    uint8_t retry_count;           // 重试次数
} UPDATE_APP_CONF;

typedef uint8_t (*app_function)(void);

typedef struct
{
    uint32_t update_app_command;
    app_function update_app_function;
} UPDATE_APP_FUNCTION;

void update_app_entry(uint8_t *data, uint16_t dataLen);
void update_app_start();
void calcutate_app_size(void);
void erase_app_start();
void check_erase_app_result(void);
void pre_download_app();
void download_app();
void start_install_app();
void download_app_crc_check();
void finish_download_app();
void int_app_conf();
void download_finish_CRC_Validate();
#endif