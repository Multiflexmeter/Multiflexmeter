/**
  ******************************************************************************
  * @addtogroup     : App
  * @{
  * @file           : SensorFunctions.c
  * @brief          : Sensor functions
  * @author         : P.Kwekkeboom
  * @date           : Nov 22, 2023
  * @}
  ******************************************************************************
  */

#include <string.h>

#include "SensorRegister.h"
#include "SensorFunctions.h"

/* cross reference from module ID to module address */
SensorAddress sensorId2Address[] =
{
    MODULE_ADDRESS1,
    MODULE_ADDRESS2,
    MODULE_ADDRESS3,
    MODULE_ADDRESS4,
    MODULE_ADDRESS5,
    MODULE_ADDRESS6
};

/**
 * @brief Get the firmware version of the sensor
 *
 * @param moduleId The sensor module id, value 0-5.
 */
void sensorFirmwareVersion(int moduleId, char* firmwareVersion, uint16_t dataLength)
{
  assert_param( moduleId >=  SENSOR_MODULE_ID1 && moduleId < MAX_SENSOR_MODULE );

  if( moduleId < SENSOR_MODULE_ID1 || moduleId >= MAX_SENSOR_MODULE )
  {
    memset(firmwareVersion, 0x00, dataLength); //force to 0x00.
    return;
  }

  sensorMasterRead(sensorId2Address[moduleId], REG_FIRMWARE_VERSION, (uint8_t*) firmwareVersion, dataLength);
}

/**
 * @brief Get the protocol version of the sensor
 *
 * @param moduleId The sensor module id, value 0-5.
 * @return The protocol version
 */
uint8_t sensorProtocolVersion(int moduleId)
{
  assert_param( moduleId >=  SENSOR_MODULE_ID1 && moduleId < MAX_SENSOR_MODULE );

  if( moduleId < SENSOR_MODULE_ID1 || moduleId >= MAX_SENSOR_MODULE )
    return -1;

  uint8_t protocolVersion;
  sensorMasterRead(sensorId2Address[moduleId], REG_PROTOCOL_VERSION, &protocolVersion, sizeof(protocolVersion));
  return protocolVersion;
}

/**
 * @brief Read the type of sensor connected
 *
 * @param moduleId The sensor module id, value 0-5.
 * @return The sensor type
 */
uint16_t sensorReadType(int moduleId)
{
  assert_param( moduleId >=  SENSOR_MODULE_ID1 && moduleId < MAX_SENSOR_MODULE );

  if( moduleId < SENSOR_MODULE_ID1 || moduleId >= MAX_SENSOR_MODULE )
    return -1;

  uint8_t sensorType[2];
  sensorMasterRead(sensorId2Address[moduleId], REG_SENSOR_TYPE, sensorType, sizeof(sensorType));
  return sensorType[0] + (sensorType[1]<<8);
}

/**
 * @brief Start the measurement
 *
 * @param moduleId The sensor module id, value 0-5.
 */
void sensorStartMeasurement(int moduleId)
{
  assert_param( moduleId >=  SENSOR_MODULE_ID1 && moduleId < MAX_SENSOR_MODULE );

  if( moduleId < SENSOR_MODULE_ID1 || moduleId >= MAX_SENSOR_MODULE )
    return;

  uint8_t startCommand = 0x01;
  sensorMasterWrite(sensorId2Address[moduleId], REG_MEAS_START, &startCommand);
}

/**
 * @brief Read the measurement status
 *
 * @param moduleId The sensor module id, value 0-5.
 * @return The measurement status
 */
MeasurementStatus sensorMeasurementStatus(int moduleId)
{
  assert_param( moduleId >=  SENSOR_MODULE_ID1 && moduleId < MAX_SENSOR_MODULE );

  if( moduleId < SENSOR_MODULE_ID1 || moduleId >= MAX_SENSOR_MODULE )
    return MEASUREMENT_ERROR;

  uint8_t sensorStatus;
  sensorMasterRead(sensorId2Address[moduleId], REG_MEAS_STATUS, &sensorStatus, sizeof(sensorStatus));
  return sensorStatus;
}

