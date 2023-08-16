#ifndef CRC16_TABLE_H
#define CRC16_TABLE_H

#include <stdint.h>

#define CRC16INITVALUE  0xFFFF  //Initial CRC value

uint16_t calculateCRC_CCITT(uint8_t * data, int length);

#endif
