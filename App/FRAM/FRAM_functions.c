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

#define ADDRESS_OTHER_SETTINGS 0x0000
#define ADDRESS_LORA_SETTINGS 0x0100

typedef struct
{
    uint8_t sensorModuleId;
}struct_FRAM_setting;

struct_FRAM_setting stFramSettings;

/**
 * @fn const void setup_io_for_fram(bool)
 * @brief weak function to be override in application for enable I/O for FRAM operations
 *
 * @param state
 */
__weak const void setup_io_for_fram(bool state)
{
  UNUSED(state);
  __NOP();
}

/**
 * @fn const void saveLoraSettings(const void*, size_t)
 * @brief function to save Lora session settings in FRAM
 *
 * @param pSource : pointer of source data
 * @param length : size of data to write
 */
const void saveLoraSettings( const void *pSource, size_t length )
{
  assert_param( ADDRESS_LORA_SETTINGS + length < 2048);

  if( ADDRESS_LORA_SETTINGS + length > 2048)
  {
    APP_LOG(TS_OFF, VLEVEL_L, "Error FRAM lora size\r\n");
    return;
  }

  setup_io_for_fram(true);

  FRAM_WriteData(ADDRESS_LORA_SETTINGS,(uint8_t*)pSource, length);

  setup_io_for_fram(false);
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
  assert_param( ADDRESS_LORA_SETTINGS + length < 2048);

  if( ADDRESS_LORA_SETTINGS + length > 2048)
  {
    APP_LOG(TS_OFF, VLEVEL_L, "Error FRAM lora size\r\n");
    return;
  }

  setup_io_for_fram(true);

  FRAM_ReadData(ADDRESS_LORA_SETTINGS,(uint8_t*)pSource, length);

  setup_io_for_fram(false);
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
  assert_param( ADDRESS_OTHER_SETTINGS + length < ADDRESS_LORA_SETTINGS);

  if( ADDRESS_OTHER_SETTINGS + length > ADDRESS_LORA_SETTINGS)
  {
    APP_LOG(TS_OFF, VLEVEL_L, "Error FRAM user size\r\n");
    return;
  }

  setup_io_for_fram(true);

  FRAM_WriteData(ADDRESS_OTHER_SETTINGS,(uint8_t*)pSource, length);

  setup_io_for_fram(false);
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
  assert_param( ADDRESS_OTHER_SETTINGS + length < ADDRESS_LORA_SETTINGS);

  if( ADDRESS_OTHER_SETTINGS + length > ADDRESS_LORA_SETTINGS)
  {
    APP_LOG(TS_OFF, VLEVEL_L, "Error FRAM user size\r\n");
    return;
  }

  setup_io_for_fram(true);

  FRAM_ReadData(ADDRESS_OTHER_SETTINGS,(uint8_t*)pSource, length);

  setup_io_for_fram(false);
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

/**
 * @fn const int8_t setFramSetting(ENUM_FRAM_SETTING, void*, bool)
 * @brief function to set a setting in FRAM setting struct
 *
 * @param setting : item to set
 * @param value : value to set
 * @param forceWrite : true = force write data to FRAM
 * @return
 */
const int8_t setFramSetting( ENUM_FRAM_SETTING setting, void * value, bool forceWrite )
{
  switch(setting)
  {
    case FRAM_SETTING_MODEMID:
      stFramSettings.sensorModuleId = *((uint8_t*)value);
      break;

    default:
      return -1;
      break;

  }
  if( forceWrite )
  {
    saveFramSettings(&stFramSettings, sizeof(stFramSettings)); //save value to FRAM
  }
  return 0;
}

/**
 * @fn const int8_t getFramSetting(ENUM_FRAM_SETTING, void*, bool)
 * @brief
 *
 * @param setting : item to get
 * @param value : value to get
 * @param forceRead : true = force read data to FRAM
 * @return
 */
const int8_t getFramSetting( ENUM_FRAM_SETTING setting, void * value, bool forceRead )
{
  if( forceRead )
  {
    restoreFramSettings( &stFramSettings, sizeof(stFramSettings));
  }

  switch(setting)
  {
    case FRAM_SETTING_MODEMID:
      *((uint8_t*)value) = stFramSettings.sensorModuleId;
      break;

    default:
      return -1;
      break;

  }
  return 0;
}
