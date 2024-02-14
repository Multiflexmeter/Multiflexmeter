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
  BACKUP_REGISTER_STATUS,
  BACKUP_REGISTER_LAST_WAKEUP_TIME,

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

typedef struct __attribute__((packed))
{
    uint8_t EOS;
    uint16_t voltage;
    uint8_t measureActive:1;
    uint8_t spareBits:7;
}struct_registerBattery;

typedef union
{
    uint32_t reg;
    uint8_t bytes[4];
    struct_registerBattery stRegBattery;
} UNION_registerBattery;

typedef struct __attribute__((packed))
{
    uint8_t testmodeActive:1;
    uint8_t testmodeBatteryGauge:1;
    uint32_t spareBits:30;
}struct_registerStatus;

typedef union
{
    uint32_t reg;
    uint8_t bytes[4];
    struct_registerStatus stRegStatus;
} UNION_registerStatus;

void detectResetBackup(void);
bool getResetBackup(void);
uint32_t getResetSource(void);
bool powerOnReset(void);
const void startDelayedReset(void);
void writeBackupRegister(ENUM_backupRegister backupRegisterId, uint32_t value);
uint32_t readBackupRegister(ENUM_backupRegister backupRegisterId);

uint32_t SYS_GetAdc(int channel);
int32_t SYS_GetVoltage(int channel, uint32_t adcValue);

int getDecimal(float value, int digits);

const void saveBatteryEos(bool measureNextInterval, uint8_t batteryEos, uint16_t batteryVoltage);
const struct_registerBattery getBatteryEos(void);
const struct_registerStatus getStatusRegister(void);
const void saveStatusTestmode( int mode );

const void setForceMeasurement( bool status );
const bool getForceMeasurement( void );
const void setForceInitSensor( bool status );
const bool getForceInitSensor( void );

const void setLastWakeupTime( uint32_t wakeTime );
const uint32_t getLastWakeupTime( void );

#endif /* COMMON_COMMON_H_ */