/**
 * @brief Write the time to wait after applying the power to start measuring
 *
 * @param moduleId The sensor module id, value 0-5.
 * @param setupTime The time to wait after applying power
 */
void sensorWriteSetupTime(int moduleId, uint16_t setupTime)
{
  assert_param( moduleId >=  SENSOR_MODULE_ID1 && moduleId < MAX_SENSOR_MODULE );

  if( moduleId < SENSOR_MODULE_ID1 || moduleId >= MAX_SENSOR_MODULE )
    return;

  uint8_t setupTimeBuffer[2] = {(setupTime), setupTime>>8};
  sensorMasterWrite(sensorId2Address[moduleId], REG_MEAS_TIME, setupTimeBuffer);
}

/**
 * @brief Read the setup time
 *
 * @param moduleId The sensor module id, value 0-5.
 * @return The setup time
 */
uint16_t sensorReadSetupTime(int moduleId)
{
  assert_param( moduleId >=  SENSOR_MODULE_ID1 && moduleId < MAX_SENSOR_MODULE );

  if( moduleId < SENSOR_MODULE_ID1 || moduleId >= MAX_SENSOR_MODULE )
    return -1;

  uint8_t setupTime[2];
  sensorMasterRead(sensorId2Address[moduleId], REG_MEAS_TIME, setupTime, sizeof(setupTime));
  return setupTime[0] + (setupTime[1]<<8);
}

/**
 * @brief Read the measurement data register form the sensor card
 *
 * @param moduleId The sensor module id, value 0-5.
 * @param measurementData The pointer to store the measurement data. Max 32 bytes.
 * @return Return I2C_OK when the function is successful.
 *         Else returns a specific error based on the type of fault.
 */
SensorError sensorReadMeasurement(int moduleId, uint8_t* measurementData, uint16_t dataLength)
{
  assert_param( moduleId >=  SENSOR_MODULE_ID1 && moduleId < MAX_SENSOR_MODULE );

  if( moduleId < SENSOR_MODULE_ID1 || moduleId >= MAX_SENSOR_MODULE )
  {
    memset(measurementData, 0x00, dataLength); //force to 0x00
    return SENSOR_ID_ERROR;
  }

  return sensorMasterReadVariableLength(sensorId2Address[moduleId], REG_MEAS_DATA, measurementData, dataLength);
}

/**
 * @brief Read the amount of sensors connected
 *
 * @param moduleId The sensor module id, value 0-5.
 * @return The amount of sensors connected
 */
uint8_t sensorReadAmount(int moduleId)
{
  assert_param( moduleId >=  SENSOR_MODULE_ID1 && moduleId < MAX_SENSOR_MODULE );

  if( moduleId < SENSOR_MODULE_ID1 || moduleId >= MAX_SENSOR_MODULE )
    return -1;

  uint8_t sensorAmount;
  sensorMasterRead(sensorId2Address[moduleId], REG_SENSOR_AMOUNT, &sensorAmount, sizeof(sensorAmount));
  return sensorAmount;
}

/**
 * @brief Read which sensor is selected
 *
 * @param moduleId The sensor module id, value 0-5.
 * @return The selected sensor
 */
uint8_t sensorReadSelection(int moduleId)
{
  assert_param( moduleId >=  SENSOR_MODULE_ID1 && moduleId < MAX_SENSOR_MODULE );

  if( moduleId < SENSOR_MODULE_ID1 || moduleId >= MAX_SENSOR_MODULE )
    return -1;

  uint8_t sensor;
  sensorMasterRead(sensorId2Address[moduleId], REG_SENSOR_SELECTED, &sensor, sizeof(sensor));
  return sensor;
}

