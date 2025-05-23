/**
  ******************************************************************************
  * @file           : SensorFunctions.c
  * @brief          : Header for SensorFunctions.c file.
  * @author         : P.Kwekkeboom
  * @date           : Nov 22, 2023
  ******************************************************************************
  */

#ifndef I2CMASTER_SENSORFUNCTIONS_H_
#define I2CMASTER_SENSORFUNCTIONS_H_

#include "I2C_Master.h"

typedef enum{
  SENSOR_MODULE_ID1 = 0,
  SENSOR_MODULE_ID2,
  SENSOR_MODULE_ID3,
  SENSOR_MODULE_ID4,
  SENSOR_MODULE_ID5,
  SENSOR_MODULE_ID6,
  MAX_SENSOR_MODULE
}ENUM_SensorModuleId;

typedef enum{
  MODULE_ADDRESS1 = 0x11<<1,
  MODULE_ADDRESS2 = 0x12<<1,
  MODULE_ADDRESS3 = 0x13<<1,
  MODULE_ADDRESS4 = 0x14<<1,
  MODULE_ADDRESS5 = 0x15<<1,
  MODULE_ADDRESS6 = 0x16<<1
}SensorAddress;

typedef enum{
  SENSOR_OK = I2C_TRANSFER_OK ,
  SENSOR_CRC_ERROR = I2C_CRC_ERROR,
  SENSOR_REGISTER_ERROR = I2C_REGISTER_ERROR,
  SENSOR_TIMEOUT = I2C_TIMEOUT,
  SENSOR_BUFFER_ERROR = I2C_BUFFER_ERROR,
  SENSOR_ID_ERROR,
}SensorError;

typedef enum{
  SLOT_GPIO_0 = 0x01,
  SLOT_GPIO_1 = 0x02,
  SLOT_GPIO_2 = 0x04,
  INTERRUPT = 0x08,
}ControlIO;

uint8_t sensorFirmwareVersion(int moduleId, uint8_t *firmwareVersion, uint16_t dataLength);
uint8_t sensorProtocolVersion(int moduleId, uint8_t * protocol);
uint8_t sensorReadType(int moduleId, uint16_t * type);
uint8_t sensorInitStart(int moduleId);
CommandStatus sensorInitStatus(int moduleId);
uint8_t sensorStartMeasurement(int moduleId);
CommandStatus sensorMeasurementStatus(int moduleId);
uint8_t sensorWriteSetupTime(int moduleId, uint16_t setupTime);
uint8_t sensorReadSetupTime(int moduleId, uint16_t * setupTime);
SensorError sensorReadMeasurement(int moduleId, uint8_t* measurementData, uint16_t dataLength);
uint8_t sensorReadAmount(int moduleId, uint8_t * numberOfSensors);
uint8_t sensorReadSelection(int moduleId, uint8_t * selectedSensor);
uint8_t sensorWriteSelection(int moduleId, uint8_t sensor);
uint8_t sensorWriteSampleType(int moduleId, uint8_t sampleType);
uint8_t sensorReadSampleType(int moduleId, uint8_t * sensorType);
uint8_t sensorReadSamples(int moduleId, uint8_t * numberOfSamples);
uint8_t sensorSetSamples(int moduleId, uint8_t samples);
SensorError sensorReadSelected(int moduleId, uint8_t* measurementData, uint16_t dataLength);
void sensorDirectionIO(int moduleId, uint8_t data);
void sensorWriteIO(int moduleId, uint8_t data);

#endif /* I2CMASTER_SENSORFUNCTIONS_H_ */
