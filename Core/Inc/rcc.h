#ifndef _RCC_H_
#define _RCC_H_

#include "typedef_conf.h"

typedef struct
{
    volatile uint32_t CR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t APB2RSTR;
    volatile uint32_t APB1RSTR;
    volatile uint32_t AHBENR;
    volatile uint32_t APB2ENR;
    volatile uint32_t APB1ENR;
    volatile uint32_t BDCR;
    volatile uint32_t CSR;
} RCC_TypeDef;

//配置HSE作为PLL时钟源,PLL时钟为72MHZ，PLL作为系统时钟
uint8_t init_HSE_SYSCLK_72MHZ();
//配置AHB、APB1、APB2、PCLK1、PCLk2
void init_AHB_APB1_APB2();



#endif