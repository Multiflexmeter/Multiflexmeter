/**
  ******************************************************************************
  * @file           : FRAM_functions.h
  * @brief          : Header for FRAM_functions.c file.
  * @author         : P.Kwekkeboom
  * @date           : Oct 12, 2023
  ******************************************************************************
  */
#ifndef FRAM_FRAM_FUNCTIONS_H_
#define FRAM_FRAM_FUNCTIONS_H_

typedef struct __attribute__((packed))
{
    uint8_t sensorModuleId;
    uint32_t nextIntervalBatteryEOS;
}struct_FRAM_settings;

const void saveLoraSettings( const void *pSource, size_t length );
const void restoreLoraSettings( const void *pSource, size_t length);

const void saveFramSettings( const void *pSource, size_t length );
const void restoreFramSettings( const void *pSource, size_t length);


#endif /* FRAM_FRAM_FUNCTIONS_H_ */
