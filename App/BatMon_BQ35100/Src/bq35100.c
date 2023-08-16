/**
 * @file bq35100.c
 *
 * @brief Driver to interface with the bq35100 battery monitor.
 *
 */

#include "../Inc/bq35100.h"

static I2C_HandleTypeDef bq35100Handle;

void bq35100_init(I2C_HandleTypeDef *i2cHandle)
{
  bq35100Handle = *i2cHandle;
}

int16_t bq35100_readVoltage(void)
{
  int16_t voltage;
  uint8_t data[2] = {0xFF, 0xFF};
  uint8_t command = BQ35100_REG_VOLTAGE;

  HAL_I2C_Mem_Read(&bq35100Handle, BQ35100_ADDRESS, command, 1, data, 2, 100);

  voltage = (data[1]<<8 & 0xFF00) | (data[0] & 0x00FF);

  return voltage;
}

int16_t bq35100_readCurrent(void)
{
  int16_t current;
  uint8_t data[2] = {0xFF, 0xFF};
  uint8_t command = BQ35100_REG_CURRENT;

  HAL_I2C_Mem_Read(&bq35100Handle, BQ35100_ADDRESS, command, 1, data, 2, 100);

  current = (data[1]<<8 & 0xFF00) | (data[0] & 0x00FF);

  return current;
}

float bq35100_readTemp(void)
{
  float temperature;
  uint8_t data[2] = {0xFF, 0xFF};
  uint8_t command = BQ35100_REG_TEMPERATURE;

  HAL_I2C_Mem_Read(&bq35100Handle, BQ35100_ADDRESS, command, 1, data, 2, 100);

  temperature = ((data[1]<<8 & 0xFF00) | (data[0] & 0x00FF))/10 - 273;

  return temperature;
}
