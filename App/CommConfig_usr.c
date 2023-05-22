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

/**
 * @brief override function for getBatterijSupply()
 *
 * @return battery supply in mV
 */
const uint16_t getBatterijSupply(void)
{

  return SYS_GetBatteryLevel();
}
