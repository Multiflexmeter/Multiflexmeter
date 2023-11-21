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
 * @fn int32_t SYS_GetAdc(int)
 * @brief weak function SYS_GetAdc(), can be override in application code
 *
 * @param sensorId
 * @return
 */
__weak const uint32_t SYS_GetAdc(int channel)
{
  return 0;
}

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
  return (uint16_t)SYS_GetAdc(CHANNEL_4_ADC);
}

