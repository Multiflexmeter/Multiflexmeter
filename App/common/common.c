/**
  ******************************************************************************
  * @addtogroup     : common
  * @{
  * @file           : common.c
  * @brief          : common functions
  * @author         : P.Kwekkeboom
  * @date           : Aug 31, 2023
  * @copyright      : 2023 Dekimo Goes
  * @}
  ******************************************************************************
  */

#include "main.h"
#include "stm32wlxx_ll_rtc.h"
#include "app_types.h"
#include "usart.h"

#include "common.h"

static uint32_t resetSource;

/**
 * @fn uint32_t getResetSource(void)
 * @brief function to read out reset cause
 *
 * @return
 */
uint32_t getResetSource(void)
{
  resetSource |= HAL_RCC_GetResetSource();

  return resetSource;
}

/**
 * @fn bool powerOnReset(void)
 * @brief function to get a powerOnReset status
 *
 * @return true if \ref RCC_RESET_FLAG_PWR is set.
 */
bool powerOnReset(void)
{
  return (getResetSource() & RCC_RESET_FLAG_PWR) ? true : false;
}

/**
 * @fn void writeBackupRegister(ENUM_backupRegister, uint32_t)
 * @brief user function to write a backup register
 *
 * @param backupRegisterId \ref ENUM_backupRegister
 * @param value
 */
void writeBackupRegister(ENUM_backupRegister backupRegisterId, uint32_t value)
{
  LL_RTC_BKP_SetRegister(RTC, backupRegisterId, value); //write value in backup register
}

/**
 * @fn uint32_t readBackupRegister(ENUM_backupRegister)
 * @brief user function to read a backup register
 *
 * @param backupRegisterId \ref ENUM_backupRegister
 * @return
 */
uint32_t readBackupRegister(ENUM_backupRegister backupRegisterId)
{
  return LL_RTC_BKP_GetRegister(RTC, backupRegisterId); //read value in backup register
}
