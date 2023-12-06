/**
  ******************************************************************************
  * @addtogroup     : App
  * @{
  * @file           : board_io_functions.c
  * @brief          : Board IO functions
  * @author         : P.Kwekkeboom
  * @date           : Nov 22, 2023
  * @}
  ******************************************************************************
  */
#include "main.h"
#include "sys_app.h"

#include "stm32_timer.h"

#include "board_io.h"
#include "board_io_functions.h"

static UTIL_TIMER_Object_t AlwaysOnSwitch_Timer;
static UTIL_TIMER_Time_t AlwaysOnSwitchOnTime = 3000; //3sec
static UTIL_TIMER_Time_t AlwaysOnSwitchOffTime = 6000; //6sec

/**
 * @fn const bool lightInput(void)
 * @brief function to get light sensor status
 *
 * @return true = light found, false = dark
 */
const bool getLigthSensorStatus(void)
{
  return readInput_board_io((ENUM_IO_ITEM)INT_IO_BOX_OPEN);

}

/**
 * @fn const void enableVsys(void)
 * @brief function to enable vSYS
 * sets also all CS lines to output
 *
 */
const void enableVsys(void)
{
  writeOutput_board_io(EXT_IOVSYS_EN, GPIO_PIN_SET);
  setAsOutput(EXT_IO_FRAM_CS);
  setAsOutput(EXT_IO_FLASH_SD_CS);
  setAsOutput(EXT_IO_FLASH_NOR_CS);
}

/**
 * @fn const void disableVsys(void)
 * @brief function to disable vSYS
 * sets also all CS lines to input (low power mode).
 *
 */
const void disableVsys(void)
{
  writeOutput_board_io(EXT_IOVSYS_EN, GPIO_PIN_RESET);
  setAsInput(EXT_IO_FRAM_CS);
  setAsInput(EXT_IO_FLASH_SD_CS);
  setAsInput(EXT_IO_FLASH_NOR_CS);
}

/**
 * @fn const void FRAM_EnableChipSelect(void)
 * @brief override function for FRAM chip select enable
 *
 */
const void FRAM_EnableChipSelect(void)
{
  writeOutput_board_io(EXT_IO_FRAM_CS, GPIO_PIN_SET);
}

/**
 * @fn const void FRAM_DisableChipSelect(void)
 * @brief override function for FRAM chip select disable
 *
 */
const void FRAM_DisableChipSelect(void)
{
  writeOutput_board_io(EXT_IO_FRAM_CS, GPIO_PIN_RESET);
}


/**
 * @fn const void SD_EnableChipSelect(void)
 * @brief override function for SD Card chip select enable
 *
 */
const void SD_EnableChipSelect(void)
{
  writeOutput_board_io(EXT_IO_FLASH_SD_CS, GPIO_PIN_SET);
}

/**
 * @fn const void SD_DisableChipSelect(void)
 * @brief override function for SD card chip select disable
 *
 */
const void SD_DisableChipSelect(void)
{
  writeOutput_board_io(EXT_IO_FLASH_SD_CS, GPIO_PIN_RESET);
}

/**
 * @fn const void dataflash_EnableChipSelect(void)
 * @brief override function for dataflash chip select enable
 *
 */
const void dataflash_EnableChipSelect(void)
{
  writeOutput_board_io(EXT_IO_FLASH_NOR_CS, GPIO_PIN_SET);
}

/**
 * @fn const void dataflash_DisableChipSelect(void)
 * @brief override function for dataflash chip select disable
 *
 */
const void dataflash_DisableChipSelect(void)
{
  writeOutput_board_io(EXT_IO_FLASH_NOR_CS, GPIO_PIN_RESET);
}

/**
 * @fn const void slotPower(ENUM_slotId, bool)
 * @brief function to control the slot power
 * NOTE: for disable also the common vSensor supply is switched off
 *
 * @param slotId
 * @param enable true = enable, false = disable
 */
const void slotPower(ENUM_slotId slotId, bool enable)
{
  assert_param( slotId >= 0 && slotId < MAX_SENSOR_SLOT );

  if( slotId < 0 || slotId >= MAX_SENSOR_SLOT )
  {
    APP_LOG(TS_OFF, VLEVEL_H, "Wrong slotid\r\n", slotId );
    return; //error, exit
  }

  GPIO_PinState pinState = enable ? GPIO_PIN_SET : GPIO_PIN_RESET;

  switch(slotId)
  {
    case sensorSlot1:
      writeOutput_board_io(EXT_IOVSENSOR_EN, pinState );
      writeOutput_board_io(EXT_IOLOADSW1, pinState);
      break;
    case sensorSlot2:
      writeOutput_board_io(EXT_IOVSENSOR_EN, pinState );
      writeOutput_board_io(EXT_IOLOADSW2, pinState);
      break;
    case sensorSlot3:
      writeOutput_board_io(EXT_IOVSENSOR_EN, pinState );
      writeOutput_board_io(EXT_IOLOADSW3, pinState);
      break;
    case sensorSlot4:
      writeOutput_board_io(EXT_IOVSENSOR_EN, pinState );
      writeOutput_board_io(EXT_IOLOADSW4, pinState);
      break;
    case sensorSlot5:
      writeOutput_board_io(EXT_IOVSENSOR_EN, pinState );
      writeOutput_board_io(EXT_IOLOADSW5, pinState);
      break;
    case sensorSlot6:
      writeOutput_board_io(EXT_IOVSENSOR_EN, pinState );
      writeOutput_board_io(EXT_IOLOADSW6, pinState);
      break;
    default:
      break;
  }
}

