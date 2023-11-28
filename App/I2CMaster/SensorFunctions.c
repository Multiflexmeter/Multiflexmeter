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
 * @fn uint8_t sensorFirmwareVersion(int, uint8_t*, uint16_t)
 * @brief Get the firmware version of the sensor
 *
 * @param moduleId The sensor module id, value 0-5.
 * @param firmwareVersion : result of firmware version string
 * @param dataLength : length of buffer
 * @return result of read action \ref SensorError
 */
uint8_t sensorFirmwareVersion(int moduleId, uint8_t* firmwareVersion, uint16_t dataLength)
{
  assert_param( moduleId >=  SENSOR_MODULE_ID1 && moduleId < MAX_SENSOR_MODULE );

  if( moduleId < SENSOR_MODULE_ID1 || moduleId >= MAX_SENSOR_MODULE )
  {
    memset(firmwareVersion, 0x00, dataLength); //force to 0x00.
    return SENSOR_ID_ERROR;
  }

  return sensorMasterRead(sensorId2Address[moduleId], REG_FIRMWARE_VERSION, (uint8_t*) firmwareVersion, dataLength);
}

/**
 * @fn uint8_t sensorProtocolVersion(int, uint8_t*)
 * @brief Get the protocol version of the sensor
 *
 * @param moduleId The sensor module id, value 0-5.
 * @param protocol :  The protocol version
 * @return result of read action \ref SensorError
 */
uint8_t sensorProtocolVersion(int moduleId, uint8_t * protocol)
{
  assert_param( moduleId >=  SENSOR_MODULE_ID1 && moduleId < MAX_SENSOR_MODULE );

  if( moduleId < SENSOR_MODULE_ID1 || moduleId >= MAX_SENSOR_MODULE )
    return SENSOR_ID_ERROR;

  uint8_t protocolVersion;
  ENUM_I2C_Error result = sensorMasterRead(sensorId2Address[moduleId], REG_PROTOCOL_VERSION, &protocolVersion, sizeof(protocolVersion));

  if( I2C_TRANSFER_OK == result)
  {
    *protocol = protocolVersion;
  }

  return result;
}

/**
 * @fn uint8_t sensorReadType(int, uint16_t*)
 * @brief Read the type of sensor connected
 *
 * @param moduleId The sensor module id, value 0-5.
 * @param type : The sensor type
 * @return result of read action \ref SensorError
 */
uint8_t sensorReadType(int moduleId, uint16_t * type)
{
  assert_param( moduleId >=  SENSOR_MODULE_ID1 && moduleId < MAX_SENSOR_MODULE );

  if( moduleId < SENSOR_MODULE_ID1 || moduleId >= MAX_SENSOR_MODULE )
    return SENSOR_ID_ERROR;

  uint8_t sensorType[2];
  ENUM_I2C_Error result = sensorMasterRead(sensorId2Address[moduleId], REG_SENSOR_TYPE, sensorType, sizeof(sensorType));

  if( result == I2C_TRANSFER_OK)
  {
    *type = sensorType[0] + (sensorType[1]<<8);
  }

  return result;
}

/**
 * @fn uint8_t sensorStartMeasurement(int)
 * @brief Start the measurement
 *
 * @param moduleId The sensor module id, value 0-5.
 * @return result of read action \ref SensorError
 */
