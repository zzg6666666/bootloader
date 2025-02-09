#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_
#include "typedef_conf.h"
// 内置中断配置寄存器

// 外部中断使能寄存器
typedef struct
{
    volatile uint32_t ISER[8];
} InterruptEnableConf;

// 外部中断优先级寄存器
typedef struct
{
    volatile uint8_t IP[240];
} InterruptPriorityConf;


void myPriorityGroupSetting();
void usart1PrioritySetAndEnable();
void usart2PrioritySetAndEnable();

void USART2_IRQHandler(void);
void USART1_IRQHandler(void);
#endif