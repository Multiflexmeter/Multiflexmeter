/**
  ******************************************************************************
  * @addtogroup     : App
  * @{
  * @file           : BatMon_functions.c
  * @brief          : Battery monitor functions
  * @author         : P.Kwekkeboom
  * @date           : Nov 1, 2023
  * @}
  ******************************************************************************
  */

#include "main.h"
#include "sys_app.h"

#include "../IO/board_io.h"
#include "bq35100.h"

/**
 * @fn const void initBatMon(void)
 * @brief function to initialize battery monitor
 *
 */
const void initBatMon(void)
{
  writeOutput_board_io(EXT_IO_GE_EN, GPIO_PIN_SET); //enable GE_EN, to operate battery monitor
  bq35100_init(&hi2c1);
}

/**
 * @fn const void testBatMon(int, int32_t*)
 * @brief test function battery monitor
 *
 * @param mode
 * @param value
 */
const void testBatMon( int mode, int32_t * value )
{
  if( mode == 0 ) //enable gauge
  {
    *value = (int32_t)bq35100_enableGauge();
  }

  if( mode == 1) //read supply
  {
    *value = (int32_t)bq35100_getVoltage();
  }
  else if( mode == 2) //read current
  {
    *value = (int32_t)bq35100_getCurrent();
  }

  else if( mode == 3 ) //read temperature
  {
    *value = (int32_t)bq35100_getInternalTemp();
  }

  else if( mode == 4 ) //read state of health
  {
    *value = (int32_t)bq35100_getStateOfHealth();
  }

  else if( mode == 5 ) //read design capacity
  {
    *value = (int32_t)bq35100_getDesignCapacity();
  }

  else if( mode == 6 ) //read scaled R
  {
    *value = (int32_t)bq35100_getScaledR();
  }

  else if( mode == 7 ) //read measured Z
  {
    *value = (int32_t) bq35100_getMeasuredZ();
  }

  else if( mode == 8 ) //disable gauge
  {
    *value = (int32_t)bq35100_disableGauge(true);
  }

  else if( mode == 9 ) //enable batmonitor
  {
    writeOutput_board_io(EXT_IO_GE_EN, GPIO_PIN_SET); //enable GE_EN, to operate battery monitor
    *value = 1;
  }

  else if( mode == 10 ) //disable batmonitor
  {
    writeOutput_board_io(EXT_IO_GE_EN, GPIO_PIN_RESET); //disable GE_EN, to disable battery monitor
    *value = 1;
  }

}
