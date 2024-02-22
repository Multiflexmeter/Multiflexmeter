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

#include "../common/crc16.h"
#include "FRAM.h"
#include "FRAM_functions.h"

static_assert (MAX_SIZE_OTHER_SETTINGS + MAX_SIZE_LORA_SETTINGS <= SIZE_FRAM, "FRAM area sizes not correct");
static_assert (ADDRESS_OTHER_SETTINGS + MAX_SIZE_OTHER_SETTINGS <= ADDRESS_LORA_SETTINGS, "FRAM area OTHER SETTINGS not correct");
static_assert (ADDRESS_LORA_SETTINGS + MAX_SIZE_LORA_SETTINGS <= SIZE_FRAM, "FRAM area LORA SETTINGS not correct");

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

#include "LoRaMac.h"
#include "LoRaMacInterfaces.h"
/**
 * @fn const void saveLoraSettings(const void*, size_t)
 * @brief function to save Lora session settings in FRAM
 *
 * @param pSource : pointer of source data
 * @param length : size of data to write
 */
const void saveLoraSettings( const void *pSource, size_t length )
{
  assert_param( ADDRESS_LORA_SETTINGS + length < SIZE_FRAM);

  if( ADDRESS_LORA_SETTINGS + length > SIZE_FRAM)
  {
    APP_LOG(TS_OFF, VLEVEL_L, "Error FRAM lora size\r\n");
    return;
  }

  LoRaMacNvmData_t* nvmData = (LoRaMacNvmData_t*)pSource;
  uint32_t crc;
  uint16_t notifyFlags = LORAMAC_NVM_NOTIFY_FLAG_NONE;
  // Crypto
  crc = Crc32( ( uint8_t* ) &nvmData->Crypto, sizeof( nvmData->Crypto ) -
                                              sizeof( nvmData->Crypto.Crc32 ) );
  if( crc != nvmData->Crypto.Crc32 )
  {
      nvmData->Crypto.Crc32 = crc;
      notifyFlags |= LORAMAC_NVM_NOTIFY_FLAG_CRYPTO;
  }

  // MacGroup1
  crc = Crc32( ( uint8_t* ) &nvmData->MacGroup1, sizeof( nvmData->MacGroup1 ) -
                                                 sizeof( nvmData->MacGroup1.Crc32 ) );
  if( crc != nvmData->MacGroup1.Crc32 )
  {
      nvmData->MacGroup1.Crc32 = crc;
      notifyFlags |= LORAMAC_NVM_NOTIFY_FLAG_MAC_GROUP1;
  }

  // MacGroup2
  crc = Crc32( ( uint8_t* ) &nvmData->MacGroup2, sizeof( nvmData->MacGroup2 ) -
                                                 sizeof( nvmData->MacGroup2.Crc32 ) );
  if( crc != nvmData->MacGroup2.Crc32 )
  {
      nvmData->MacGroup2.Crc32 = crc;
      notifyFlags |= LORAMAC_NVM_NOTIFY_FLAG_MAC_GROUP2;
  }

  // Secure Element
  crc = Crc32( ( uint8_t* ) &nvmData->SecureElement, sizeof( nvmData->SecureElement ) -
                                                     sizeof( nvmData->SecureElement.Crc32 ) );
  if( crc != nvmData->SecureElement.Crc32 )
  {
      nvmData->SecureElement.Crc32 = crc;
      notifyFlags |= LORAMAC_NVM_NOTIFY_FLAG_SECURE_ELEMENT;
  }

  // Region
  crc = Crc32( ( uint8_t* ) &nvmData->RegionGroup1, sizeof( nvmData->RegionGroup1 ) -
                                              sizeof( nvmData->RegionGroup1.Crc32 ) );
  if( crc != nvmData->RegionGroup1.Crc32 )
  {
      nvmData->RegionGroup1.Crc32 = crc;
      notifyFlags |= LORAMAC_NVM_NOTIFY_FLAG_REGION_GROUP1;
  }

  crc = Crc32( ( uint8_t* ) &nvmData->RegionGroup2, sizeof( nvmData->RegionGroup2 ) -
                                              sizeof( nvmData->RegionGroup2.Crc32 ) );
  if( crc != nvmData->RegionGroup2.Crc32 )
  {
      nvmData->RegionGroup2.Crc32 = crc;
      notifyFlags |= LORAMAC_NVM_NOTIFY_FLAG_REGION_GROUP2;
  }

  // ClassB
  crc = Crc32( ( uint8_t* ) &nvmData->ClassB, sizeof( nvmData->ClassB ) -
                                              sizeof( nvmData->ClassB.Crc32 ) );
  if( crc != nvmData->ClassB.Crc32 )
  {
      nvmData->ClassB.Crc32 = crc;
      notifyFlags |= LORAMAC_NVM_NOTIFY_FLAG_CLASS_B;
  }

  if( notifyFlags)
  {
    APP_LOG(TS_OFF, VLEVEL_L, "CRC error: %d\r\n", notifyFlags);
  }

  APP_LOG(TS_OFF, VLEVEL_L, "CRC NVM save: 0x%04x, length: %d, address: 0x%08x\r\n", calculateCRC_CCITT((uint8_t*)pSource, 1468), length, pSource);

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
  assert_param( ADDRESS_LORA_SETTINGS + length < SIZE_FRAM);

  if( ADDRESS_LORA_SETTINGS + length > SIZE_FRAM)
  {
    APP_LOG(TS_OFF, VLEVEL_L, "Error FRAM lora size\r\n");
    return;
  }

  setup_io_for_fram(true);

  FRAM_ReadData(ADDRESS_LORA_SETTINGS,(uint8_t*)pSource, length);

  setup_io_for_fram(false);

  APP_LOG(TS_OFF, VLEVEL_L, "CRC NVM read: 0x%04x, length: %d, address: 0x%08x\r\n", calculateCRC_CCITT((uint8_t*)pSource, length), length, pSource);
}


