
#ifndef I2C_MASTER_H_
#define I2C_MASTER_H_

#include "main.h"
#include "SensorRegister.h"

/* Typedefs */
typedef enum{
  I2C_TRANSFER_OK,
  I2C_CRC_ERROR,
  I2C_REGISTER_ERROR,
  I2C_TIMEOUT,
  I2C_BUFFER_ERROR,
}ENUM_I2C_Error;

ENUM_I2C_Error sensorMasterRead(uint8_t slaveAddress, uint8_t regAddress, uint8_t *data, uint16_t dataLength);
ENUM_I2C_Error sensorMasterWrite(uint8_t slaveAddress, uint8_t regAddress, uint8_t *data);
ENUM_I2C_Error sensorMasterReadVariableLength(uint8_t slaveAddress, uint8_t regAddress, uint8_t* data, uint16_t dataLength);

#endif /* I2C_MASTER_H_ */
