/**
  ******************************************************************************
  * @file           : MFMconfiguration.h
  * @brief          : Header for MFMconfiguration.c file.
  * @author         : P.Kwekkeboom
  * @date           : Sep 14, 2023
  ******************************************************************************
  */
#ifndef MFMCONFIGURATION_H_
#define MFMCONFIGURATION_H_

#define NR_OF_SLOTS   6

#define PARA_LORA_INTERVAL_MIN  5
#define PARA_LORA_INTERVAL_MAX  1440

#define PARA_MEASURE_TIME_MIN  0
#define PARA_MEASURE_TIME_MAX  60000L

typedef struct __attribute__((__packed__))
{
    uint16_t moduleType;      //received from sensor module
    uint16_t measureTime;     //time between enable sensor and first measurement.
    bool enabled;             //true = sensor slot is enabled, false = sensor slot is disabled
    uint8_t reserved[11];     //reserved memory for future use
}struct_sensorSlotSettings;

typedef struct __attribute__((__packed__))
{
    uint16_t intervalLora;      //interval of lora sending
    bool alwaysOnSupplyEnabled; //true = alwaysOnSupply is always on, false = alwaysOnSupply is off in sleep
    struct_sensorSlotSettings slotModuleSettings[NR_OF_SLOTS];
    uint8_t spare[154];         //reserved memory for future use
    uint16_t crc;               //CRC for validate the data
}struct_MFMSettings;


typedef enum {

  //IDX_RESERVED = 0,  //(0x0000 corresponds to an EEPROM element invalidated by the driver

  IDX_INTERVAL_LORA = 100,
  IDX_ALWAYS_ON_SUPPLY_ENABLED,

  IDX_SENSOR1_MODULETYPE = 200,
  IDX_SENSOR2_MODULETYPE,
  IDX_SENSOR3_MODULETYPE,
  IDX_SENSOR4_MODULETYPE,
  IDX_SENSOR5_MODULETYPE,
  IDX_SENSOR6_MODULETYPE,

  IDX_SENSOR1_MEASURETIME = 300,
  IDX_SENSOR2_MEASURETIME,
  IDX_SENSOR3_MEASURETIME,
  IDX_SENSOR4_MEASURETIME,
  IDX_SENSOR5_MEASURETIME,
  IDX_SENSOR6_MEASURETIME,

  IDX_SENSOR1_ENABLED = 400,
  IDX_SENSOR2_ENABLED,
  IDX_SENSOR3_ENABLED,
  IDX_SENSOR4_ENABLED,
  IDX_SENSOR5_ENABLED,
  IDX_SENSOR6_ENABLED,

  IDX_CRC = 999,
  IDX_LAST = UINT16_MAX,

}ENUM_virtualAddress;


typedef enum {
  VIRTUAL_ELEMENT_SIZE_8bits = 1,                       // 8 bits element
  VIRTUAL_ELEMENT_SIZE_16bits,                          //16 bits element
  VIRTUAL_ELEMENT_SIZE_32bits,                          //32 bits element
}ENUM_virtualElementSize;

typedef struct {
    ENUM_virtualAddress virtualAddress;                 //virtual address from 1 to 65534, 0 and 65535 are reserved and may not used
    ENUM_virtualElementSize virtualElementSize;         //element size type 8bits, 16bits or 32bits
    void* pointerToItem;                                //pointer to element
}struct_virtual_EEPROM_item;

const int eraseVirtualEEPROM(void);
const int saveSettingsToVirtualEEPROM(void);
const int reloadSettingsFromVirtualEEPROM(void);
const int32_t getSensorStatus(int32_t sensorId);
const bool getAlwaysOn(void);
const bool getAlwaysOn_changed(bool reset);

#endif /* MFMCONFIGURATION_H_ */