/**
 * @fn const void saveFramSettings(const void*, size_t)
 * @brief
 *
 * @param pSource : pointer of source data
 * @param length : size of data to write
 */
static const void saveFramSettings( const void *pSource, size_t length )
{
  static_assert (sizeof(struct_FRAM_settings) <= ADDRESS_LORA_SETTINGS, "Size struct_FRAM_settings is too large");
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
static const void restoreFramSettings( const void *pDest, size_t length)
{
  static_assert (sizeof(struct_FRAM_settings) <= ADDRESS_LORA_SETTINGS, "Size struct_FRAM_settings is too large");
  assert_param( ADDRESS_OTHER_SETTINGS + length < ADDRESS_LORA_SETTINGS);

  if( ADDRESS_OTHER_SETTINGS + length > ADDRESS_LORA_SETTINGS)
  {
    APP_LOG(TS_OFF, VLEVEL_L, "Error FRAM user size\r\n");
    return;
  }

  setup_io_for_fram(true);

  FRAM_ReadData(ADDRESS_OTHER_SETTINGS,(uint8_t*)pDest, length);

  setup_io_for_fram(false);
}


/**
 * @fn const void saveFramSettingsStruct(struct_FRAM_settings*, size_t)
 * @brief stores FRAM settings struct, first calculates the CRC.
 *
 * @param pSource : pointer of source data
 * @param length : size of data to write
 */
const void saveFramSettingsStruct( struct_FRAM_settings *pSource, size_t length )
{
  pSource->protocolId = FRAM_SETTINGS_PROTOCOL_ID;
  pSource->crc16 = calculateCRC_CCITT((uint8_t*)&pSource->protocolId, sizeof(struct_FRAM_settings)-sizeof(pSource->crc16));

  saveFramSettings(pSource, length);
}

/**
 * @fn const void restoreFramSettingsStruct(const void*, size_t)
 * @brief restore FRAM settings struct, checks crc if not valid data is set to 0x0
 *
 * @param pSource : pointer of source data
 * @param length : size of data to write
 */
const void restoreFramSettingsStruct( const struct_FRAM_settings *pDest, size_t length)
{
  restoreFramSettings(pDest, length);

  uint16_t crc = calculateCRC_CCITT((uint8_t*)&pDest->protocolId, sizeof(struct_FRAM_settings)-sizeof(pDest->crc16));

  if( crc != pDest->crc16)
  {
    APP_LOG(TS_OFF, VLEVEL_L, "Error FRAM CRC, settings reset to 0x00\r\n");
    memset((uint8_t*)pDest, 0x00, sizeof(struct_FRAM_settings));
    return;
  }

  if( pDest->protocolId != FRAM_SETTINGS_PROTOCOL_ID)
  {
    APP_LOG(TS_OFF, VLEVEL_L, "Warning FRAM protocolID changed\r\n");
#if FRAM_SETTINGS_PROTOCOL_ID > 0x00
#warning make sure FRAM protocol changes are handled
#endif
    //implement converting for new protocol here
  }
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

  setup_io_for_fram(true);

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

  setup_io_for_fram(false);

  return result;
}
