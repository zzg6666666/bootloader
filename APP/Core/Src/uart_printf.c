#include "uart_printf.h"
#include "string.h"       //strlen
#include "typedef_conf.h" //uint8_t ..
#include "rcc.h"
#include "crc.h"
#include "uds.h"

// 系统时钟HCLK
extern uint32_t SystemCoreClock;
extern RCC_TypeDef *MyRCC_BASS;
// fifo的句柄 uart1
static HANDLE_LOG_FIFO log_fifo_handle = {.initFifo = false};
// fifo的句柄 uart2
static HANDLE_LOG_FIFO log_fifo_handle2 = {.initFifo = false};

// GPIOA 端口L配置寄存器
static volatile uint32_t *GPIOA_CRL = (uint32_t *)(0x40010800);
// GPIOA 端口h配置寄存器
static volatile uint32_t *GPIOA_CRH = (uint32_t *)(0x40010800 + 0x04UL);

// // USART1 寄存器基地址
// const uint32_t USART1_BASS = 0x40013800;

#if !uart_IT
// USART1_SR寄存器
// static volatile uint32_t *USART1_SR = (uint32_t *)0x40013800;
#endif

// APB1、2 倍频倍数
uint8_t APBPrescTable[8U] = {0, 0, 0, 0, 1, 2, 3, 4};
MyUSART_tydef *USART1_BASS = (MyUSART_tydef *)0x40013800UL;
MyUSART_tydef *USART2_BASS = (MyUSART_tydef *)0x40004400UL;

/************ usart 1 中断*************/
// 将data写到DR寄存器
#define UART_SEND_DATA_DR(data) USART1_BASS->DR = (data & 0xFF)
// 禁用DR寄存器空中断
#define DISABLE_UART_TX_DR_IT() USART1_BASS->CR1 &= ~(1U << 7)
// 启用DR寄存器空中断
#define ENABLE_UART_TX_DR_IT() USART1_BASS->CR1 |= (1U << 7)
// 禁用DR寄存器满中断
#define DISABLE_UART_RX_DR_IT() USART1_BASS->CR1 &= ~(1U << 5)
// 启用DR寄存器满中断
#define ENABLE_UART_RX_DR_IT() USART1_BASS->CR1 |= (1U << 5)

/************ usart 2 中断*************/
// 将data写到DR寄存器
#define UART2_SEND_DATA_DR(data) USART2_BASS->DR = (data & 0xFF)
// 禁用DR寄存器空中断
#define DISABLE_UART2_TX_DR_IT() USART2_BASS->CR1 &= ~(1U << 7)
// 启用DR寄存器空中断
#define ENABLE_UART2_TX_DR_IT() USART2_BASS->CR1 |= (1U << 7)
// 禁用DR寄存器满中断
#define DISABLE_UART2_RX_DR_IT() USART2_BASS->CR1 &= ~(1U << 5)
// 启用DR寄存器满中断
#define ENABLE_UART2_RX_DR_IT() USART2_BASS->CR1 |= (1U << 5)

// 写数据到fifo
static uint8_t uart_fifo_put(ST_FIFO_LOG *fifo, const void *data, const uint16_t dataLen);
// 初始化FIFO
static void uart_printf_init(HANDLE_LOG_FIFO *handle);
// 初始化uart硬件
static void uart_hardware_init();
void uart2_hardware_init();
// 从fifo出数据
static uint8_t uart_fifo_get(ST_FIFO_LOG *fifo, uint8_t *data, const uint16_t dataLen);
// 格式转换
static void HEX_TO_STR(uint8_t data, char *str);
// 从uart读取数据
void uart_in(uint8_t *data);
// 仅操作DR寄存器
void uart_out(uint8_t data);

// 从uart读取数据
void uart2_in(uint8_t *data);
// 仅操作DR寄存器
void uart2_out(uint8_t data);

