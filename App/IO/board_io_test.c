/**
  ******************************************************************************
  * @addtogroup     : App
  * @{
  * @file           : board_io_test.c
  * @brief          : Board IO test functions
  * @author         : P.Kwekkeboom
  * @date           : Nov 22, 2023
  * @}
  ******************************************************************************
  */

#include "main.h"
#include "sys_app.h"

#include "board_io.h"
#include "board_io_functions.h"
#include "board_io_test.h"

/**
 * @fn const void testOutput_board_io(uint8_t, bool)
 * @brief test function for command interface
 *
 * @param item
 * @param state
 */
const void testOutput_board_io(uint8_t item, bool state)
{
  assert_param( item<MAX_IO_ITEM );
  if( item >= MAX_IO_ITEM)
  {
    return;
  }
  writeOutput_board_io((ENUM_IO_ITEM)item, (GPIO_PinState)state);
}

/**
 * @fn const int8_t testInput_board_io(uint8_t)
 * @brief test function for command interface
 *
 * @param item
 * @return
 */
const int8_t testInput_board_io(uint8_t item)
{
  assert_param( item<MAX_IO_ITEM );
  if( item >= MAX_IO_ITEM)
  {
    return -99;
  }
  return readInput_board_io((ENUM_IO_ITEM)item);
}

/**
 * @fn const void enable_vAlwaysOn(void)
 * @brief weak dummy function, must be override in application
 *
 */
__weak const void enable_vAlwaysOn(void)
{

}

/**
 * @fn const void disable_vAlwaysOn(void)
 * @brief weak dummy function, must be override in application
 *
 */
__weak const void disable_vAlwaysOn(void)
{

}

/**
 * @fn const void testSystemChecks(int, int32_t*)
 * @brief
 *
 * @param mode
 * @param value
 */
const void testSystemChecks( int mode, int32_t value )
{
  if( mode == 1) //1 = switch vSys
  {
    if( value == 0 ) //0 = off
    {
      disableVsys(); //disable vSys
    }
    else
    {
      enableVsys(); //enable vSys
    }
  }

  else if (mode == 2) //2 = switch vAlwaysOn
  {
    if (value == 0) //0 = off
    {
      disable_vAlwaysOn(); //disable vAlwaysOn
    }
    else
    {
      enable_vAlwaysOn(); //enable vAlwaysOn
    }
  }
}

/**
 * @fn const int8_t checkSpiPullupsVsystem(void)
 * @brief test function to verify one of SPI lines is high by the external pull-ups, which means the vSYS is enabled.
 *
 * @return 1 = one SPI line is high, 0 = all SPI lines are low
 */
const int8_t checkSpiPullupsVsystem(void)
{
  GPIO_PinState pinStateSCK = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5 ); //read input SPI SCK
  GPIO_PinState pinStateMISO = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6 ); //read input SPI MISO
  GPIO_PinState pinStateMOSI = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7 ); //read input SPI MOSI

  if ( pinStateSCK == GPIO_PIN_SET || pinStateMISO == GPIO_PIN_SET || pinStateMOSI == GPIO_PIN_SET )
  {
    return 1;
  }

  return 0;
}
