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
 * @brief override function for getBatterijSupply()
 *
 * @return battery supply in mV
 */
const uint16_t getBatterijSupply(void)
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
  uint16_t supply_mV = SYS_GetVoltage(CHANNEL_4_ADC, measure);
  return supply_mV;
}

