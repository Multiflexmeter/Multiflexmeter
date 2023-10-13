/**
  ******************************************************************************
  * @addtogroup     : App
  * @{
  * @file           : FRAM_functions.c
  * @brief          : FRAM functions
  * @author         : P.Kwekkeboom
  * @date           : Oct 12, 2023
  * @}
  ******************************************************************************
  */
#include "main.h"
#include "sys_app.h"

#include "FRAM.h"
#include "FRAM_functions.h"

#define ADDRESS_LORA_SETTINGS 0x0000

/**
 * @fn const void saveLoraSettings(const void*, size_t)
 * @brief function to save Lora session settings in FRAM
 *
 * @param pSource : pointer of source data
 * @param length : size of data to write
 */
const void saveLoraSettings( const void *pSource, size_t length )
{
  FRAM_WriteData(ADDRESS_LORA_SETTINGS,(uint8_t*)pSource, length);
}

/**
 * @fn const void restoreLoraSettings(const void*, size_t)
 * @brief function to restore Lora session settings in FRAM
 *
 * @param pSource : pointer of source data
 * @param length : size of data to write
 */
const void restoreLoraSettings( const void *pSource, size_t length)
{
  FRAM_ReadData(ADDRESS_LORA_SETTINGS,(uint8_t*)pSource, length);
}
