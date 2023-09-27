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
#include "adc_if.h"

#include "common.h"

static uint32_t adc_vref = VREFINT_CAL_VREF;

uint32_t crossReferenceChannelAdc[]=
{
    ADC_CHANNEL_0,
    ADC_CHANNEL_1,
    ADC_CHANNEL_2,
    ADC_CHANNEL_3,
    ADC_CHANNEL_4,
    ADC_CHANNEL_5,
    ADC_CHANNEL_6,
    ADC_CHANNEL_7,
    ADC_CHANNEL_8,
    ADC_CHANNEL_9,
    ADC_CHANNEL_10,
    ADC_CHANNEL_11,
    ADC_CHANNEL_12,
    ADC_CHANNEL_13,
    ADC_CHANNEL_14,
    ADC_CHANNEL_15,
    ADC_CHANNEL_16,
    ADC_CHANNEL_17,
    ADC_CHANNEL_VREFINT,
    ADC_CHANNEL_TEMPSENSOR,
    ADC_CHANNEL_VBAT,
};

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


/**
 * @fn uint32_t SYS_GetAdc(int)
 * @brief function to read an internal ADC channel
 *
 * @param channel from 0 till \ref MAX_CHANNEL_ADC
 * @return ADC measurement value
 */
uint32_t SYS_GetAdc(int channel)
{
  assert_param(channel < MAX_CHANNEL_ADC);

  if( channel >= MAX_CHANNEL_ADC )
  {
    return 0;
  }

  return user_ADC_ReadChannels(crossReferenceChannelAdc[channel]);
}

/**
 * @fn uint16_t SYS_GetVoltage(int, uint32_t)
 * @brief
 *
 * @param channel from 0 till \ref MAX_CHANNEL_ADC
 * @param adcValue rough ADC measure
 * @return calculated value in mV and for temp channel in degree Celius
 */
uint16_t SYS_GetVoltage(int channel, uint32_t adcValue)
{
  uint16_t result = 0;

  assert_param(channel < MAX_CHANNEL_ADC);

  if (channel >= MAX_CHANNEL_ADC)
  {
    return 0;
  }

  //check vref is allready read, if not force to read first.
  if( adc_vref == 0 )
  {
    SYS_GetAdc(CHANNEL_VREFINT_ADC);
  }

  //check channel is VREFINT
  if( channel == CHANNEL_VREFINT_ADC )
  {
    if ((uint32_t) *VREFINT_CAL_ADDR != (uint32_t) 0xFFFFU)
    {
      /* Device with Reference voltage calibrated in production:
       use device optimized parameters */
      result = __LL_ADC_CALC_VREFANALOG_VOLTAGE(adcValue, ADC_RESOLUTION_12B); //convert to millivolt
    }
    else
    {
      result = (VREFINT_CAL_VREF * 1510) / adcValue;
    }

    if( channel == CHANNEL_VREFINT_ADC )
    {
      adc_vref = result; //save locally
    }

  }

  //check channel is TEMP SENSOR
  else if( channel == CHANNEL_TEMPSENSOR_ADC )
  {
    result = __LL_ADC_CALC_TEMPERATURE(adc_vref, adcValue, ADC_RESOLUTION_12B); //convert to temperature
  }

  //other channels
  else
  {
    result = __LL_ADC_CALC_DATA_TO_VOLTAGE(adc_vref, adcValue, ADC_RESOLUTION_12B); //convert to millivolt
  }

  return result;
}