// 初始化fifo
static void uart_printf_init(HANDLE_LOG_FIFO *handle)
{
    // handle->fifo_len = FIFO_LOG_LEN;
    handle->initFifo = true;
    handle->uart_error_code = UART_TX_READY;

    handle->fifo_log.in = 0;
    handle->fifo_log.out = 0;
    handle->fifo_log.fifo_len = FIFO_LOG_LEN;
    if (handle == &log_fifo_handle2)
    {
        handle->u.frame_count = 0;
    }
    else
    {
        handle->u.log_level = info_log | system_log | standard_log | error_log;
    }

#if uart_IT
    handle->fifo_log_in.in = 0;
    handle->fifo_log_in.out = 0;
    handle->fifo_log_in.fifo_len = FIFO_LOG_LEN;
#endif
}

static uint8_t uart_fifo_put(ST_FIFO_LOG *fifo, const void *data, const uint16_t dataLen)
{
    // 此次写到fifo的长度
    uint16_t maxLength = 0;
    // fifo的剩余空间
    uint16_t buffSize = 0;
    // 从in到fifo_len的长度
    uint16_t forwardLength = 0;
    uint8_t *buff = NULL;
    // 下次写的位置
    uint16_t *in = NULL;
    // 下次读的位置
    uint16_t *out = NULL;

    // 初始化指针
    in = &(fifo->in);
    out = &(fifo->out);
    buff = &(fifo->buff[0]);

    // 检查buff是否已经满  w + 1 = r 时， buff就是满的状态
    // 检查缓冲区是否已满

    if ((*in == (fifo->fifo_len - 1) && *out == 0) || (*in + 1) == *out)
    {
        return FIFO_IN_FAIL;
    }

    // 计算出buff的容量

    // *in > *out时,能写的位置是 buff[in] 到 buff[len -1],  buff[0] 到 buff[out -1]
    if ((*in) > (*out))
    {
        buffSize = (fifo->fifo_len - (*in)) + (*out);
    }
    // buff是空的，这个时候，能从buff[0] 写到 buff[len -1]
    else if ((*out) == (*in))
    {
        buffSize = fifo->fifo_len;
    }
    // *in < *out,此时能从buff[in] 写到 buff[out -1]
    else
    {
        buffSize = *out - *in;
    }

    /*计算出，此次写buff的数据长度
      当 *in > *out时,能从buff[*in],写到buff[bufflen -1],从buff[0]写到buff[buffsize - forwardLength -1]
      假如buff长度= 20,*in = 15，*out = 8, buffsize = 20-15+8=13,forwardLength=5,
      能从buff[15]写到buff[19],从buff[0]写到buff[7]，更新*in为8。但是*out=8,不符合 *in +1 = *w的规则。
      设置maxLength为buffSize - 1 和 dataLen的最小值，maxLength = 12在写满buff的情况下,能从buff[0]-buff[6],
      更新*in = maxLength - forwardLength = 7，符合判定满的规则

      假如buff长度= 20,*in = 8，*out = 15,buffSize =15-8=7,在写满的情况下，设置maxLength = 7时，从buff[8]到
      buff[14],更新*in为15,和*out =15冲突不满足判定满的情况，设置maxLength为buffSize - 1 和 dataLen的最小值，
      maxLength = 6,从buff[8]到buff[13],更新*in为14,符合判定满的规则
    */

    maxLength = MIN(buffSize - 1, dataLen);

    // 当 *in > *out 时，计算出从fifo[in]到fifo[fifo_len - 1]的长度
    forwardLength = fifo->fifo_len - *in;

    // maxLength > forwardLength ,一定是*in > *out的情况,需要写 buff[*in] 到 buff[len -1],buff[0]到buff[maxLength - forwardLength -1]
    if (maxLength > forwardLength)
    {

        memcpy(&buff[*in], data, forwardLength);                                        // 复制到 buff[*in] 到 buff[len -1]
        memcpy(&buff[0], &((uint8_t *)data)[forwardLength], maxLength - forwardLength); // 复制到 buff[0] 到 buff[ maxLength - forwardLength]

        // 更新 *in
        *in = maxLength - forwardLength; // 下次写入的位置是buff[maxLength - forwardLength]
    }
    //*in <= *out , maxLength =forwardLength  写入 buff[in] 到 buff[in-1] , 或是 *out < *in 且 写入长度小于forwardLength,写入的位置是buff[*in] 到 buff[*in + maxlen ]
    else
    {
        memcpy(&buff[*in], data, maxLength);
        *in = *in + maxLength;

        // 防止*in溢出
        if (*in == fifo->fifo_len)
        {
            *in = 0;
        }
    }

    return FIFO_IN_OR_OUT_COMPLETE;
}

