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

#include "board_io.h"
#include "board_io_functions.h"

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
