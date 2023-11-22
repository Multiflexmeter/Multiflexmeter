
#ifndef I2C_MASTER_H_
#define I2C_MASTER_H_

#include "main.h"
#include "SensorRegister.h"

/* Typedefs */
typedef enum{
  SENSOR_OK,
  SENSOR_CRC_ERROR,
  SENSOR_ADDR_ERROR,
  SENSOR_TIMEOUT
}SensorError;

typedef enum{
  SENSOR_MODULE_1 = 0x11<<1,
  SENSOR_MODULE_2 = 0x12<<1,
  SENSOR_MODULE_3 = 0x13<<1,
  SENSOR_MODULE_4 = 0x14<<1,
  SENSOR_MODULE_5 = 0x15<<1,
  SENSOR_MODULE_6 = 0x16<<1
}SensorAddress;

void sensorFirmwareVersion(SensorAddress address, char *firmwareVersion, uint16_t dataLength);
uint8_t sensorProtocolVersion(SensorAddress address, uint16_t dataLength);
uint16_t sensorReadType(SensorAddress address, uint16_t dataLength);
void sensorStartMeasurement(SensorAddress address);
MeasurementStatus sensorMeasurementStatus(SensorAddress address, uint16_t dataLength);
void sensorWriteSetupTime(SensorAddress address, uint16_t setupTime);
uint16_t sensorReadSetupTime(SensorAddress address, uint16_t dataLength);
SensorError sensorReadMeasurement(SensorAddress address, uint8_t* measurementData, uint16_t dataLength);
uint8_t sensorReadSelection(SensorAddress address, uint16_t dataLength);
void sensorWriteSelection(SensorAddress address, uint8_t sensor);
void sensorSetSamples(SensorAddress address, uint8_t samples);
uint8_t sensorReadSamples(SensorAddress address, uint16_t dataLength);
SensorError sensorReadSelected(SensorAddress address, uint8_t* measurementData, uint16_t dataLength);

#endif /* I2C_MASTER_H_ */
