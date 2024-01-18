/**
  ******************************************************************************
  * @addtogroup     : App
  * @{
  * @file           : CommConfig_usr
  * @brief          : Communication configuration user functions
  * @author         : P.Kwekkeboom
  * @date           : May 22, 2023
  * @copyright      : 2023 Dekimo Goes
  * @}
  ******************************************************************************
  */

#include "adc_if.h"
#include "common/common.h"
#include "CommConfig_usr.h"


/**
 * @brief override function for getVcc()
 *
 * @return Vcc supply in mV
 */
const uint16_t getVccSupply(void)
{

  return SYS_GetBatteryLevel();
}

/**
 * @fn const uint16_t getVbusSupply(void)
 * @brief override function for getVbusSupply
 *
 * @return vbus supply in mV
 */
const uint16_t getVbusSupply(void)
{
  uint32_t measure = SYS_GetAdc(CHANNEL_4_ADC);
  uint16_t supply_mV = (uint16_t)SYS_GetVoltage(CHANNEL_4_ADC, measure);

  const int32_t R18 = 1000;
  const int32_t R19 = 100;

  supply_mV *= (R18 + R19)/R19;

  return supply_mV;
}

/**
 * @fn const uint8_t getTemperature(void)
 * @brief function to get temperature of controller
 *
 * @return temperature in degree celsius (only whole degrees and above zero).
 */
const int8_t getTemperature(void)
{
  uint32_t measure = SYS_GetAdc(CHANNEL_TEMPSENSOR_ADC);
  int32_t temperature = SYS_GetVoltage(CHANNEL_TEMPSENSOR_ADC, measure);

  return (int8_t)temperature;
}
