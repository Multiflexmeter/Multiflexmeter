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

#include <math.h>

#include "main.h"
#include "sys_app.h"
#include "stm32wlxx_ll_rtc.h"
#include "app_types.h"
#include "usart.h"
#include "adc_if.h"
#include "timer_if.h"

#include "common.h"

static bool adc_vref_read = false;
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

static bool resetBackupDetected;

static UTIL_TIMER_Object_t reset_Timer;
static UTIL_TIMER_Time_t resetWaitTime = 1000;

static bool forceMeasurement;
static bool forceInitSensor;

/**
 * @fn void detectResetBackup(void)
 * @brief function to detect a reset of the backup domain.
 *
 */
void detectResetBackup(void)
{
  resetBackupDetected = LL_RCC_GetRTCClockSource() == LL_RCC_RTC_CLKSOURCE_NONE; //detect a reset of the RTC
}

/**
 * @fn bool getResetBackup(void)
 * @brief function to get the stored resetBackup state
 *
 * @return
 */
bool getResetBackup(void)
{
  return resetBackupDetected;
}

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
 * @fn const void trigger_reset(void*)
 * @brief function to reset the system
 *
 * @param context
 */
static const void trigger_reset(void *context)
{
  NVIC_SystemReset(); //reset
}

/**
 * @fn void startDelayedReset(void)
 * @brief function to start a delayed reboot.
 *
 */
const void startDelayedReset(void)
{
  UTIL_TIMER_Create(&reset_Timer, resetWaitTime, UTIL_TIMER_PERIODIC, trigger_reset, NULL); //create timer for delayed reset
  UTIL_TIMER_Start(&reset_Timer); //start timer
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
int32_t SYS_GetVoltage(int channel, uint32_t adcValue)
{
  int32_t result = 0;

  assert_param(channel < MAX_CHANNEL_ADC);

  if (channel >= MAX_CHANNEL_ADC)
  {
    return 0;
  }

  //check vref is allready read, if not force to read first.
  if( adc_vref_read == false && channel != CHANNEL_VREFINT_ADC)
  {
    uint32_t vref_adc = SYS_GetAdc(CHANNEL_VREFINT_ADC);
    SYS_GetVoltage(CHANNEL_VREFINT_ADC,vref_adc);
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

/**
 * @fn int getDecimal(float, int)
 * @brief function to return the decimals of a floating number.
 *
 * @param value float input
 * @param digits number of digits after decimal point
 * @return number after decimal point.
 */
int getDecimal(float value, int digits)
{
  int factor = pow(10,digits);
  return ((int)(value * factor)) % factor;
}

/**
 * @fn const void saveBatteryEos(bool, uint8_t, uint16_t)
 * @brief function to save the battery EOS to the backup registers
 *
 * @param measureNextInterval : true for activating measureming battery next interval
 * @param batteryEos : current battery EOS percentage
 * @param batteryVoltage : current battery voltage
 */
const void saveBatteryEos(bool measureNextInterval, uint8_t batteryEos, uint16_t batteryVoltage)
{
  static_assert (sizeof(UNION_registerBattery) == sizeof(uint32_t), "Size UNION_registerBattery is not correct");

  UNION_registerBattery UNvalue;

  UNvalue.stRegBattery.EOS = batteryEos;
  UNvalue.stRegBattery.voltage = batteryVoltage;
  UNvalue.stRegBattery.measureActive = measureNextInterval;

  writeBackupRegister(BACKUP_REGISTER_BATTERY_EOS, UNvalue.reg);
}

/**
 * @fn const struct_registerBattery_t getBatteryEos(void)
 * @brief function to get the battery EOS, voltage and measure status from the backup registers
 *
 */
const struct_registerBattery getBatteryEos(void)
{
  UNION_registerBattery UNvalue;
  UNvalue.reg= readBackupRegister(BACKUP_REGISTER_BATTERY_EOS);
  return UNvalue.stRegBattery;
}

/**
 * @fn const struct_registerStatus getStatusRegister(void)
 * @brief function to get the status register
 *
 * @return status register
 */
const struct_registerStatus getStatusRegister(void)
{
  UNION_registerStatus UNvalue;
  UNvalue.reg= readBackupRegister(BACKUP_REGISTER_STATUS);
  return UNvalue.stRegStatus;
}

/**
 * @fn const void saveStatusTestmode(bool)
 * @brief function to enable / disable the testmode bit in the status register.
 *
 * @param status : true is testmode enabled, false is testmode disabled
 */
const void saveStatusTestmode( bool status )
{
  static_assert (sizeof(UNION_registerStatus) == sizeof(uint32_t), "Size UNION_registerStatus is not correct");

  UNION_registerStatus UNvalue;

  UNvalue.reg= readBackupRegister(BACKUP_REGISTER_STATUS); //read original register first
  UNvalue.stRegStatus.testmodeActive = status; //set new value

  writeBackupRegister(BACKUP_REGISTER_STATUS, UNvalue.reg); //write changed register back
}

/**
 * @fn const void setForceMeasurement(bool)
 * @brief function to force a new measurement and send it by LoRA
 * Function only triggers the next sensorModule
 *
 * @param status
 */
const void setForceMeasurement( bool status )
{
  forceMeasurement = status;
}

/**
 * @fn const bool getForceMeasurement(void)
 * @brief function to get the forced measurement status
 *
 * @return actual status
 */
const bool getForceMeasurement( void )
{
  return forceMeasurement;
}

/**
 * @fn const void setForceInitSensor(bool)
 * @brief function to force a new trigger init sensor
 * Function only triggers the next sensorModule
 *
 * @param status
 */
const void setForceInitSensor( bool status )
{
  forceInitSensor = status;
}

/**
 * @fn const bool getForceInitSensor(void)
 * @brief function to get the forced init sensor status
 *
 * @return actual status
 */
const bool getForceInitSensor( void )
{
  return forceInitSensor;
}