// 从fifo中获得数据
static uint8_t uart_fifo_get(ST_FIFO_LOG *fifo, uint8_t *data, const uint16_t dataLen)
{
    // 此次进fifo的最长长度
    uint16_t maxLength = 0;
    // 从in到fifo_len的长度
    uint16_t forwardLength = 0;
    uint8_t *buff;
    uint16_t *in = 0;
    uint16_t *out = 0;

    in = &(fifo->in);
    out = &(fifo->out);
    buff = &(fifo->buff[0]);

    // buf内是否有数据
    if (*in == *out)
    {

        return FIFO_OUT_FAIL;
    }

    // 计算已经入buff数据长度
    if (*in < *out)
    {
        // maxLength = log_fifo_handle.fifo_len - *out + *in;
        maxLength = fifo->fifo_len - *out + *in;
    }
    //*in > *out
    else
    {
        maxLength = *in - *out;
    }

    // 计算出从fifo[out]到fifo[fifo_len - 1]的长度
    forwardLength = fifo->fifo_len - *out;

    /* 此次出buff的长度,不能超过buff的长度*/
    maxLength = MIN(maxLength, dataLen);
    if (maxLength > forwardLength)
    {
        memcpy(data, &buff[*out], forwardLength);
        memcpy(&data[forwardLength], buff, maxLength - forwardLength);
        *out = maxLength - forwardLength;
    }
    else
    {
        memcpy(data, &buff[*out], maxLength);
        *out = *out + maxLength;
        if (*out == fifo->fifo_len)
        {
            *out = 0;
        }
    }

    if (*in == *out)
    {
        return FIFO_LAST_DATA;
    }

    return FIFO_IN_OR_OUT_COMPLETE;
    // 开启os调度
}

uint8_t uart_printf(uint8_t logLevel, const void *strData, const uint8_t *data, const uint8_t len)
{
#if use_RTOS
    // 关闭中断
    portDISABLE_INTERRUPTS();

#endif

    // 初始化
    if (log_fifo_handle.initFifo == false)
    {
        uart_hardware_init();
        uart_printf_init(&log_fifo_handle);
    }

    // 检查当前log等级

    if ((logLevel & log_fifo_handle.u.log_level) == 0x00)
    {
        return 0;
    }

    // 将数据放入到FIFO中
    if (strData != NULL)
    {
        uart_fifo_put(&log_fifo_handle.fifo_log, strData, strlen((char *)strData));
    }

    if (data != NULL)
    {
        // uart_fifo_put(strData, strlen((char *)strData));

        // 储存HEX_TO_STR()的str,"-"，"ASCLl"，"ASCLl"
        char tempData[3] = "";
        for (uint8_t i = 0; i < len; i++)
        {
            HEX_TO_STR(*data, &tempData[0]);
            data++;

            // i == 1 删除 "-"
            if (i == 0)
            {
                // 复制"ascll"、"ascll"
                uart_fifo_put(&log_fifo_handle.fifo_log, &tempData[1], 2);
            }
            else
            {
                // 复制"-"","ascll"、"ascll"
                uart_fifo_put(&log_fifo_handle.fifo_log, &tempData[0], 3);
            }
        }
        uart_fifo_put(&log_fifo_handle.fifo_log, "\r\n", 4);
    }
#if uart_IT
    // 使能串口中断
    if (log_fifo_handle.uart_error_code == UART_TX_READY)
    {
        // 打开串口中断 发送DR空中断
        log_fifo_handle.uart_error_code = UART_TX_BUSY;
        ENABLE_UART_TX_DR_IT();
    }
#else

    uint8_t temp = 0;
    while (uart_fifo_get(&log_fifo_handle.fifo_log, &temp, 1) != FIFO_OUT_FAIL)
    {
        uart_out(temp);
    }

#endif

#if use_RTOS
    // 打开中断
    portENABLE_INTERRUPTS();

#endif
    return 0;
}

