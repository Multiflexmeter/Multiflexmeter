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

#define ADDRESS_TEST_SETTINGS 0x0000
#define ADDRESS_LORA_SETTINGS 0x0100

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


/**
 * @fn const void saveFramSettings(const void*, size_t)
 * @brief
 *
 * @param pSource : pointer of source data
 * @param length : size of data to write
 */
const void saveFramSettings( const void *pSource, size_t length )
{
  FRAM_WriteData(ADDRESS_TEST_SETTINGS,(uint8_t*)pSource, length);
}

/**
 * @fn const void restoreFramSettings(const void*, size_t)
 * @brief
 *
 * @param pSource : pointer of source data
 * @param length : size of data to write
 */
const void restoreFramSettings( const void *pSource, size_t length)
{
  FRAM_ReadData(ADDRESS_TEST_SETTINGS,(uint8_t*)pSource, length);
}

/**
 * @fn const int8_t testFram(uint8_t * status)
 * @brief function to test FRAM
 *
 * @return 0 = successful, -1 is error.
 */
const int8_t testFram(uint8_t * status)
{
  int8_t result = 0;
  uint8_t testReadOriginal;
  uint8_t testRead;
  uint8_t testWrite;

  *status = FRAM_ReadStatusRegister(); //read status register

  restoreFramSettings(&testReadOriginal, sizeof(testReadOriginal)); // first read original value

  testWrite = 0xAA;
  saveFramSettings(&testWrite, sizeof(testWrite)); //save test value to FRAM

  restoreFramSettings(&testRead, sizeof(testRead)); //read test value back from FRAM

  if( testWrite != testRead ) //test value is not okay
  {
    result = -1;
  }

  saveFramSettings(&testReadOriginal, sizeof(testReadOriginal)); //write back orinal value to FRAM

  return result;
}
