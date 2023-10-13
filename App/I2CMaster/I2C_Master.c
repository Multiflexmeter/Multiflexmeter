
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
 * @return Return I2C_OK when the function is successful.
 *         Else returns a specific error based on the type of fault.
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
 * @return Return I2C_OK when the function is successful.
 *         Else returns a specific error based on the type of fault.
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
 * @param address The sensor address
 */
void sensorFirmwareVersion(SensorAddress address, char* firmwareVersion)
{
  sensorMasterRead(address, REG_FIRMWARE_VERSION, (uint8_t*) firmwareVersion);
}

/**
 * @brief Get the protocol version of the sensor
 *
 * @param address The sensor address
 * @return The protocol version
 */
uint8_t sensorProtocolVersion(SensorAddress address)
{
  uint8_t protocolVersion;
  sensorMasterRead(address, REG_PROTOCOL_VERSION, &protocolVersion);
  return protocolVersion;
}

/**
 * @brief Read the type of sensor connected
 *
 * @param address The sensor address
 * @return The sensor type
 */
uint16_t sensorReadType(SensorAddress address)
{
  uint8_t sensorType[2];
  sensorMasterRead(address, REG_SENSOR_TYPE, sensorType);
  return sensorType[0] + (sensorType[1]<<8);
}

/**
 * @brief Start the measurement
 *
 * @param address The sensor address
 */
void sensorStartMeasurement(SensorAddress address)
{
  uint8_t startCommand = 0x01;
  sensorMasterWrite(address, REG_MEAS_START, &startCommand);
}

/**
 * @brief Read the measurement status
 *
 * @param address The sensor address
 * @return The measurement status
 */
uint8_t sensorMeasurementStatus(SensorAddress address)
{
  uint8_t sensorStatus;
  sensorMasterRead(address, REG_MEAS_STATUS, &sensorStatus);
  return sensorStatus;
}

/**
 * @brief Write the time to wait after applying the power to start measuring
 *
 * @param address The sensor address
 * @param setupTime The time to wait after applying power
 */
void sensorWriteSetupTime(SensorAddress address, uint16_t setupTime)
{
  uint8_t setupTimeBuffer[2] = {(setupTime), setupTime>>8};
  sensorMasterWrite(address, REG_MEAS_TIME, setupTimeBuffer);
}

/**
 * @brief Read the setup time
 *
 * @param address The sensor address
 * @return The setup time
 */
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

/**
 * @brief Read the amount of sensors connected
 *
 * @param address The sensor address
 * @return The amount of sensors connected
 */
uint8_t sensorReadAmount(SensorAddress address)
{
  uint8_t sensorAmount;
  sensorMasterRead(address, REG_SENSOR_AMOUNT, &sensorAmount);
  return sensorAmount;
}

/**
 * @brief Read which sensor is selected
 *
 * @param address The sensor address
 * @return The selected sensor
 */
uint8_t sensorReadSelection(SensorAddress address)
{
  uint8_t sensor;
  sensorMasterRead(address, REG_SENSOR_SELECTED, &sensor);
  return sensor;
}

/**
 * @brief Select which sensor to sample from
 *
 * @param address The sensor address
 * @param sensor The sensor to select
 */
void sensorWriteSelection(SensorAddress address, uint8_t sensor)
{
  sensorMasterWrite(address, REG_SENSOR_SELECTED, &sensor);
}

/**
 * @brief Set how to calculate the samples
 *
 * @param address The sensor address
 * @param sampleType The type of sample
 */
void sensorWriteSampleType(SensorAddress address, uint8_t sampleType)
{
  sensorMasterWrite(address, REG_MEAS_TYPE, &sampleType);
}

/**
 * @brief Read the sample type
 *
 * @param address The sensor address
 * @return The sample type
 */
uint8_t sensorReadSampleType(SensorAddress address)
{
  uint8_t sampleType;
  sensorMasterRead(address, REG_MEAS_TYPE, &sampleType);
  return sampleType;
}

/**
 * @brief Read the amount of samples the sensor card has to take for a measurement.
 *
 * @param address The sensor address
 * @return The amount of samples
 */
uint8_t sensorReadSamples(SensorAddress address)
{
  uint8_t samples;
  sensorMasterRead(address, REG_MEAS_SAMPLES, &samples);
  return samples;
}

/**
 * @brief Set the amount of samples to take for a measurement
 *
 * @param address The sensor address
 * @param samples The amount of samples to take
 */
void sensorSetSamples(SensorAddress address, uint8_t samples)
{
  sensorMasterWrite(address, REG_MEAS_SAMPLES, &samples);
}
