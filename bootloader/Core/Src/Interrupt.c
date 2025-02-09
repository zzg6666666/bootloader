#include "Interrupt.h"
#include "uart_printf.h"

static InterruptPriorityConf *InterruptPriority = (InterruptPriorityConf *)0xE000E400;
static InterruptEnableConf *InterruptEnable = (InterruptEnableConf *)0xE000E100;

uint8_t data_count = 0;         // 收到的消息帧计数
uint8_t get_frame_head = false; // 收到了信息头

extern MyUSART_tydef *USART1_BASS;
extern MyUSART_tydef *USART2_BASS;
// 设置优先级分组
void myPriorityGroupSetting()
{
    // 2 抢占 2 子
    volatile uint32_t *SCB_AIRCR = (uint32_t *)0xE000ED00;
    *SCB_AIRCR |= (0x05FAUL << 16) | (0x05 << 8);
}

// 设置uart中断的优先级和使能 最低的15
void usart1PrioritySetAndEnable()
{
    // 中断的接受和发送空中断由程序代码控制

    // 中断优先级
    InterruptPriority->IP[37] = 15UL << 4;
    // 使能中断
    InterruptEnable->ISER[37 / 32] = 1UL << (37 - 32);
}

// 设置uart中断的优先级和使能 最低的15
void usart2PrioritySetAndEnable()
{
    // 中断的接受和发送空中断由程序代码控制

    // 中断优先级
    InterruptPriority->IP[38] = 15UL << 4;
    // 使能中断
    InterruptEnable->ISER[38 / 32] = 1UL << (38 - 32);
}

// 中断函数
void USART2_IRQHandler(void)
{
    // 发送寄存器空 且开启了发送中断
    if (((USART2_BASS->SR & (0x1UL << 7)) == 0x1UL << 7) && ((USART2_BASS->CR1 & (0x1UL << 7)) == 0x1UL << 7))
    {
        uart2_out_IT();
    }
    // 接受寄存器满 且开启了发送中断
    if (((USART2_BASS->SR & (0x1UL << 5)) == 0x1UL << 5) && ((USART2_BASS->CR1 & (0x1UL << 5)) == 0x1UL << 5))
    {
#if uart_IT
        uint8_t data = USART2_BASS->DR;

        // 开始接收新的一帧
        if (data == 0x5a && data_count == 0)
        {
            // 标记为接收到了第二字节
            get_frame_head = true;
            data_count = 1;
            return;
        }

        // 接收到了一帧的第二字节
        if (get_frame_head == true)
        {
            if (data == 0xa5)
            {
                // 允许接收
                data_count = 2;
                uart2_in_IT(0x5a);
                uart2_in_IT(0xa5);
            }
            else
            {
                data_count = 0; // 不是第二字节
            }
            get_frame_head = false;
            return;
        }

        // 写入数据
        if (data_count >= 2)
        {
            uart2_in_IT(data);
            data_count++;
            if (data_count == 14) // 接满一帧
            {
                uint8_t temp = get_frame_count();
                set_frame_count(temp + 1);
                data_count = 0; //重新接收帧头
            }
        }
#endif
    }
}
void USART1_IRQHandler(void)
{
    // 发送寄存器空 且开启了发送中断
    if (((USART1_BASS->SR & (0x1UL << 7)) == 0x1UL << 7) && ((USART1_BASS->CR1 & (0x1UL << 7)) == 0x1UL << 7))
    {
        uart_out_IT();
    }
    // 接受寄存器满 且开启了发送中断
    if (((USART1_BASS->SR & (0x1UL << 5)) == 0x1UL << 5) && ((USART1_BASS->CR1 & (0x1UL << 5)) == 0x1UL << 5))
    {
#if uart_IT
        uart_in_IT(USART1_BASS->DR);
#endif
    }
}