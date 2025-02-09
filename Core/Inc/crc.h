#ifndef _CRC_H_
#define _CRC_H_
#include "typedef_conf.h"
void crc_init(void);


void crc_init();
uint32_t crc_calculate(uint32_t *data, uint32_t dataLength);
void crc_reset();
#endif