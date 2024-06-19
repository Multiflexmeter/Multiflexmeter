/**
  ******************************************************************************
  * @file           : FRAM.h
  * @brief          : Header for FRAM.c file.
  * @author         : P.Kwekkeboom
  * @date           : Oct 12, 2023
  ******************************************************************************
  */

#ifndef FRAM_FRAM_H_
#define FRAM_FRAM_H_

typedef struct __attribute__((packed))
{
    uint8_t bit0_not_used:1;
    uint8_t bit1_WEL:1;
    uint8_t bit2_BP0:1;
    uint8_t bit3_BP1:1;
    uint8_t bit4_not_used:1;
    uint8_t bit5_not_used:1;
    uint8_t bit6_not_used:1;
    uint8_t bit7_WPEN:1;
} struct_RegisterRDSR;

typedef union
{
    uint8_t byte;
    struct_RegisterRDSR bit;
}UNION_RegisterRDSR;

const void FRAM_WriteData(uint16_t address, uint8_t *data, size_t length);
const void FRAM_ReadData(uint16_t address, uint8_t *data, size_t length);
const uint8_t FRAM_ReadStatusRegister(void);
const void FRAM_WriteStatusRegister(uint8_t data);


#endif /* FRAM_FRAM_H_ */
