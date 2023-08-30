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

void restoreLatestLogId(void);
void restoreLatestTimeFromLog(void);
int8_t writeNewLog( uint8_t sensorModuleType, uint8_t * sensorData, uint8_t dataLength );
uint32_t getLatestLogId(void);

#endif /* LOGGING_LOGGING_H_ */
