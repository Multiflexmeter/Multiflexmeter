/**
  ******************************************************************************
  * @file           : common.h
  * @brief          : Header for common.c file.
  * @author         : P.Kwekkeboom
  * @date           : Aug 31, 2023
  * @copyright      : 2023 Dekimo Goes
  ******************************************************************************
  */

#ifndef COMMON_COMMON_H_
#define COMMON_COMMON_H_

//#define ERASE_DATAFLASH       //enable for erasing dataflash
//#define ERASE_VIRTUAL_EEPROM  //enable for erasing virtual EEPROM
//#define ERASE_FRAM_NVM        //enable for erasing FRAM NVM

typedef enum {
  BACKUP_REGISTER_SECONDS,
  BACKUP_REGISTER_SUBSECONDS,
  BACKUP_REGISTER_MSBTICKS,
  BACKUP_REGISTER_OLDEST_MEASUREMENT_ID,
  BACKUP_REGISTER_LATEST_MEASUREMENT_ID,
  BACKUP_REGISTER_LAST_TIME_SYNC,
  BACKUP_REGISTER_REJOIN,
  BACKUP_REGISTER_BATTERY_EOS,

} ENUM_backupRegister;

typedef enum{
  CHANNEL_0_ADC,
  CHANNEL_1_ADC,
  CHANNEL_2_ADC,
  CHANNEL_3_ADC,
  CHANNEL_4_ADC,
  CHANNEL_5_ADC,
  CHANNEL_6_ADC,
  CHANNEL_7_ADC,
  CHANNEL_8_ADC,
  CHANNEL_9_ADC,
  CHANNEL_10_ADC,
  CHANNEL_11_ADC,
  CHANNEL_12_ADC,
  CHANNEL_13_ADC,
  CHANNEL_14_ADC,
  CHANNEL_15_ADC,
  CHANNEL_16_ADC,
  CHANNEL_17_ADC,
  CHANNEL_VREFINT_ADC,
  CHANNEL_TEMPSENSOR_ADC,
  CHANNEL_VBAT_ADC,
  CHANNEL_DACCH1_ADC,
  MAX_CHANNEL_ADC,
} ENUM_channelAdc;

typedef struct
{
    uint8_t day;
    uint8_t month;
    uint16_t year; //years from 0
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t century;
}struct_dateTime;

void detectResetBackup(void);
bool getResetBackup(void);
uint32_t getResetSource(void);
bool powerOnReset(void);
void writeBackupRegister(ENUM_backupRegister backupRegisterId, uint32_t value);
uint32_t readBackupRegister(ENUM_backupRegister backupRegisterId);

uint32_t SYS_GetAdc(int channel);
int32_t SYS_GetVoltage(int channel, uint32_t adcValue);

int getDecimal(float value, int digits);

const void saveBatteryEos(bool measureNextInterval, uint8_t batteryEos);
const uint32_t getBatteryEos(void);

#endif /* COMMON_COMMON_H_ */
