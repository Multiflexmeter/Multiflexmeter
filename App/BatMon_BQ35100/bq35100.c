/**
 * @file bq35100.c
 *
 * @brief Driver to interface with the bq35100 battery monitor.
 *
 */

#include "bq35100.h"

#define BQ35100_I2C_WAIT 110 //ms
//technical reference manual (SLUUBH1C) mention 100ms, this is not enough after GE pin is enabled.
//BQ35100 answer on I2C master request freezes for 103.5ms.

static I2C_HandleTypeDef *bq35100Handle;

/**
 * @brief Initialize the bq35100
 *
 * @param i2cHandle is the i2c handle of the peripheral on which the BQ35100 is connected.
 */
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

  HAL_I2C_Mem_Read(bq35100Handle, BQ35100_ADDRESS, REG_VOLTAGE, 1, data, 2, BQ35100_I2C_WAIT);

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

  HAL_I2C_Mem_Read(bq35100Handle, BQ35100_ADDRESS, REG_CURRENT, 1, data, 2, BQ35100_I2C_WAIT);

  current = (data[1]<<8 & 0xFF00) | (data[0] & 0x00FF);

  return current;
}

/**
 * @brief Reads the external temperature form the BQ35100
 *
 * @return The external temperature in degrees celsius *10 to display the decimal without a float.
 */
int16_t bq35100_getExternalTemp(void)
{
  int16_t temperature;
  uint8_t data[2] = {0xFF, 0xFF};

  HAL_I2C_Mem_Read(bq35100Handle, BQ35100_ADDRESS, REG_TEMPERATURE, 1, data, 2, BQ35100_I2C_WAIT);

  temperature = ((int16_t) (data[1]<<8 & 0xFF00) | (data[0] & 0x00FF)) - 2730;

  return temperature;
}

/**
 * @brief Reads the internal temperature form the BQ35100
 *
 * @return The internal temperature in degrees celsius *10 to display the decimal without a float.
 */
int16_t bq35100_getInternalTemp(void)
{
  int16_t temperature;
  uint8_t data[2] = {0xFF, 0xFF};

  HAL_I2C_Mem_Read(bq35100Handle, BQ35100_ADDRESS, REG_INTERNAL_TEMP, 1, data, 2, BQ35100_I2C_WAIT);

  temperature = ((int16_t) (data[1]<<8 & 0xFF00) | (data[0] & 0x00FF)) - 2730;

  return temperature;
}

/**
 * @brief Gets the scaled resistance from the battery. Only in EOS mode.
 *
 * @return The scaled resistance in mOhms
 */
uint16_t bq35100_getScaledR(void)
{
  uint16_t scaledR;
  uint8_t data[2];

  HAL_I2C_Mem_Read(bq35100Handle, BQ35100_ADDRESS, REG_SCALED_R, 1, data, 2, BQ35100_I2C_WAIT);

  scaledR = (data[1]<<8 & 0xFF00) | (data[0] & 0x00FF);

  return scaledR;
}

/**
 * @brief Gets the measured impedance from the battery. Only in EOS mode.
 *
 * @return The measured impedance in mOhms
 */
uint16_t bq35100_getMeasuredZ(void)
{
  uint16_t measuredZ;
  uint8_t data[2];

  HAL_I2C_Mem_Read(bq35100Handle, BQ35100_ADDRESS, REG_MEASURED_Z, 1, data, 2, BQ35100_I2C_WAIT);

  measuredZ = (data[1]<<8 & 0xFF00) | (data[0] & 0x00FF);

  return measuredZ;
}

/**
 * @brief Gets the state of health of the battery in EOS and SOH mode
 *
 * @return The state of health in a percentage.
 */
uint16_t bq35100_getStateOfHealth(void)
{
  uint16_t stateOfHealth;
  uint8_t data[2];

  HAL_I2C_Mem_Read(bq35100Handle, BQ35100_ADDRESS, REG_STATE_OF_HEALTH, 1, data, 2, BQ35100_I2C_WAIT);

  stateOfHealth = (data[1]<<8 & 0xFF00) | (data[0] & 0x00FF);

  return stateOfHealth;
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

  HAL_I2C_Mem_Read(bq35100Handle, BQ35100_ADDRESS, REG_DESIGN_CAPACITY, 1, data, 2, BQ35100_I2C_WAIT);

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

  HAL_I2C_Mem_Read(bq35100Handle, BQ35100_ADDRESS, REG_ACCUMULATED_CAPACITY, 1, data, 4, BQ35100_I2C_WAIT);

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
  uint32_t designCapacityUAh = (uint32_t) bq35100_getDesignCapacity() * 1000;

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
  uint8_t data[2];

  HAL_I2C_Mem_Read(bq35100Handle, BQ35100_ADDRESS, REG_CONTROL, 1, data, 2, BQ35100_I2C_WAIT);

  return data[1] & 0x60;
}

/**
 * @fn bool bq35100_getControlStatus(uint16_t*)
 * @brief functions reads the control status register.
 *
 * @param controlStatus
 * @return true if successful, otherwise false.
 */
bool bq35100_getControlStatus(uint8_t * controlStatus, uint8_t length)
{
  //read control status
  HAL_StatusTypeDef result = HAL_I2C_Mem_Read(bq35100Handle, BQ35100_ADDRESS, REG_CONTROL, 1, controlStatus, length, BQ35100_I2C_WAIT);

  if( result == HAL_OK)
    return true; //successful

  return false; //error
}

/**
 * @brief Check if the battery gauging is enabled or not.
 *
 * @return true if battery gauging is enabled, otherwise false.
 */
bool bq35100_isGaugeEnabled(void)
{
  uint8_t controlStatus[2];

  HAL_I2C_Mem_Read(bq35100Handle, BQ35100_ADDRESS, REG_CONTROL, 1, controlStatus, 2, BQ35100_I2C_WAIT);

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

  HAL_I2C_Master_Transmit(bq35100Handle, BQ35100_ADDRESS, data, 3, BQ35100_I2C_WAIT);

  for(uint8_t i=0; i<10; i++)
  {
    HAL_Delay(10);
    if(bq35100_isGaugeEnabled())
      return true;
  }

  return false;
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

  HAL_I2C_Master_Transmit(bq35100Handle, BQ35100_ADDRESS, data, 3, BQ35100_I2C_WAIT);

  // returns true to skip the check function to save time.
  if(ignoreCheck)
    return true;

  // Check if the device is done all its tasks and is turn off.
  for(uint8_t i=0; i<10; i++)
  {
    HAL_Delay(200);
    HAL_I2C_Mem_Read(bq35100Handle, BQ35100_ADDRESS, REG_CONTROL, 1, controlStatus, 2, BQ35100_I2C_WAIT);

    if((controlStatus[0] & 0x41) == 0x40)
      return !bq35100_isGaugeEnabled();
  }

  // returns false if the device is not turned of after 2 seconds
  return false;
}
