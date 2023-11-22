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

void sensorFirmwareVersion(int moduleId, char *firmwareVersion, uint16_t dataLength);
uint8_t sensorProtocolVersion(int moduleId, uint16_t dataLength);
uint16_t sensorReadType(int moduleId, uint16_t dataLength);
void sensorStartMeasurement(int moduleId);
MeasurementStatus sensorMeasurementStatus(int moduleId, uint16_t dataLength);
void sensorWriteSetupTime(int moduleId, uint16_t setupTime);
uint16_t sensorReadSetupTime(int moduleId, uint16_t dataLength);
SensorError sensorReadMeasurement(int moduleId, uint8_t* measurementData, uint16_t dataLength);
uint8_t sensorReadSelection(int moduleId, uint16_t dataLength);
void sensorWriteSelection(int moduleId, uint8_t sensor);
void sensorSetSamples(int moduleId, uint8_t samples);
uint8_t sensorReadSamples(int moduleId, uint16_t dataLength);
SensorError sensorReadSelected(int moduleId, uint8_t* measurementData, uint16_t dataLength);

#endif /* I2CMASTER_SENSORFUNCTIONS_H_ */
