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

typedef enum {
  BACKUP_REGISTER_SECONDS,
  BACKUP_REGISTER_SUBSECONDS,
  BACKUP_REGISTER_MSBTICKS,
  BACKUP_REGISTER_OLDEST_LOG,
  BACKUP_REGISTER_LATEST_LOG,

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

uint32_t getResetSource(void);
bool powerOnReset(void);
void writeBackupRegister(ENUM_backupRegister backupRegisterId, uint32_t value);
uint32_t readBackupRegister(ENUM_backupRegister backupRegisterId);

#endif /* COMMON_COMMON_H_ */
