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

#define FRAM_SETTINGS_PROTOCOL_ID   0x00

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
    uint8_t lightSensorActive:1;
    uint8_t usbConnected:1;
    uint8_t batteryLow:1;
    uint32_t spare:29;
}struct_diagnosticStatusBits;

typedef union
{
    uint32_t byte[4];
    uint32_t uint32;
    struct_diagnosticStatusBits bit;
} UNION_diagnosticStatusBits;

typedef struct __attribute__((packed))
{
    uint16_t crc16;
    uint8_t protocolId;
    uint8_t currentSensorModuleIndex; // index of active sensor 0-5
    uint32_t nextIntervalBatteryEOS;
    struct_sensorModuleFirmwareVersion modules[NR_SENSOR_MODULE];
    uint8_t sensorModuleProtocol[NR_SENSOR_MODULE];
    uint8_t sensorModuleEnabled;
    UNION_sensorModuleSettings sensorModuleSettings[NR_SENSOR_MODULE];
    UNION_diagnosticStatusBits diagnosticBits; //32 bits reserved, only the first 8 bits are send to portal and saved in dataflash
}struct_FRAM_settings;

const void saveLoraSettings( const void *pSource, size_t length );
const void restoreLoraSettings( const void *pSource, size_t length);

const void saveFramSettingsStruct( struct_FRAM_settings *pSource, size_t length );
const void restoreFramSettingsStruct( const struct_FRAM_settings *pDest, size_t length);

const int8_t testFram(uint8_t * status);

#endif /* FRAM_FRAM_FUNCTIONS_H_ */
