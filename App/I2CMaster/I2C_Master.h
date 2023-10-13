
#ifndef I2C_MASTER_H_
#define I2C_MASTER_H_

#include "main.h"
#include "SensorRegister.h"

/* Typedefs */
typedef enum{
  I2C_OK,
  I2C_CRC_ERROR,
  I2C_ADDR_ERROR,
  I2C_TIMEOUT
}tENUM_SensorError;

typedef enum{
  SENSOR_MODULE_1 = 0x11<<1,
  SENSOR_MODULE_2 = 0x12<<1,
  SENSOR_MODULE_3 = 0x13<<1,
  SENSOR_MODULE_4 = 0x14<<1,
  SENSOR_MODULE_5 = 0x15<<1,
  SENSOR_MODULE_6 = 0x16<<1
}SensorAddress;

void sensorFirmwareVersion(SensorAddress address, char *firmwareVersion);
uint8_t sensorProtocolVersion(SensorAddress address);
uint16_t sensorReadType(SensorAddress address);
void sensorStartMeasurement(SensorAddress address);
uint8_t sensorMeasurementStatus(SensorAddress address);
void sensorWriteSetupTime(SensorAddress address, uint16_t setupTime);
uint16_t sensorReadSetupTime(SensorAddress address);
int32_t sensorReadMeasurement(SensorAddress address);
void sensorSetSamples(SensorAddress address, uint8_t samples);
uint8_t sensorReadSamples(SensorAddress address);

#endif /* I2C_MASTER_H_ */