/**
 * @fn const void delayedSwitchOff_IO_vAlwaysOn(void)
 * @brief function to delayed switch off the vAlwaysOn set pin
 *
 */
static const void delayedSwitchOff_IO_vAlwaysOn(void *context)
{
  writeOutput_board_io(EXT_IOVALWAYS_EN, GPIO_PIN_RESET);
  int8_t vAlways_sens = readInput_board_io(EXT_IO_VALWAYS_SENS);
  APP_LOG(TS_OFF, VLEVEL_H, "Supply V-always: sens state after %d\r\n", vAlways_sens );
}

/**
 * @fn const void init_vAlwaysOn(void)
 * @brief function to create timer at power up
 *
 */
const void init_vAlwaysOn(void)
{
  UTIL_TIMER_Create(&AlwaysOnSwitch_Timer, AlwaysOnSwitchOnTime, UTIL_TIMER_ONESHOT, delayedSwitchOff_IO_vAlwaysOn, NULL); //create timer
}

/**
 * @fn const void enable_vAlwaysOn(void)
 * @brief function to start enable vAlways on supply
 *
 */
const void enable_vAlwaysOn(void)
{
  int8_t vAlways_sens = readInput_board_io(EXT_IO_VALWAYS_SENS);

  //check vAlways supply is off
  if( vAlways_sens == 0 )
  { //then switch on
    writeOutput_board_io(EXT_IOVALWAYS_EN, GPIO_PIN_SET);
    UTIL_TIMER_StartWithPeriod(&AlwaysOnSwitch_Timer, AlwaysOnSwitchOnTime); //set new time and start timer
  }
  else
  {
    //already on, do nothing.
  }

  APP_LOG(TS_OFF, VLEVEL_H, "Supply V-always: switch ON, sens state before %d\r\n", vAlways_sens );

}

/**
 * @fn const void disable_vAlwaysOn(void)
 * @brief function to start disable vAlways on supply
 *
 */
const void disable_vAlwaysOn(void)
{
  int8_t vAlways_sens = readInput_board_io(EXT_IO_VALWAYS_SENS);

  //check vAlways supply is on
  if( readInput_board_io(EXT_IO_VALWAYS_SENS) )
  { //then switch off
    writeOutput_board_io(EXT_IOVALWAYS_EN, GPIO_PIN_SET);
    UTIL_TIMER_StartWithPeriod(&AlwaysOnSwitch_Timer, AlwaysOnSwitchOffTime); //set new time and start timer
  }
  else
  {
    //already off, do nothing.
  }

  APP_LOG(TS_OFF, VLEVEL_H, "Supply V-always: switch OFF, sens state before %d\r\n", vAlways_sens );

}


/**
 * @fn const void setup_io_for_SPI_devices(bool)
 * @brief override of weak function. To enable I/O needed for SPI devices on vSys
 * function detects the vSys previous state and turns ON/OFF vSys if it was off.
 *
 * @param state
 */
const void setup_io_for_SPI_devices(bool state)
{
  static int8_t vSysState = 0;


  if( state == true ) //vSys needs to be active
  {
    vSysState = readInput_board_io(EXT_IOVSYS_EN); //read first current state

    if( vSysState == 0 ) //if current state is off, switch ON vSys
    {
      enableVsys();
    }
  }

  else //state = false, Vsys needs to be in old state.
  {
    if( vSysState == 0 ) //if previous state was OFF, then disable vSys again.
    {
      disableVsys();
    }
  }
}

/**
 * @fn const void setup_io_for_fram(bool)
 * @brief override of weak function. To enable I/O needed for FRAM
 * function detects the vSys previous state and turns ON/OFF vSys if it was off.
 *
 * @param state
 */
const void setup_io_for_fram(bool state)
{
  setup_io_for_SPI_devices(state);
}

/**
 * @fn const void setup_io_for_sdcard(bool)
 * @brief override of weak function. To enable I/O needed for dataflash
 * function detects the vSys previous state and turns ON/OFF vSys if it was off.
 *
 * @param state
 */
const void setup_io_for_dataflash(bool state)
{
  setup_io_for_SPI_devices(state);
}

/**
 * @fn const void setup_io_for_SdCard(bool)
 * @brief override of weak function. To enable I/O needed for SD-card
 * function detects the vSys previous state and turns ON/OFF vSys if it was off.
 *
 * @param state
 */
const void setup_io_for_SdCard(bool state)
{
  setup_io_for_SPI_devices(state);
}


