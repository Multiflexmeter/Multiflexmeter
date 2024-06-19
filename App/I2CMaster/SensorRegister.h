
#ifndef SENSORREGISTER_H_
#define SENSORREGISTER_H_

#include "stdint.h"

/* Sensor Registers */
#define REG_FIRMWARE_VERSION      0x01
#define REG_PROTOCOL_VERSION      0x02
#define REG_SENSOR_TYPE           0x03
#define REG_INIT_START            0x0A
#define REG_INIT_STATUS           0x0B
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


/**
 * @enum tENUM_Datatype
 * @brief sensor I2C communication data type
 *
 */
typedef enum{
  UINT8_T = 1,  /**< UINT8_T */
  UINT16_T = 2, /**< UINT16_T */
  SENSORDATA = 8/**< SENSORDATA */
}tENUM_Datatype;

/**
 * @enum  tENUM_Error
 * @brief sensor I2C communication error type
 *
 */
typedef enum{
  NO_ERROR,     /**< NO_ERROR */
  CRC_ERROR,    /**< CRC_ERROR */
  ADDRESS_ERROR,/**< ADDRESS_ERROR */
  WRITE_ERROR   /**< WRITE_ERROR */
}tENUM_Error;

/**
 * @enum tENUM_READWRITE
 * @brief sensor I2C communication read/write setting
 *
 */
typedef enum{
  READ,    /**< READ */
  READWRITE/**< READWRITE */
}tENUM_READWRITE;

/**
 * @enum CommandStatus
 * @brief sensor I2C communication command status values
 *
 */
typedef enum{
  NO_ACTIVE_COMMAND = 0x00,   /**< NO_ACTIVE_COMMAND */
  COMMNAND_ACTIVE = 0x01,     /**< COMMNAND_ACTIVE */
  COMMAND_DONE = 0x0A,        /**< COMMAND_DONE */
  COMMAND_FAILED = 0x0F,      /**< COMMAND_FAILED */
  COMMAND_ERROR = 0xF0,       /**< COMMAND_ERROR */
  COMMAND_NOTAVAILABLE = 0xFF,/**< COMMAND_NOTAVAILABLE */
}CommandStatus;

/**
 * @enum ENUM_MFM_MODULE
 * @brief Definition of MFM modules, only for debug purposes
 *
 */
typedef enum{
  MFM_NOT_USED = 0x00,                             /**< MFM_NOT_USED */
  MFM_PREASURE_RS485,  /**< MFM_PREASURE_RS485 */  /**< MFM_PREASURE_RS485 */
  MFM_PREASURE_ONEWIRE,/**< MFM_PREASURE_ONEWIRE *//**< MFM_PREASURE_ONEWIRE */
}ENUM_MFM_MODULE;

/**
 * @struct SensorReg
 * @brief sensor I2C communication register definition
 *
 */
typedef struct
{
  uint8_t adres;
  tENUM_Datatype datatype;
  uint8_t size;
  tENUM_READWRITE RW;
}SensorReg;

/**
 * @struct structDataPressureSensor
 * @brief definition of data package of MFM pressure sensor, type RS485, Keller, only for debug purposes
 *
 */
typedef struct __attribute__((packed))
{
  uint8_t dataLength;
  float pressure1;
  float temperature1;
  float pressure2;
  float temperature2;
  uint16_t crc;
} structDataPressureSensor;

/**
 * @struct structDataPressureSensorOneWire
 * @brief definition of data package of MFM pressure sensor, type ONE WIRE, HUBA, only for debug purposes
 *
 */
typedef struct __attribute__((packed))
{
  uint8_t dataLength;
  uint16_t pressure1;
  uint8_t temperature1;
  uint16_t pressure2;
  uint8_t temperature2;
  uint16_t crc;
} structDataPressureSensorOneWire;

extern const SensorReg registers[];

/* Functions */
int8_t findRegIndex(uint8_t regAddress);

#endif /* SENSORREGISTER_H_ */
