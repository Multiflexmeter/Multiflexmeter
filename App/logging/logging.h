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

#define MAX_SIZE_LOGDATA  0x100 //max of 256 bytes, pagesize of flash

typedef struct __attribute__((packed)){
    uint32_t measurementId;
    uint32_t timestamp;
    uint8_t sensorModuleType;
    uint8_t sensorModuleDatasize;
    uint8_t sensorModuleData[40];
    uint16_t crc;
    uint8_t spare[204];
}STRUCT_logdata;

typedef union{
    uint8_t logdata[MAX_SIZE_LOGDATA];
    STRUCT_logdata log;
}UNION_logdata;

int8_t restoreLatestLogId(void);
int8_t restoreLatestTimeFromLog(void);
int8_t searchLatestLogInDataflash( uint32_t * logId );
int8_t writeNewLog( uint8_t sensorModuleType, uint8_t * sensorData, uint8_t dataLength );
int8_t readLog( uint32_t logId, uint8_t * buffer, uint32_t bufferLength );
uint32_t getLatestLogId(void);

#endif /* LOGGING_LOGGING_H_ */
