/**
  ******************************************************************************
  * @file           : logging.h
  * @brief          : Header for logging.c file.
  * @author         : P.Kwekkeboom
  * @date           : Aug 30, 2023
  * @copyright      : 2023 Dekimo Goes
  ******************************************************************************
  */

#ifndef LOGGING_LOGGING_H_
#define LOGGING_LOGGING_H_

#define MAX_SENSOR_DATASIZE 36

#define MAX_SIZE_LOGDATA  0x100 //max of 256 bytes, pagesize of flash

typedef struct __attribute__((packed))
{
  uint8_t sensorModuleSlotId;
  uint8_t sensorModuleTypeId;
  uint8_t sensorModuleProtocolId;
  uint8_t sensorModuleDataSize;
  uint8_t sensorModuleData[MAX_SENSOR_DATASIZE];
}struct_MFM_sensorModuleData;

typedef struct __attribute__((packed)){
  uint32_t measurementId;
  uint32_t timestamp;
  uint8_t protocolMFM;
  struct_MFM_sensorModuleData sensorModuleData;
  uint16_t sensorModuleData_crc;
  uint8_t spare[205];
}STRUCT_logdata;

typedef union{
  uint8_t logdata[MAX_SIZE_LOGDATA];
  STRUCT_logdata log;
}UNION_logdata;

typedef struct __attribute__((packed))
{
  uint8_t batteryStateEos;
}struct_MFM_baseData;

int8_t restoreLatestLogId(void);
int8_t restoreLatestTimeFromLog(void);
int8_t searchLatestLogInDataflash( uint32_t * logId );
int8_t writeNewLog( uint8_t MFM_protocol, struct_MFM_sensorModuleData * sensorModuleData, struct_MFM_baseData * MFM_data);
int8_t writeNewLog_old( uint8_t sensorModuleSlotId, uint8_t sensorModuleType, uint8_t protocol, uint8_t * sensorData, uint8_t dataLength );
int8_t readLog( uint32_t logId, uint8_t * buffer, uint32_t bufferLength );
uint32_t getLatestLogId(void);
uint32_t getOldestLogId(void);

#endif /* LOGGING_LOGGING_H_ */
