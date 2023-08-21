/**
 * @file bq35100.c
 *
 * @brief Driver to interface with the bq35100 battery monitor.
 *
 */

#include "bq35100.h"

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

  HAL_I2C_Mem_Read(bq35100Handle, BQ35100_ADDRESS, REG_VOLTAGE, 1, data, 2, 100);

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

  HAL_I2C_Mem_Read(bq35100Handle, BQ35100_ADDRESS, REG_CURRENT, 1, data, 2, 100);

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

  HAL_I2C_Mem_Read(bq35100Handle, BQ35100_ADDRESS, REG_TEMPERATURE, 1, data, 2, 100);

  temperature = ((data[1]<<8 & 0xFF00) | (data[0] & 0x00FF))/10 - 273;

  return temperature;
}

/**
 * @brief Reads the design capacity of the battery
 *
 * @return The design capacity in mAh.
 */
uint16_t bq35100_getDesignCapacity(void)
{
  uint16_t designCapacity;
  uint8_t data[2];

  HAL_I2C_Mem_Read(bq35100Handle, BQ35100_ADDRESS, REG_DESIGN_CAPACITY, 1, data, 2, 100);

  designCapacity = (data[1]<<8 & 0xFF00) | (data[0] & 0x00FF);

  return designCapacity;
}

/**
 * @brief Reads the used capacity of the battery
 *
 * @return The used capacity in uAh.
 */
uint32_t bq35100_getUsedCapacity(void)
{
  uint32_t usedCapacity;
  uint8_t data[4];

  HAL_I2C_Mem_Read(bq35100Handle, BQ35100_ADDRESS, REG_ACCUMULATED_CAPACITY, 1, data, 4, 100);

  usedCapacity = (((uint32_t) data[3]) << 24) + (((uint32_t) data[2]) << 16) + (((uint32_t) data[1]) << 8) + data[0];

  return usedCapacity;
}

/**
 * @brief Calculates the remaining battery capacity
 *
 * @return The remaining capacity in uAh.
 */
uint32_t bq35100_getRemainingCapacity(void)
{
  uint32_t usedCapacityUAh = bq35100_getUsedCapacity();
  uint32_t designCapacityUAh = (uint32_t) bq35100_getDesignCapacity * 1000;

  if(usedCapacityUAh > designCapacityUAh)
    usedCapacityUAh = designCapacityUAh;

  uint32_t remainingCapacityUAh = designCapacityUAh - usedCapacityUAh;

  return remainingCapacityUAh;
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

  HAL_I2C_Mem_Read(bq35100Handle, BQ35100_ADDRESS, REG_CONTROL, 1, data, 2, 100);

  //todo always results in 0 even though the formula is correct
  const uint8_t test = ((data[1]>>5) & 0x3);
  securityMode = test;

  return securityMode;
}

/**
 * @brief Check if the battery gauging is enabled or not.
 *
 * @return true if battery gauging is enabled, otherwise false.
 */
bool bq35100_isGaugeEnabled(void)
{
  uint8_t controlStatus[2];

  HAL_I2C_Mem_Read(bq35100Handle, BQ35100_ADDRESS, REG_CONTROL, 1, controlStatus, 2, 100);

  return (controlStatus[0] & 0x01) == 0x01;
}

/**
 * @brief Enables the battery gauge.
 *
 * @return true if successful, otherwise false.
 */
bool bq35100_enableGauge(void)
{
  uint8_t data[3];

  data[0] = 0x3e;                     // Set address to ManufacturerAccessControl
  data[1] = SUB_CMD_GAUGE_START;      // First byte of GAUGE_START sub-command (0x11)
  data[2] = SUB_CMD_GAUGE_START>>8;   // Second byte of GAUGE_START sub-command (0x00) (register address will auto-increment)

  HAL_I2C_Master_Transmit(bq35100Handle, BQ35100_ADDRESS, data, 3, 100);

  HAL_Delay(10);

  return bq35100_isGaugeEnabled();
}

/**
 * @brief Disables the battery gauge.
 *
 * @return true if successful, otherwise false.
 */
bool bq35100_disableGauge(bool ignoreCheck)
{
  uint8_t data[3];
  uint8_t controlStatus[2];

  data[0] = 0x3e;                     // Set address to ManufacturerAccessControl
  data[1] = SUB_CMD_GAUGE_STOP;       // First byte of GAUGE_STOP sub-command (0x12)
  data[2] = SUB_CMD_GAUGE_STOP>>8;    // Second byte of GAUGE_START sub-command (0x00) (register address will auto-increment)

  HAL_I2C_Master_Transmit(bq35100Handle, BQ35100_ADDRESS, data, 3, 100);

  // returns true to skip the check function to save time.
  if(ignoreCheck)
    return true;

  // Check if the device is done all its tasks and is turn off.
  for(uint8_t i=0; i<10; i++)
  {
    HAL_Delay(200);
    HAL_I2C_Mem_Read(bq35100Handle, BQ35100_ADDRESS, REG_CONTROL, 1, controlStatus, 2, 100);

    if((controlStatus[0] & 0x41) == 0x40)
      return !bq35100_isGaugeEnabled();
  }

  // returns false if the device is not turned of after 2 seconds
  return false;
}
