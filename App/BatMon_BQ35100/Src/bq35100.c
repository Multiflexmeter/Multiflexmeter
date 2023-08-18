/**
 * @file bq35100.c
 *
 * @brief Driver to interface with the bq35100 battery monitor.
 *
 */

#include "../Inc/bq35100.h"

static I2C_HandleTypeDef *bq35100Handle;

void bq35100_init(I2C_HandleTypeDef *i2cHandle)
{
  bq35100Handle = i2cHandle;
}

/**
 * @brief Reads the battery voltage form the BQ35100
 *
 * @return The battery voltage in mV.
 */
int16_t bq35100_getVoltage(void)
{
  int16_t voltage;
  uint8_t data[2] = {0xFF, 0xFF};
  uint8_t command = BQ35100_REG_VOLTAGE;

  HAL_I2C_Mem_Read(bq35100Handle, BQ35100_ADDRESS, command, 1, data, 2, 100);

  voltage = (data[1]<<8 & 0xFF00) | (data[0] & 0x00FF);

  return voltage;
}

/**
 * @brief Reads the battery current form the BQ35100
 *
 * @return The battery current in mA.
 */
int16_t bq35100_getCurrent(void)
{
  int16_t current;
  uint8_t data[2] = {0xFF, 0xFF};
  uint8_t command = BQ35100_REG_CURRENT;

  HAL_I2C_Mem_Read(bq35100Handle, BQ35100_ADDRESS, command, 1, data, 2, 100);

  current = (data[1]<<8 & 0xFF00) | (data[0] & 0x00FF);

  return current;
}

/**
 * @brief Reads the temperature form the BQ35100
 *
 * @return The temperature in degrees celsius.
 */
float bq35100_getTemp(void)
{
  float temperature;
  uint8_t data[2] = {0xFF, 0xFF};
  uint8_t command = BQ35100_REG_TEMPERATURE;

  HAL_I2C_Mem_Read(bq35100Handle, BQ35100_ADDRESS, command, 1, data, 2, 100);

  temperature = ((data[1]<<8 & 0xFF00) | (data[0] & 0x00FF))/10 - 273;

  return temperature;
}

/**
 * @brief Reads the security mode of the BQ35100
 *
 * @return The BQ35100 security mode.
 */
SecurityMode bq35100_getSecurityMode(void)
{
  SecurityMode securityMode;
  uint8_t data[2];
  uint8_t command = BQ35100_REG_CONTROL;

  HAL_I2C_Mem_Read(bq35100Handle, BQ35100_ADDRESS, command, 1, data, 2, 100);

  //todo always results in 0 even though the formula is correct
  const uint8_t test = ((data[1]>>5) & 0x3);
  securityMode = test;

  return securityMode;
}
