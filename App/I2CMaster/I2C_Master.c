
#include "I2C_Master.h"

#include <string.h>
#include "../common/crc16.h"
#include "SensorRegister.h"

#define CRC_SIZE 2

extern I2C_HandleTypeDef hi2c2;

/**
 * @brief Read a register on a sensor module
 *
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
  uint8_t rxBuffer[regSize + CRC_SIZE];
  HAL_StatusTypeDef error = HAL_I2C_Mem_Read(&hi2c2, slaveAddress, regAddress, 1, rxBuffer, regSize + CRC_SIZE, 1000);
  if(error == HAL_TIMEOUT)
    return I2C_TIMEOUT;

  // Check the CRC of the incoming message
  if(calculateCRC_CCITT(rxBuffer, regSize + CRC_SIZE) != 0)
    return I2C_CRC_ERROR;

  // Copy the data to the provided memory location and remove the CRC from the data
  memcpy(data, rxBuffer, regSize);

  return I2C_OK;
}

/**
 * @brief Write data to a register on a sensor module
 *
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
  memcpy(&txBuffer[1], data, regSize);

  // Calculate the CRC and add this to the tx buffer
  uint16_t crc = calculateCRC_CCITT(txBuffer, regSize+1);
  txBuffer[regSize+2] = crc & 0xFF;
  txBuffer[regSize+1] = (crc >> 8) & 0xFF;

  // Write data the the register
  HAL_StatusTypeDef error = HAL_I2C_Mem_Write(&hi2c2, slaveAddress, regAddress, 1, &txBuffer[1], regSize + CRC_SIZE, 1000);
  if(error == HAL_TIMEOUT)
    return I2C_TIMEOUT;

  return I2C_OK;
}

/**
 * @brief Get the firmware version of the sensor
 *
 * @param address is the sensor address
 */
void sensorFirmwareVersion(SensorAddress address)
{
  uint8_t rxBuffer[10];
  sensorMasterRead(address, REG_FIRMWARE_VERSION, rxBuffer);
}

/**
 * @brief Get the protocol version of the sensor
 *
 * @param address is the sensor address
 * @return The protocol version
 */
uint8_t sensorProtocolVersion(SensorAddress address)
{
  uint8_t protocolVersion;
  sensorMasterRead(address, REG_PROTOCOL_VERSION, &protocolVersion);
  return protocolVersion;
}

uint16_t sensorReadType(SensorAddress address)
{
  uint8_t sensorType[2];
  sensorMasterRead(address, REG_SENSOR_TYPE, sensorType);
  return sensorType[0] + (sensorType[1]<<8);
}

void sensorStartMeasurement(SensorAddress address)
{
  uint8_t startCommand = 0x01;
  sensorMasterWrite(address, REG_MEAS_START, &startCommand);
}

uint8_t sensorMeasurementStatus(SensorAddress address)
{
  uint8_t sensorStatus;
  sensorMasterRead(address, REG_MEAS_STATUS, &sensorStatus);
  return sensorStatus;
}

void sensorWriteSetupTime(SensorAddress address, uint16_t setupTime)
{
  uint8_t setupTimeBuffer[2] = {(setupTime), setupTime>>8};
  sensorMasterWrite(address, REG_MEAS_TIME, setupTimeBuffer);
}

uint16_t sensorReadSetupTime(SensorAddress address)
{
  uint8_t setupTime[2];
  sensorMasterRead(address, REG_MEAS_TIME, setupTime);
  return setupTime[0] + (setupTime[1]<<8);
}

void sensorReadMeasurement(SensorAddress address)
{
  //TODO implement sensor type function
}

void sensorAmount(SensorAddress address)
{
  //TODO implement sensor type function
}

void sensorSelect(SensorAddress address)
{
  //TODO implement sensor type function
}

void sensorSampleType(SensorAddress address)
{
  //TODO implement sensor type function
}

/**
 * @brief Read the amount of samples the sensor card has to take for a measurement.
 *
 * @param address is the address of the sensor
 * @return the amount of samples
 */
uint8_t sensorReadSamples(SensorAddress address)
{
  uint8_t samples;
  sensorMasterRead(address, REG_MEAS_SAMPLES, &samples);
  return samples;
}

void sensorSetSamples(SensorAddress address, uint8_t samples)
{
  sensorMasterWrite(address, REG_MEAS_SAMPLES, &samples);
}
