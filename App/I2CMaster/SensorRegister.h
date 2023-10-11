
#ifndef SENSORREGISTER_H_
#define SENSORREGISTER_H_

#include "stdint.h"

/* Sensor Registers */
#define REG_FIRMWARE_VERSION      0x01
#define REG_PROTOCOL_VERSION      0x02
#define REG_SENSOR_TYPE           0x03
#define REG_MEAS_START            0x10
#define REG_MEAS_STATUS           0x11
#define REG_MEAS_TIME             0x12
#define REG_MEAS_DATA             0x20
#define REG_SENSOR_AMOUNT         0x30
#define REG_SENSOR_SELECTED       0x31
#define REG_MEAS_TYPE             0x32
#define REG_MEAS_SAMPLES          0x33
#define REG_SENSOR_DATA           0x38
#define REG_ERROR_COUNT           0x50
#define REG_ERROR_STATUS          0x51


/* Typedefs */
typedef enum{
  UINT8_T = 1,
  UINT16_T = 2,
  INT32_T = 4
}tENUM_Datatype;

typedef enum{
  NO_ERROR,
  CRC_ERROR,
  ADDRESS_ERROR,
  WRITE_ERROR
}tENUM_Error;

typedef enum{
  READ,
  READWRITE
}tENUM_READWRITE;

typedef struct
{
  uint8_t adres;
  tENUM_Datatype datatype;
  uint8_t size;
  tENUM_READWRITE RW;
}SensorReg;

extern const SensorReg registers[];

/* Functions */
int8_t findRegIndex(uint8_t regAddress);

#endif /* SENSORREGISTER_H_ */