uint8_t uart_get(uint8_t *data)
{
#if uart_IT

    if (uart_fifo_get(&log_fifo_handle.fifo_log_in, data, 1) == FIFO_OUT_FAIL)
    {
        return false;
    }
    else
    {
        return true;
    }

#else
    uart_in(data);
    return true;
#endif
}

void uart_out_IT()
{
    uint8_t data = 0;
    uint8_t fifo_status = uart_fifo_get(&log_fifo_handle.fifo_log, &data, 1);
    if (fifo_status == FIFO_LAST_DATA)
    {
        log_fifo_handle.uart_error_code = UART_TX_READY;
        UART_SEND_DATA_DR(data);
        DISABLE_UART_TX_DR_IT();
    }
    else
    {
        UART_SEND_DATA_DR(data);
    }
}
void uart2_out_IT()
{
    uint8_t data = 0;
    uint8_t fifo_status = uart_fifo_get(&log_fifo_handle2.fifo_log, &data, 1);
    if (fifo_status == FIFO_LAST_DATA)
    {
        log_fifo_handle2.uart_error_code = UART_TX_READY;
        UART2_SEND_DATA_DR(data);
        DISABLE_UART2_TX_DR_IT();
    }
    else
    {
        UART2_SEND_DATA_DR(data);
    }
}

void uart_in(uint8_t *data)
{
    while ((USART1_BASS->SR & (1U << 5)) == 0)
    {
    }
    *data = USART1_BASS->DR;
}

void uart2_in(uint8_t *data)
{
    while ((USART2_BASS->SR & (1U << 5)) == 0)
    {
    }
    *data = USART2_BASS->DR;
}

#if uart_IT
void uart_in_IT(uint8_t data)
{
    uart_fifo_put(&log_fifo_handle.fifo_log_in, &data, 1);
}
void uart2_in_IT(uint8_t data)
{
    uart_fifo_put(&log_fifo_handle2.fifo_log_in, &data, 1);
}
#endif

void uart_out(uint8_t data)
{
    // *USART1_DR = data & 0xFF;
    while ((USART1_BASS->SR & (1U << 7)) == 0)
    {
    }
    USART1_BASS->DR = data;
}

void uart2_out(uint8_t data)
{
    // *USART1_DR = data & 0xFF;
    while ((USART2_BASS->SR & (1U << 7)) == 0)
    {
    }
    USART2_BASS->DR = data;
}

// 输入data 转换成asill
static void HEX_TO_STR(uint8_t data, char *str)
{
    uint8_t temp = 0;
    memcpy(str, "-", 1);

    for (uint8_t i = 0; i < 2; i++)
    {
        // 高4位
        if (i == 0)
        {
            temp = (data & 0xf0) >> 4;
        }
        // 低四位
        else
        {
            temp = data & 0x0f;
        }
        temp = temp < 0x0a ? (temp + '0') : (temp - 0x0a + 'A');

        memcpy(&str[i + 1], &temp, 1);
    }
}

// 输入2字节的char 返回成uint8_t
void STR_TO_HEX(char *str, uint8_t *data)
{

    *data = 0;
    uint8_t i = 1;
    while (*str != '\0')
    {
        if (('/' < *str) && (*str < ':'))
        {
            *data |= (*str - '0') << (4 * i);
        }
        // ascll A-F
        else if (('@' < *str) && (*str < 'G'))
        {
            *data |= (*str - 'A' + 0x0a) << (4 * i);
        }
        // ascll a-f
        else if (('`' < *str) && (*str < 'g'))
        {
            *data |= (*str - 'a' + 0x0a) << (4 * i);
        }
        str++;

        if (i == 1)
        {
            i = 0;
        }
        else
        {
            i = 1;
            data++;
            *data = 0;
        };
    }
}

