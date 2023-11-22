
#ifndef I2C_MASTER_H_
#define I2C_MASTER_H_

#include "main.h"
#include "SensorRegister.h"

/* Typedefs */
typedef enum{
  SENSOR_OK,
  SENSOR_CRC_ERROR,
  SENSOR_ADDR_ERROR,
  SENSOR_TIMEOUT,
  SENSOR_BUFFER_ERROR,
}SensorError;

SensorError sensorMasterRead(uint8_t slaveAddress, uint8_t regAddress, uint8_t *data, uint16_t dataLength);
SensorError sensorMasterWrite(uint8_t slaveAddress, uint8_t regAddress, uint8_t *data);
SensorError sensorMasterReadVariableLength(uint8_t slaveAddress, uint8_t regAddress, uint8_t* data, uint16_t dataLength);

#endif /* I2C_MASTER_H_ */
