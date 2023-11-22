
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
SensorError sensorMasterRead(uint8_t slaveAddress, uint8_t regAddress, uint8_t *data, uint16_t dataLength)
{
  // Determine the index of the register based on the register address
  int8_t regIndex = findRegIndex(regAddress);
  if(regIndex < 0)
    return SENSOR_ADDR_ERROR;

  // Determine the size of the register
  uint8_t regSize = registers[regIndex].datatype * registers[regIndex].size;

  // Request the data from the register
  uint8_t rxBuffer[regSize + CRC_SIZE];
  HAL_StatusTypeDef error = HAL_I2C_Mem_Read(&hi2c2, slaveAddress, regAddress, 1, rxBuffer, regSize + CRC_SIZE, 1000);
  if(error == HAL_TIMEOUT)
    return SENSOR_TIMEOUT;

  // Check the CRC of the incoming message
  if(calculateCRC_CCITT(rxBuffer, regSize + CRC_SIZE) != 0)
    return SENSOR_CRC_ERROR;

  // blank the destination buffer
  memcpy(data, 0x00, dataLength );

  // Copy the data to the provided memory location and remove the CRC from the data
  memcpy(data, rxBuffer, regSize > dataLength ? dataLength : regSize);

  return SENSOR_OK;
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
SensorError sensorMasterWrite(uint8_t slaveAddress, uint8_t regAddress, uint8_t *data)
{
  // Determine the index of the register based on the register address
  int8_t regIndex = findRegIndex(regAddress);
  if(regIndex < 0)
    return SENSOR_ADDR_ERROR;

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
    return SENSOR_TIMEOUT;

  return SENSOR_OK;
}

/**
 * @brief Get the firmware version of the sensor
 *
 * @param address The sensor address
 */
void sensorFirmwareVersion(SensorAddress address, char* firmwareVersion, uint16_t dataLength)
{
  sensorMasterRead(address, REG_FIRMWARE_VERSION, (uint8_t*) firmwareVersion, dataLength);
}

/**
 * @brief Get the protocol version of the sensor
 *
 * @param address The sensor address
 * @return The protocol version
 */
uint8_t sensorProtocolVersion(SensorAddress address, uint16_t dataLength)
{
  uint8_t protocolVersion;
  sensorMasterRead(address, REG_PROTOCOL_VERSION, &protocolVersion, dataLength);
  return protocolVersion;
}

/**
 * @brief Read the type of sensor connected
 *
 * @param address The sensor address
 * @return The sensor type
 */
uint16_t sensorReadType(SensorAddress address, uint16_t dataLength)
{
  uint8_t sensorType[2];
  sensorMasterRead(address, REG_SENSOR_TYPE, sensorType, dataLength);
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
MeasurementStatus sensorMeasurementStatus(SensorAddress address, uint16_t dataLength)
{
  uint8_t sensorStatus;
  sensorMasterRead(address, REG_MEAS_STATUS, &sensorStatus, dataLength);
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
uint16_t sensorReadSetupTime(SensorAddress address, uint16_t dataLength)
{
  uint8_t setupTime[2];
  sensorMasterRead(address, REG_MEAS_TIME, setupTime, dataLength);
  return setupTime[0] + (setupTime[1]<<8);
}

/**
 * @brief Read the measurement data register form the sensor card
 *
 * @param address The sensor address
 * @param measurementData The pointer to store the measurement data. Max 32 bytes.
 * @return Return I2C_OK when the function is successful.
 *         Else returns a specific error based on the type of fault.
 */
SensorError sensorReadMeasurement(SensorAddress address, uint8_t* measurementData, uint16_t dataLength)
{
  uint8_t regAddress = REG_MEAS_DATA;
  uint8_t rxBuffer[34];

  /* Select the measurement data register */
  HAL_I2C_Master_Transmit(&hi2c2, address, &regAddress, 1, 10);

  /* Receive the lenght of the measurement data */
  HAL_I2C_Master_Seq_Receive_IT(&hi2c2, address, rxBuffer, 1, I2C_FIRST_AND_NEXT_FRAME);
  while (HAL_I2C_GetState(&hi2c2) != HAL_I2C_STATE_READY);

  /* Limit the message length to 32 bytes */
  if(rxBuffer[0] > 32)
    rxBuffer[0] = 32;

  /* Receive the measurement data */
  HAL_I2C_Master_Seq_Receive_IT(&hi2c2, address, rxBuffer+1, rxBuffer[0] + CRC_SIZE, I2C_LAST_FRAME);
  while (HAL_I2C_GetState(&hi2c2) != HAL_I2C_STATE_READY);

  /* Check the CRC of the incoming message */
  if(calculateCRC_CCITT(rxBuffer, rxBuffer[0] + CRC_SIZE + 1) != 0)
    return SENSOR_CRC_ERROR;

  else
    memcpy(measurementData, rxBuffer+1, rxBuffer[0]);

  return SENSOR_OK;
}

/**
 * @brief Read the amount of sensors connected
 *
 * @param address The sensor address
 * @return The amount of sensors connected
 */
uint8_t sensorReadAmount(SensorAddress address, uint16_t dataLength)
{
  uint8_t sensorAmount;
  sensorMasterRead(address, REG_SENSOR_AMOUNT, &sensorAmount, dataLength);
  return sensorAmount;
}

/**
 * @brief Read which sensor is selected
 *
 * @param address The sensor address
 * @return The selected sensor
 */
uint8_t sensorReadSelection(SensorAddress address, uint16_t dataLength)
{
  uint8_t sensor;
  sensorMasterRead(address, REG_SENSOR_SELECTED, &sensor, dataLength);
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
uint8_t sensorReadSampleType(SensorAddress address, uint16_t dataLength)
{
  uint8_t sampleType;
  sensorMasterRead(address, REG_MEAS_TYPE, &sampleType, dataLength);
  return sampleType;
}

/**
 * @brief Read the amount of samples the sensor card has to take for a measurement.
 *
 * @param address The sensor address
 * @return The amount of samples
 */
uint8_t sensorReadSamples(SensorAddress address, uint16_t dataLength)
{
  uint8_t samples;
  sensorMasterRead(address, REG_MEAS_SAMPLES, &samples, dataLength);
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

/**
 * @fn SensorError sensorReadSelected(SensorAddress, uint8_t*, uint16_t)
 * @brief function to read selected sensor data
 *
 * @param address
 * @param measurementData
 * @param dataLength
 * @return
 */
SensorError sensorReadSelected(SensorAddress address, uint8_t* measurementData, uint16_t dataLength)
{
  uint8_t regAddress = REG_SENSOR_DATA;
  uint8_t rxBuffer[18];

  /* Select the measurement data register */
  HAL_I2C_Master_Transmit(&hi2c2, address, &regAddress, 1, 10);

  /* Receive the lenght of the measurement data */
  HAL_I2C_Master_Seq_Receive_IT(&hi2c2, address, rxBuffer, 1, I2C_FIRST_AND_NEXT_FRAME);
  while (HAL_I2C_GetState(&hi2c2) != HAL_I2C_STATE_READY);

  /* Limit the message length to 32 bytes */
  if(rxBuffer[0] > 16)
    rxBuffer[0] = 16;

  /* Receive the measurement data */
  HAL_I2C_Master_Seq_Receive_IT(&hi2c2, address, rxBuffer+1, rxBuffer[0] + CRC_SIZE, I2C_LAST_FRAME);
  while (HAL_I2C_GetState(&hi2c2) != HAL_I2C_STATE_READY);

  /* Check the CRC of the incoming message */
  if(calculateCRC_CCITT(rxBuffer, rxBuffer[0] + CRC_SIZE + 1) != 0)
    return SENSOR_CRC_ERROR;

  else
  {
    memset(measurementData, 0x00, dataLength ); //clear destination buffer
    memcpy(measurementData, rxBuffer+1, rxBuffer[0] > dataLength ? dataLength : rxBuffer[0]); //copy received data, make sure it fits in buffer
  }

  return SENSOR_OK;
}
