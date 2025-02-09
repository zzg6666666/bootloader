#ifndef UART_PRINTF_H
#define UART_PRINTF_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#define MIN(a, b) ((a < b) ? a : b)

// 使用中断进行输出
#define uart_IT 1

// 使用RTOS
#define use_RTOS 0

#if use_RTOS
#include "FreeRTOS.h"
#endif

#if uart_IT
// #include "stm32f103xb.h"
extern void usart1PrioritySetAndEnable();
extern void usart2PrioritySetAndEnable();
#endif

#define FIFO_LOG_LEN 1024

typedef struct
{
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t BRR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t GTPR;

} MyUSART_tydef;

typedef enum
{
    standard_log = 1, // 函数内部打印信息
    info_log = 2,     // 用于接收到信息
    error_log = 4,    // 出错
    system_log = 8,   // 系统级，shell 在usart2 中也当做消息通知

} LOG_LEVEL;

typedef enum
{
    UART_TX_FAIL,
    UART_TX_READY,
    UART_TX_BUSY,
    UART_TX_DR_EMPTY,
    UART_TX_NONE
} UART_ERROR_CODE;

typedef enum
{
    FIFO_IN_OR_OUT_COMPLETE = 0, // 写或读fifo成功
    FIFO_LAST_DATA,              // 最后一个数据 用于中断
    FIFO_OUT_FAIL,               // 读fifo失败，因为in = out, fifo为空
    FIFO_IN_FAIL                 // 写fifo失败，因为fifo写满了
} FIFO_ERROR_CODE;

typedef struct
{
    // FIFO buff
    uint8_t buff[FIFO_LOG_LEN];
    // 下次写fifo index
    uint16_t in;
    // 下次读fifo index
    uint16_t out;
    uint16_t fifo_len;
} ST_FIFO_LOG;

typedef struct
{
    // uint16_t fifo_len;
    volatile UART_ERROR_CODE uart_error_code;
    // 输出的fifo
    ST_FIFO_LOG fifo_log;
#if uart_IT
    // 输入的fifo
    ST_FIFO_LOG fifo_log_in;
#endif
    union uart_printf
    {
        LOG_LEVEL log_level;
        uint8_t frame_count; // usart2接收到的帧数
    } u;

    uint8_t initFifo;
} HANDLE_LOG_FIFO;

/******************Macro definition function************/

#define UART_ASSERT(x) \
    if ((x) == 0)      \
    {                  \
        for (;;)       \
            ;          \
    }

/***********************function***********************/

// 发动到DR寄存器
void uart_out_IT();
void uart_in_IT(uint8_t data);
// 发动到DR寄存器
void uart2_out_IT();
void uart2_in_IT(uint8_t data);
// 从dr寄存器读字节
void uart_in(uint8_t *data);
// 写字节到DR寄存器
void uart_out(uint8_t data);
void uart2_in(uint8_t *data);
// 写字节到DR寄存器
void uart2_out(uint8_t data);

// 使用串口打印
uint8_t uart_printf(uint8_t logLevel, const void *strData, const uint8_t *data, const uint8_t len);
uint8_t uart2_communicate_tx(const uint8_t *data, const uint8_t len);
uint8_t uart2_communicate_rx();
// 从串口读取数据
uint8_t uart_get(uint8_t *data);
void STR_TO_HEX(char *str, uint8_t *data);
void cancel_log_level(uint8_t level);
void set_log_level(uint8_t level);
void set_frame_count(uint8_t count);
uint8_t get_frame_count();
void uart2_init();
void uart2_hardware_init();

void convert_uint8_t_to_uint32_t(const uint8_t *data_to_uint8_t, uint32_t *data);
void convert_uint32_t_to_uint8_t(const uint32_t data, uint8_t *data_to_uint8_t);
/******************************************************/
#endif