// 串口硬件初始化
static void uart_hardware_init()
{

    uint32_t temp = 0;
    // USARTDIV的整数部分
    uint32_t DIV_Mantissa = 0;
    // USARTDIV的小数部分
    uint32_t DIV_Fraction = 0;

    /* 启用时钟 */

    // 启用USART1时钟
    MyRCC_BASS->APB2ENR |= (1U << 14);

    // 启用GPIO A时钟
    MyRCC_BASS->APB2ENR |= (1U << 2);

    /* 配置gpio pin (确保GPIOA_CRH = 0x4bX)*/

    // 清除GPIO A9的配置位 bit4:7
    *GPIOA_CRH &= 0xFFFFFF0F;

    // 配置GPIO A9 输出模式为复用推挽输出(bit6:7)和输出速度为高(bit4:5)
    *GPIOA_CRH |= (3U << 4) | (2U << 6);

    // 清除GPIO A10的配置位 bit8:11
    *GPIOA_CRH &= 0xFFFFF0FF;

    // 配置GPIO A10 输出模式为浮空输入(bit10:11)和输入模式(bit8:9)
    *GPIOA_CRH |= (1U << 10);

    /*配置串口参数*/

    // 配置字长bit12、校验使能bit9、奇偶校验bit8、接受使能bit3、发送使能bit2
    // 清除位
    USART1_BASS->CR1 &= (~((1U << 12) | (1U << 9) | (1 << 8) | (1 << 3) | (1 << 2)));

    // 配置位 bit2、3为1，其他位为0
    USART1_BASS->CR1 |= ((1UL << 2) | (1U << 3));

    // 配置字长(bit12:13) 一个停止位、禁用LIN模式bit14、禁用CLKEN bit11
    // 清除位
    USART1_BASS->CR2 &= (~(3U << 12) | (1U << 11) | (1U << 14));

    // 配置禁用红外模式、半双工模式、智能卡模式
    // 清除位
    USART1_BASS->CR3 &= (~(3U << 1) | (1U << 3) | (1U << 5));

    // 配置 USART_BRR寄存器 USART1时钟源PCLK2,先获取到PCLK2的分频率数
    temp = (MyRCC_BASS->CFGR & (7U << 11)) >> 11;  // 分频倍数
    temp = SystemCoreClock >> APBPrescTable[temp]; // PCLK2 频率

    // 波特率计算公式 ：baud = fck/(16 * USARTDIV )，USARTDIV =fck / ( 16 * baud )

    // 计算整数部分， 将PCLK2 放大100倍数(HAL库)，用于提高精度
    DIV_Mantissa = (((temp * 25U) / (4U * 115200U)) / 100U);
    // 小数部分 *16是为了将小数转换成二进制数 加50是补偿(手动四舍五入)
    DIV_Fraction = ((((temp * 25U) / (4U * 115200U)) - DIV_Mantissa * 100U) * 16U + 50U) / 100U;
    USART1_BASS->BRR = (DIV_Mantissa << 4) + DIV_Fraction;

#if uart_IT

    // // 设置USART1 中断优先级 nvic 偏移值0x0000_00D4
    // NVIC_SetPriority(37, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));

    // // 启用中断
    // NVIC_EnableIRQ(37);

    // 启用优先级
    usart1PrioritySetAndEnable();

    // 启用接收中断
    ENABLE_UART_RX_DR_IT();

#endif
    // 使能USART1
    USART1_BASS->CR1 |= (1UL << 13);
}

// 允许某个等级的log
void set_log_level(uint8_t level)
{
    if (level <= system_log)
    {
        log_fifo_handle.u.log_level |= level;
    }
}

void set_frame_count(uint8_t count)
{
    log_fifo_handle2.u.frame_count = count;
}
uint8_t get_frame_count()
{
    return log_fifo_handle2.u.frame_count;
}

// 禁止某个等级的log
void cancel_log_level(uint8_t level)
{
    if (level <= system_log)
    {

        level = (~level) & 0xff;

        log_fifo_handle.u.log_level &= level;
    }
}

