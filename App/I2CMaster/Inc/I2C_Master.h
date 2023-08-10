
#ifndef I2C_MASTER_H_
#define I2C_MASTER_H_

#include "main.h"

/* Typedefs */
typedef enum{
  I2C_OK,
  I2C_CRC_ERROR,
  I2C_ADDR_ERROR,
  I2C_TIMEOUT
}tENUM_SensorError;

tENUM_SensorError sensorMasterRead(uint8_t slaveAddress, uint8_t regAddress, uint8_t *data);
tENUM_SensorError sensorMasterWrite(uint8_t slaveAddress, uint8_t regAddress, uint8_t *data);

#endif /* I2C_MASTER_H_ */