uint8_t sensorStartMeasurement(int moduleId)
{
  assert_param( moduleId >=  SENSOR_MODULE_ID1 && moduleId < MAX_SENSOR_MODULE );

  if( moduleId < SENSOR_MODULE_ID1 || moduleId >= MAX_SENSOR_MODULE )
    return SENSOR_ID_ERROR;

  uint8_t startCommand = 0x01;
  return sensorMasterWrite(sensorId2Address[moduleId], REG_MEAS_START, &startCommand);
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
 * @fn uint8_t sensorWriteSetupTime(int, uint16_t)
 * @brief Write the time to wait after applying the power to start measuring
 *
 * @param moduleId The sensor module id, value 0-5.
 * @param setupTime The time to wait after applying power
 * @return result of read action \ref SensorError
 */
uint8_t sensorWriteSetupTime(int moduleId, uint16_t setupTime)
{
  assert_param( moduleId >=  SENSOR_MODULE_ID1 && moduleId < MAX_SENSOR_MODULE );

  if( moduleId < SENSOR_MODULE_ID1 || moduleId >= MAX_SENSOR_MODULE )
    return SENSOR_ID_ERROR;

  uint8_t setupTimeBuffer[2] = {(setupTime), setupTime>>8};
  return sensorMasterWrite(sensorId2Address[moduleId], REG_MEAS_TIME, setupTimeBuffer);
}

/**
 * @fn uint8_t sensorReadSetupTime(int, uint16_t*)
 * @brief Read the setup time
 *
 * @param moduleId The sensor module id, value 0-5.
 * @param setupTime : the setup time
 * @return result of read action \ref SensorError
 */
uint8_t sensorReadSetupTime(int moduleId, uint16_t * setupTime )
{
  assert_param( moduleId >=  SENSOR_MODULE_ID1 && moduleId < MAX_SENSOR_MODULE );

  if( moduleId < SENSOR_MODULE_ID1 || moduleId >= MAX_SENSOR_MODULE )
    return SENSOR_ID_ERROR;

  uint8_t time[2];
  ENUM_I2C_Error result = sensorMasterRead(sensorId2Address[moduleId], REG_MEAS_TIME, time, sizeof(time));

  if( result == I2C_TRANSFER_OK)
  {
    *setupTime = time[0] + (time[1]<<8);
  }

  return result;
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
 * @fn uint8_t sensorReadAmount(int, uint8_t*)
 * @brief Read the amount of sensors connected
 *
 * @param moduleId The sensor module id, value 0-5.
 * @param numberOfSensors : The amount of sensors connected
 * @return result of read action \ref SensorError
 */
uint8_t sensorReadAmount(int moduleId, uint8_t * numberOfSensors)
{
  assert_param( moduleId >=  SENSOR_MODULE_ID1 && moduleId < MAX_SENSOR_MODULE );

  if( moduleId < SENSOR_MODULE_ID1 || moduleId >= MAX_SENSOR_MODULE )
    return SENSOR_ID_ERROR;

  uint8_t sensorAmount;
  ENUM_I2C_Error result = sensorMasterRead(sensorId2Address[moduleId], REG_SENSOR_AMOUNT, &sensorAmount, sizeof(sensorAmount));

  if( result == I2C_TRANSFER_OK)
  {
    *numberOfSensors = sensorAmount;
  }

  return result;
}

/**
 * @fn uint8_t sensorReadSelection(int, uint8_t*)
 * @brief Read which sensor is selected
 *
 * @param moduleId The sensor module id, value 0-5.
 * @param selectedSensor : The selected sensor
 * @return result of read action \ref SensorError
 */
uint8_t sensorReadSelection(int moduleId, uint8_t * selectedSensor)
{
  assert_param( moduleId >=  SENSOR_MODULE_ID1 && moduleId < MAX_SENSOR_MODULE );

  if( moduleId < SENSOR_MODULE_ID1 || moduleId >= MAX_SENSOR_MODULE )
    return SENSOR_ID_ERROR;

  uint8_t sensor;
  ENUM_I2C_Error result = sensorMasterRead(sensorId2Address[moduleId], REG_SENSOR_SELECTED, &sensor, sizeof(sensor));

  if( result == I2C_TRANSFER_OK)
  {
    *selectedSensor = sensor;
  }

  return result;
}

/**
 * @fn uint8_t sensorWriteSelection(int, uint8_t)
 * @brief Select which sensor to sample from
 *
 * @param moduleId The sensor module id, value 0-5.
 * @param sensor The sensor to select
 * @return result of read action \ref SensorError
 */
uint8_t sensorWriteSelection(int moduleId, uint8_t sensor)
{
  assert_param( moduleId >=  SENSOR_MODULE_ID1 && moduleId < MAX_SENSOR_MODULE );

  if( moduleId < SENSOR_MODULE_ID1 || moduleId >= MAX_SENSOR_MODULE )
    return SENSOR_ID_ERROR;

  return sensorMasterWrite(sensorId2Address[moduleId], REG_SENSOR_SELECTED, &sensor);
}

/**
 * @fn uint8_t sensorWriteSampleType(int, uint8_t)
 * @brief Set how to calculate the samples
 *
 * @param moduleId The sensor module id, value 0-5.
 * @param sampleType The type of sample
 * @return result of read action \ref SensorError
 */
uint8_t sensorWriteSampleType(int moduleId, uint8_t sampleType)
{
  assert_param( moduleId >=  SENSOR_MODULE_ID1 && moduleId < MAX_SENSOR_MODULE );

  if( moduleId < SENSOR_MODULE_ID1 || moduleId >= MAX_SENSOR_MODULE )
    return SENSOR_ID_ERROR;

  return sensorMasterWrite(sensorId2Address[moduleId], REG_MEAS_TYPE, &sampleType);
}

/**
 * @fn uint8_t sensorReadSampleType(int, uint8_t*)
 * @brief Read the sample type
 *
 * @param moduleId The sensor module id, value 0-5.
 * @param sensorType : The sample type
 * @return result of read action \ref SensorError
 */
uint8_t sensorReadSampleType(int moduleId, uint8_t * sensorType)
{
  assert_param( moduleId >=  SENSOR_MODULE_ID1 && moduleId < MAX_SENSOR_MODULE );

  if( moduleId < SENSOR_MODULE_ID1 || moduleId >= MAX_SENSOR_MODULE )
    return SENSOR_ID_ERROR;

  uint8_t sampleType;
  ENUM_I2C_Error result = sensorMasterRead(sensorId2Address[moduleId], REG_MEAS_TYPE, &sampleType, sizeof(sampleType));

  if( result == I2C_TRANSFER_OK)
  {
    *sensorType = sampleType;
  }

  return result;

}

/**
 * @fn uint8_t sensorReadSamples(int, uint8_t*)
 * @brief Read the amount of samples the sensor card has to take for a measurement.
 *
 * @param moduleId The sensor module id, value 0-5.
 * @param numberOfSamples : The amount of samples
 * @return result of read action \ref SensorError
 */
uint8_t sensorReadSamples(int moduleId, uint8_t * numberOfSamples)
{
  assert_param( moduleId >=  SENSOR_MODULE_ID1 && moduleId < MAX_SENSOR_MODULE );

  if( moduleId < SENSOR_MODULE_ID1 || moduleId >= MAX_SENSOR_MODULE )
    return SENSOR_ID_ERROR;

  uint8_t samples;
  ENUM_I2C_Error result = sensorMasterRead(sensorId2Address[moduleId], REG_MEAS_SAMPLES, &samples, sizeof(samples));

  if( result == I2C_TRANSFER_OK)
  {
    *numberOfSamples = samples;
  }

  return result;
}

/**
 * @fn uint8_t sensorSetSamples(int, uint8_t)
 * @brief Set the amount of samples to take for a measurement
 *
 * @param moduleId The sensor module id, value 0-5.
 * @param samples The amount of samples to take
 * @return result of read action \ref SensorError
 */
uint8_t sensorSetSamples(int moduleId, uint8_t samples)
{
  assert_param( moduleId >=  SENSOR_MODULE_ID1 && moduleId < MAX_SENSOR_MODULE );

  if( moduleId < SENSOR_MODULE_ID1 || moduleId >= MAX_SENSOR_MODULE )
    return SENSOR_ID_ERROR;

  return sensorMasterWrite(sensorId2Address[moduleId], REG_MEAS_SAMPLES, &samples);
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