// 配置usart2
void uart2_hardware_init()
{
    /**配置gpio**/
    // PA2     ------> USART2_TX
    // PA3     ------> USART2_RX
    MyRCC_BASS->APB2ENR |= (1U << 2); // gpio_portA时钟

    // 配置PA2 11:10 输出模式 9:8 io模式
    // *GPIOA_CRL &= 0XFFFFF0FF;
    *GPIOA_CRL &= (~(0xF << (2 * 4)));
    *GPIOA_CRL |= (0x2 << (2 * 4 + 2)) | (0x3 << (2 * 4)); // 复用推挽输出

    *GPIOA_CRL &= (~(0xF << (3 * 4)));
    *GPIOA_CRL |= (0x1 << (3 * 4 + 2)) | (0x00 << (3 * 4)); // 浮空输入

    /**配置串口参数**/

    // 启用USART2时钟
    MyRCC_BASS->APB1ENR |= 1UL << 17;

    // 配置字长bit12、校验使能bit9、奇偶校验bit8、接受使能bit3、发送使能bit2
    // 清除位
    USART2_BASS->CR1 &= (~((1U << 12) | (1U << 9) | (1 << 8) | (1 << 3) | (1 << 2)));

    // 配置位 bit2、3为1，其他位为0
    USART2_BASS->CR1 |= ((1U << 2) | (1U << 3));

    // 配置字长(bit12:13) 一个停止位、禁用LIN模式bit14、禁用CLKEN bit11
    // 清除位
    USART2_BASS->CR2 &= (~(3U << 12) | (1U << 11) | (1U << 14));

    // 配置禁用红外模式、半双工模式、智能卡模式
    // 清除位
    USART2_BASS->CR3 &= (~(3U << 1) | (1U << 3) | (1U << 5));

    uint32_t temp = 0;
    // 配置 USART_BRR寄存器 USART2时钟源PCLK1,先获取到PCLK1的分频率数
    temp = (MyRCC_BASS->CFGR & (7U << 8)) >> 8;    // APB1分频倍数
    temp = SystemCoreClock >> APBPrescTable[temp]; // PCLK1 频率

    // USARTDIV的整数部分
    uint32_t DIV_Mantissa = 0;
    // USARTDIV的小数部分
    uint32_t DIV_Fraction = 0;
    // 计算整数部分， 将PCLK2 放大100倍数(HAL库)，用于提高精度
    DIV_Mantissa = (((temp * 25U) / (4U * 115200U)) / 100U);
    // 小数部分 *16是为了将小数转换成二进制数 加50是补偿(手动四舍五入)
    DIV_Fraction = ((((temp * 25U) / (4U * 115200U)) - DIV_Mantissa * 100U) * 16U + 50U) / 100U;
    USART2_BASS->BRR = (DIV_Mantissa << 4) + DIV_Fraction;

#if uart_IT
    // 打开串口2 中断
    usart2PrioritySetAndEnable();
    // 接收DR满中断
    ENABLE_UART2_RX_DR_IT();
#endif

    // 使能UASRT2
    USART2_BASS->CR1 |= (1UL << 13);
}

void uart2_init()
{
    log_fifo_handle2.initFifo = true;
    uart2_hardware_init();
    uart_printf_init(&log_fifo_handle2);
}

