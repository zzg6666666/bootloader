#include "crc.h"
const uint32_t CRC_BASE1 = 0x40023000UL;
// 时钟
uint32_t *RCC_AHBENR = (uint32_t *)(0x40021000UL + 0x14);
// crc
uint32_t *CRC_DR = (uint32_t *)(CRC_BASE1 + 0x00UL);
uint32_t *CRC_CR = (uint32_t *)(CRC_BASE1 + 0x08UL);
// 启用crc 时钟
void crc_init()
{
    // 设置RCC_AHBENR bit6
    *RCC_AHBENR |= 1UL << 6;
}

// 进行CRC运算
uint32_t crc_calculate(uint32_t *data, uint32_t dataLength)
{
    uint32_t crc_result = 0;
    for (uint32_t i = 0; i < dataLength; i++)
    {
        *CRC_DR = data[i];
    }

    // 读取crc
    crc_result = *CRC_DR;
    return crc_result;
}
// 重置crc DR 寄存器
void crc_reset()
{
    // 重置 crc
    *CRC_CR |= 1UL;
}