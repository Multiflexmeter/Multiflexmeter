/**
  ******************************************************************************
  * @file           : measurement.h
  * @brief          : Header for measurement.c file.
  * @author         : P.Kwekkeboom
  * @date           : Aug 30, 2023
  * @copyright      : 2023 Dekimo Goes
  ******************************************************************************
  */

#ifndef MEASUREMENT_MEASUREMENT_H_
#define MEASUREMENT_MEASUREMENT_H_

#define MAX_SENSOR_DATASIZE     36
#define MAX_SENSOR_MODULE_DATA (MAX_SENSOR_DATASIZE + 4 ) //36 databytes + 4 bytes header
#define MAX_BASE_MODULE_DATA    9 // + 1 byte protocol type

#define MAX_SIZE_LOGDATA  0x100 //max of 256 bytes, pagesize of flash


typedef struct __attribute__((packed))
{
  uint8_t sensorModuleSlotId;
  uint8_t sensorModuleTypeId;
  uint8_t sensorModuleProtocolId;
  uint8_t sensorModuleDataSize;
  uint8_t sensorModuleData[MAX_SENSOR_DATASIZE];
}struct_MFM_sensorModuleData;

typedef struct __attribute__((packed))
{
  uint8_t batteryStateEos;
  uint8_t spare[8];
}struct_MFM_baseData;

typedef struct __attribute__((packed)){
  uint32_t measurementId;
  uint32_t timestamp;
  uint8_t protocolMFM;
  struct_MFM_sensorModuleData sensorModuleData;
  uint16_t sensorModuleData_crc;
  struct_MFM_baseData MFM_baseData;
  uint8_t spare[196];
}STRUCT_measurementData;

typedef union{
  uint8_t logdata[MAX_SIZE_LOGDATA];
  STRUCT_measurementData measurementData;
}UNION_logdata;

int8_t restoreLatestMeasurementId(void);
int8_t restoreLatestTimeFromMeasurement(void);
int8_t searchLatestMeasurementInDataflash( uint32_t * logId );
int8_t writeNewMeasurementToDataflash( uint8_t MFM_protocol, struct_MFM_sensorModuleData * sensorModuleData, struct_MFM_baseData * MFM_data);
int8_t readMeasurementFromDataflash( uint32_t logId, uint8_t * buffer, uint32_t bufferLength );
uint32_t getLatestMeasurementId(void);
uint32_t getOldestMeasurementId(void);

#endif /* LOGGING_LOGGING_H_ */
