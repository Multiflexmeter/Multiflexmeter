
#include "I2C_Master.h"

#include <string.h>
#include "../common/crc16.h"
#include "SensorRegister.h"

#define CRC_SIZE 2

extern I2C_HandleTypeDef hi2c1;

/**
 * @brief Read a register on a sensor module
 * @param slaveAddress The i2c slave address of the sensor module
 * @param regAddress The address of the register that needs to be read
 * @param data Pointer to a buffer to store the result in.
 * @return Return I2C_OK when the function is successful. Returns a specific error based on the type of fault.
 */
tENUM_SensorError sensorMasterRead(uint8_t slaveAddress, uint8_t regAddress, uint8_t *data)
{
  // Determine the index of the register based on the register address
  int8_t regIndex = findRegIndex(regAddress);
  if(regIndex < 0)
    return I2C_ADDR_ERROR;

  // Determine the size of the register
  uint8_t regSize = registers[regIndex].datatype * registers[regIndex].size;

  // Request the data from the register
  uint8_t rxBuffer[12];
  HAL_StatusTypeDef error = HAL_I2C_Mem_Read(&hi2c1, slaveAddress, regAddress, 1, rxBuffer, regSize + CRC_SIZE, 1000);
  if(error == HAL_TIMEOUT)
    return I2C_TIMEOUT;

  // Check the CRC of the incoming message
  if(calculateCRC_CCITT(data + regSize, regSize + CRC_SIZE) != 0)
    return I2C_CRC_ERROR;

  // Copy the data to the provided memory location and remove the CRC from the data
  memcpy(data, rxBuffer, regSize);

  return I2C_OK;
}

/**
 * @brief Write data to a register on a sensor module
 * @param slaveAddress The i2c slave address of the sensor module
 * @param regAddress The address of the register to write
 * @param data The data to write
 * @return Return I2C_OK when the function is successful. Returns a specific error based on the type of fault.
 */
tENUM_SensorError sensorMasterWrite(uint8_t slaveAddress, uint8_t regAddress, uint8_t *data)
{
  // Determine the index of the register based on the register address
  int8_t regIndex = findRegIndex(regAddress);
  if(regIndex < 0)
    return I2C_ADDR_ERROR;

  // Determine the size of the register and the tx buffer
  uint8_t regSize = registers[regIndex].datatype * registers[regIndex].size;
  uint8_t txBuffer[sizeof(regAddress) + regSize + CRC_SIZE];

  // Copy the data into the tx buffer
  txBuffer[0] = regAddress;
  memcpy(txBuffer+1, data, regSize);

  // Calculate the CRC and add this to the tx buffer
  uint16_t crc = calculateCRC_CCITT(data, regSize+1);
  txBuffer[regSize+2] = crc & 0xFF;
  txBuffer[regSize+1] = (crc >> 8) & 0xFF;

  // Write data the the register
  HAL_StatusTypeDef error = HAL_I2C_Mem_Write(&hi2c1, slaveAddress, regAddress, 1, txBuffer, sizeof(regAddress) + regSize + CRC_SIZE, 1000);
  if(error == HAL_TIMEOUT)
    return I2C_TIMEOUT;

  return I2C_OK;
}