/**
 * @brief Select which sensor to sample from
 *
 * @param moduleId The sensor module id, value 0-5.
 * @param sensor The sensor to select
 */
void sensorWriteSelection(int moduleId, uint8_t sensor)
{
  assert_param( moduleId >=  SENSOR_MODULE_ID1 && moduleId < MAX_SENSOR_MODULE );

  if( moduleId < SENSOR_MODULE_ID1 || moduleId >= MAX_SENSOR_MODULE )
    return;

  sensorMasterWrite(sensorId2Address[moduleId], REG_SENSOR_SELECTED, &sensor);
}

/**
 * @brief Set how to calculate the samples
 *
 * @param moduleId The sensor module id, value 0-5.
 * @param sampleType The type of sample
 */
void sensorWriteSampleType(int moduleId, uint8_t sampleType)
{
  assert_param( moduleId >=  SENSOR_MODULE_ID1 && moduleId < MAX_SENSOR_MODULE );

  if( moduleId < SENSOR_MODULE_ID1 || moduleId >= MAX_SENSOR_MODULE )
    return;

  sensorMasterWrite(sensorId2Address[moduleId], REG_MEAS_TYPE, &sampleType);
}

/**
 * @brief Read the sample type
 *
 * @param moduleId The sensor module id, value 0-5.
 * @return The sample type
 */
uint8_t sensorReadSampleType(int moduleId)
{
  assert_param( moduleId >=  SENSOR_MODULE_ID1 && moduleId < MAX_SENSOR_MODULE );

  if( moduleId < SENSOR_MODULE_ID1 || moduleId >= MAX_SENSOR_MODULE )
    return -1;

  uint8_t sampleType;
  sensorMasterRead(sensorId2Address[moduleId], REG_MEAS_TYPE, &sampleType, sizeof(sampleType));
  return sampleType;
}

/**
 * @brief Read the amount of samples the sensor card has to take for a measurement.
 *
 * @param moduleId The sensor module id, value 0-5.
 * @return The amount of samples
 */
uint8_t sensorReadSamples(int moduleId)
{
  assert_param( moduleId >=  SENSOR_MODULE_ID1 && moduleId < MAX_SENSOR_MODULE );

  if( moduleId < SENSOR_MODULE_ID1 || moduleId >= MAX_SENSOR_MODULE )
    return -1;

  uint8_t samples;
  sensorMasterRead(sensorId2Address[moduleId], REG_MEAS_SAMPLES, &samples, sizeof(samples));
  return samples;
}

/**
 * @brief Set the amount of samples to take for a measurement
 *
 * @param moduleId The sensor module id, value 0-5.
 * @param samples The amount of samples to take
 */
void sensorSetSamples(int moduleId, uint8_t samples)
{
  assert_param( moduleId >=  SENSOR_MODULE_ID1 && moduleId < MAX_SENSOR_MODULE );

  if( moduleId < SENSOR_MODULE_ID1 || moduleId >= MAX_SENSOR_MODULE )
    return;

  sensorMasterWrite(sensorId2Address[moduleId], REG_MEAS_SAMPLES, &samples);
}

/**
 * @fn SensorError sensorReadSelected(SensorAddress, uint8_t*, uint16_t)
 * @brief function to read selected sensor data
 *
 * @param moduleId The sensor module id, value 0-5.
 * @param measurementData
 * @param dataLength
 * @return
 */
SensorError sensorReadSelected(int moduleId, uint8_t* measurementData, uint16_t dataLength)
{
  assert_param( moduleId >=  SENSOR_MODULE_ID1 && moduleId < MAX_SENSOR_MODULE );

  if( moduleId < SENSOR_MODULE_ID1 || moduleId >= MAX_SENSOR_MODULE )
  {
    memset( measurementData, 0x00, dataLength); //force to 0x00
    return SENSOR_ID_ERROR;
  }

  return sensorMasterReadVariableLength(sensorId2Address[moduleId], REG_SENSOR_DATA, measurementData, dataLength);
}
