/**
  ******************************************************************************
  * @file           : CommConfig_usr
  * @brief          : Header for CommConfig_usr file.
  * @author         : P.Kwekkeboom
  * @date           : May 22, 2023
  * @copyright      : 2023 Dekimo Goes
  ******************************************************************************
  */
#ifndef COMMCONFIG_USR_H_
#define COMMCONFIG_USR_H_

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



#endif /* COMMCONFIG_USR_H_ */
