#include "rcc.h"
#include "command.h"

RCC_TypeDef *MyRCC_BASS = (RCC_TypeDef *)0x40021000;

extern FLASH_TypeDef *FLASH_BASS;

uint8_t init_HSE_SYSCLK_72MHZ()
{
    uint16_t i = 0;
    // 打开RCC_HSE bit16
    MyRCC_BASS->CR |= 1UL << 16;

    // 等待 RCC_HSE 就绪 bit17
    for (i = 0; i < 0xffff; i++)
    {
        if ((MyRCC_BASS->CR & 1UL << 17) == 1UL << 17)
        {
            break;
        }
    }
    if (i == 255)
    {
        return false;
    };

    // 系统时钟不能为PLL,否则不能配置PLL
    if ((MyRCC_BASS->CFGR & 1UL << 3) == 1UL << 3)
    {
        return false;
    }

    // 关闭PLL，等待配置PLL bit24
    MyRCC_BASS->CR &= ~(1UL << 24);

    // 等待PLL 处于解锁 bit25
    for (i = 0; i < 0xffff; i++)
    {
        if ((MyRCC_BASS->CR & 1UL << 25) != 1UL << 25)
        {
            break;
        }
    }
    if (i == 255)
    {
        return false;
    };
    // 配置HSE不倍频 bit17
    MyRCC_BASS->CFGR &= 1UL << 17;

    // 配置HSE作为PLL时钟输入
    MyRCC_BASS->CFGR |= 1UL << 16;

    // 配置PLL倍频率 21:18 为9倍  配置PLL来源为HSE
    MyRCC_BASS->CFGR |= (0x07UL << 18 | 0x01UL << 2);

    // 使能PLL作为时钟来源
    MyRCC_BASS->CR |= (1UL << 24);

    // 等待PLL 处于上锁 bit25
    for (i = 0; i < 0xffff; i++)
    {
        if ((MyRCC_BASS->CR & 1UL << 25) == 1UL << 25)
        {
            break;
        }
    }
    if (i == 255)
    {
        return false;
    };

    // 启用指令预取
    FLASH_BASS->ACR |= 0x01UL << 4;

    // 启用指令预取功能为2个周期
    FLASH_BASS->ACR &= ~0x07UL;
    FLASH_BASS->ACR |= 0x02UL;

    // 设置SYSCLK 为 PLL
    MyRCC_BASS->CFGR |= 0x02UL;

    // 等待SYSCLK 为 PLL
    for (i = 0; i < 0xffff; i++)
    {
        if ((MyRCC_BASS->CFGR & 0x02UL << 2) == 0x02UL << 2)
        {
            break;
        }
    }
    if (i == 255)
    {
        return false;
    };
    return true;
}

void init_AHB_APB1_APB2()
{

    // AHB 不分频
    MyRCC_BASS->CFGR &= ~(0x0f << 4);

    // APB1 2分频(PCLK1)
    MyRCC_BASS->CFGR &= ~(0x07 << 8);
    MyRCC_BASS->CFGR |= (0x04 << 8);

    // APB2 不分频
    MyRCC_BASS->CFGR &= ~(0x07 << 11);
}