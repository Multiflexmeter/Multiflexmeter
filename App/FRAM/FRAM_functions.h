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

#define FRAM_USED_FOR_NVM_DATA //comment if no FRAM must be used for LoRa NVM data.

#define NR_SENSOR_MODULE 6

#define ADDRESS_OTHER_SETTINGS 0x0000
#define MAX_SIZE_OTHER_SETTINGS 0x0200
#define ADDRESS_LORA_SETTINGS 0x0200
#define MAX_SIZE_LORA_SETTINGS 0x0600
#define SIZE_FRAM  0x800

typedef struct __attribute__((packed))
{
    char version[10];
    char nullTerminator;
}struct_sensorModuleFirmwareVersion;

typedef struct __attribute__((packed))
{
    uint8_t sensorModuleInitRequest:1;
    uint8_t sensorModuleSpare:7;
}struct_sensorModuleSettings;

typedef union
{
    uint8_t byte;
    struct_sensorModuleSettings item;
} UNION_sensorModuleSettings;

typedef struct __attribute__((packed))
{
    uint8_t sensorModuleId;
    uint32_t nextIntervalBatteryEOS;
    struct_sensorModuleFirmwareVersion modules[NR_SENSOR_MODULE];
    uint8_t sensorModuleProtocol[NR_SENSOR_MODULE];
    uint8_t sensorModuleEnabled;
    UNION_sensorModuleSettings sensorModuleSettings[NR_SENSOR_MODULE];
}struct_FRAM_settings;

const void saveLoraSettings( const void *pSource, size_t length );
const void restoreLoraSettings( const void *pSource, size_t length);

const void saveFramSettings( const void *pSource, size_t length );
const void restoreFramSettings( const void *pSource, size_t length);

const int8_t testFram(uint8_t * status);

#endif /* FRAM_FRAM_FUNCTIONS_H_ */
