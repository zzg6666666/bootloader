#ifndef _GPIO_H_
#define _GPIO_H_

#include "typedef_conf.h"

typedef struct
{
    volatile uint32_t CRL;
    volatile uint32_t CRH;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t BRR;
    volatile uint32_t LCKR;
} MyGPIO_TypeDef;

#endif