// used for chip communicate 一帧的头两个字节必须为为 0x5a 0xa5
uint8_t uart2_communicate_tx(const uint8_t *data, const uint8_t len)
{

    uart_printf(info_log, "uart2_communicate_tx", data, len);
    uint8_t tx_data[2 + 8 + 4] = {};
    uint8_t frame_head[2] = {0x5a, 0xa5};
    // 消息被拆分成的帧数
    uint8_t frame_length = 0;
    uint32_t CRC_result = 0;

    uint32_t data_to_uint32_t[2];
    uint8_t data_to_uint8_t[4];
    uint8_t temp_data[8] = {0x00};
    frame_length = len / 8;
    // 初始化硬件和fifo
    if (log_fifo_handle2.initFifo == false)
    {
        uart2_hardware_init();
        uart_printf_init(&log_fifo_handle2);
    }

    for (uint8_t i = 0; i < frame_length; i++) // 长度满足8字节
    {
        // 转换成32位，用于CRC
        convert_uint8_t_to_uint32_t(&data[i * 8], data_to_uint32_t);
        convert_uint8_t_to_uint32_t(&data[i * 8 + 4], &data_to_uint32_t[1]);
        // 进行CRC运算
        crc_reset();
        CRC_result = crc_calculate(data_to_uint32_t, 2);
        // 将crc 转换成 uint8_t;
        convert_uint32_t_to_uint8_t(CRC_result, data_to_uint8_t);
        memcpy(tx_data, frame_head, 2);           // 复制帧头到tx
        memcpy(&tx_data[2], &data[i], 8);         // 复制帧到tx
        memcpy(&tx_data[10], data_to_uint8_t, 4); // 复制crc到tx
        uart_fifo_put(&log_fifo_handle2.fifo_log, tx_data, 14);
        uart_printf(info_log, "tx ", tx_data, 14);
    }
    // 不满足长度为8的数据
    if (len % 8 != 0)
    {
        // 剩下的长度
        uint8_t data_len = len % 8;
        memset(temp_data, 0, 8);
        // 复制数据
        memcpy(&temp_data, &data[len - data_len], data_len);
        // 转换成32位，用于CRC
        convert_uint8_t_to_uint32_t(temp_data, data_to_uint32_t);
        convert_uint8_t_to_uint32_t(&temp_data[4], &data_to_uint32_t[1]);
        // 进行CRC运算
        crc_reset();
        CRC_result = crc_calculate(data_to_uint32_t, 2);
        // 将crc 转换成 uint8_t;
        convert_uint32_t_to_uint8_t(CRC_result, data_to_uint8_t);
        memcpy(tx_data, frame_head, 2);           // 复制帧头到tx
        memcpy(&tx_data[2], &temp_data, 8);       // 复制帧到tx
        memcpy(&tx_data[10], data_to_uint8_t, 4); // 复制crc到tx
        uart_fifo_put(&log_fifo_handle2.fifo_log, tx_data, 14);
        uart_printf(info_log, "tx ", tx_data, 14);
    }
 
// 使能发送dr空中断 进行数据发送
#if uart_IT
    // 使能串口中断
    if (log_fifo_handle2.uart_error_code == UART_TX_READY)
    {
        // 打开串口中断 发送DR空中断
        ENABLE_UART2_TX_DR_IT();
        log_fifo_handle2.uart_error_code = UART_TX_BUSY;
    }
#else
    uint8_t temp = 0;
    while (uart_fifo_get(&log_fifo_handle2.fifo_log, &temp, 1) != FIFO_OUT_FAIL)
    {
        uart2_out(temp);
    }
#endif
    return true;
}

uint8_t uart2_communicate_rx()
{
    uint8_t rx_data[14] = {0x00};
    uint32_t CRC_result = 0;
    uint32_t data_to_uint32_t[2];

    uint8_t return_value = true;
    // 读出一帧数据
    for (uint8_t i = 0; i < 14; i++)
    {
        return_value = uart_fifo_get(&log_fifo_handle2.fifo_log_in, &rx_data[i], 1);
    }

    uart_printf(info_log, "rx ", &rx_data[0], 14);

    // 校验
    if (rx_data[0] != 0x5a && rx_data[1] != 0xa5)
    {
        uart_printf(info_log, "rx data have not have right frame head \r\n", NULL, 0);
        return false;
    }
    convert_uint8_t_to_uint32_t(&rx_data[2], data_to_uint32_t);
    convert_uint8_t_to_uint32_t(&rx_data[6], &data_to_uint32_t[1]);
    convert_uint8_t_to_uint32_t(&rx_data[10], &CRC_result);
    // crc 计算
    crc_reset();
    if (CRC_result != crc_calculate(data_to_uint32_t, 2))
    {
        uart_printf(error_log, "the frame crc is wrong\r\n", NULL, 0);
        return false;
    }
    // 解析 信息
    can_service_entry(&rx_data[2]);
    // uds_entry(&rx_data[2], 8);

    return return_value;
}

void convert_uint32_t_to_uint8_t(const uint32_t data, uint8_t *data_to_uint8_t)
{
    data_to_uint8_t[0] = data >> 24;
    data_to_uint8_t[1] = (data & 0xff0000) >> 16;
    data_to_uint8_t[2] = (data & 0xff00) >> 8;
    data_to_uint8_t[3] = (data & 0xff);
}
void convert_uint8_t_to_uint32_t(const uint8_t *data_to_uint32_t, uint32_t *data)
{
    *data = 0;
    *data |= (data_to_uint32_t[0] << 24);
    *data |= (data_to_uint32_t[1] << 16);
    *data |= (data_to_uint32_t[2] << 8);
    *data |= (data_to_uint32_t[3]);
}