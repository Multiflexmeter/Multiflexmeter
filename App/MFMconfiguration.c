/**
  ******************************************************************************
  * @addtogroup     : App
  * @{
  * @file           : MFMconfiguration.c
  * @brief          : Configuration of MFM
  * @author         : P.Kwekkeboom
  * @date           : Sep 14, 2023
  * @}
  ******************************************************************************
  */

#include "platform.h"

#include "eeprom_emul.h"

#include "MFMconfiguration.h"

static struct_MFMSettings MFM_settings; //settings struct in RAM
static bool initAtFirstCall = true;
static bool vAlwaysStateChanged = false;

/**
 * definition of parameters with virtual address location for saving/reading from flash (virtual EEPROM)
 */
struct_virtual_EEPROM_item stVirtualEEPROM[] =
{
    { IDX_INTERVAL_LORA,                VIRTUAL_ELEMENT_SIZE_16bits,    &MFM_settings.intervalLora },
    { IDX_ALWAYS_ON_SUPPLY_ENABLED,     VIRTUAL_ELEMENT_SIZE_8bits,     &MFM_settings.alwaysOnSupplyEnabled },

    { IDX_SENSOR1_MODULETYPE,           VIRTUAL_ELEMENT_SIZE_16bits,    &MFM_settings.slotModuleSettings[0].moduleType },
    { IDX_SENSOR2_MODULETYPE,           VIRTUAL_ELEMENT_SIZE_16bits,    &MFM_settings.slotModuleSettings[1].moduleType },
    { IDX_SENSOR3_MODULETYPE,           VIRTUAL_ELEMENT_SIZE_16bits,    &MFM_settings.slotModuleSettings[2].moduleType },
    { IDX_SENSOR4_MODULETYPE,           VIRTUAL_ELEMENT_SIZE_16bits,    &MFM_settings.slotModuleSettings[3].moduleType },
    { IDX_SENSOR5_MODULETYPE,           VIRTUAL_ELEMENT_SIZE_16bits,    &MFM_settings.slotModuleSettings[4].moduleType },
    { IDX_SENSOR6_MODULETYPE,           VIRTUAL_ELEMENT_SIZE_16bits,    &MFM_settings.slotModuleSettings[5].moduleType },

    { IDX_SENSOR1_MEASURETIME,          VIRTUAL_ELEMENT_SIZE_16bits,    &MFM_settings.slotModuleSettings[0].numberOfSamples },
    { IDX_SENSOR2_MEASURETIME,          VIRTUAL_ELEMENT_SIZE_16bits,    &MFM_settings.slotModuleSettings[1].numberOfSamples },
    { IDX_SENSOR3_MEASURETIME,          VIRTUAL_ELEMENT_SIZE_16bits,    &MFM_settings.slotModuleSettings[2].numberOfSamples },
    { IDX_SENSOR4_MEASURETIME,          VIRTUAL_ELEMENT_SIZE_16bits,    &MFM_settings.slotModuleSettings[3].numberOfSamples },
    { IDX_SENSOR5_MEASURETIME,          VIRTUAL_ELEMENT_SIZE_16bits,    &MFM_settings.slotModuleSettings[4].numberOfSamples },
    { IDX_SENSOR6_MEASURETIME,          VIRTUAL_ELEMENT_SIZE_16bits,    &MFM_settings.slotModuleSettings[5].numberOfSamples },

    { IDX_SENSOR1_ENABLED,              VIRTUAL_ELEMENT_SIZE_8bits,     &MFM_settings.slotModuleSettings[0].enabled },
    { IDX_SENSOR2_ENABLED,              VIRTUAL_ELEMENT_SIZE_8bits,     &MFM_settings.slotModuleSettings[1].enabled },
    { IDX_SENSOR3_ENABLED,              VIRTUAL_ELEMENT_SIZE_8bits,     &MFM_settings.slotModuleSettings[2].enabled },
    { IDX_SENSOR4_ENABLED,              VIRTUAL_ELEMENT_SIZE_8bits,     &MFM_settings.slotModuleSettings[3].enabled },
    { IDX_SENSOR5_ENABLED,              VIRTUAL_ELEMENT_SIZE_8bits,     &MFM_settings.slotModuleSettings[4].enabled },
    { IDX_SENSOR6_ENABLED,              VIRTUAL_ELEMENT_SIZE_8bits,     &MFM_settings.slotModuleSettings[5].enabled },

    { IDX_CRC,                          VIRTUAL_ELEMENT_SIZE_16bits,    &MFM_settings.crc },
    { IDX_LAST,                         0,     0 },

};

/**
 * @fn const int eraseVirtualEEPROM(void)
 * @brief function to erase Virtual Eemprom memory in flash
 *
 * @return 0 = successful, -1 = error
 */
const int eraseVirtualEEPROM(void)
{
  EE_Status status;
  /* Unlock the Flash Program Erase controller */
  HAL_FLASH_Unlock();

  status = EE_Format(EE_FORCED_ERASE);

  /* lock the Flash Program Erase controller */
  HAL_FLASH_Lock();

  if(status == EE_OK )
  {
    return 0;
  }
  else
  {
    return -1;
  }
}

/**
 * @fn const uint16_t calculateCrcSettings(void)
 * @brief function to calculate the CRC of MFM_settings struct
 *
 * @return CRC
 */
const uint16_t calculateCrcSettings(void)
{
  uint8_t * pointer = (uint8_t*)&MFM_settings;
  uint32_t length = sizeof(MFM_settings) - sizeof(MFM_settings.crc);

  /* Reset CRC calculation unit */
   LL_CRC_ResetCRCCalculationUnit(CRC);

  while(length--)
  {
    /* Feed Data */
    LL_CRC_FeedData8(CRC, *pointer++);
  }

   /* Return computed CRC value */
   return LL_CRC_ReadData16(CRC);
}

uint16_t getCrcSettings(void)
{
  return MFM_settings.crc;
}


/**
 * @fn const int reloadSettingsFromVirtualEEPROM(void)
 * @brief function to load setting from virtual EEPROM (flash)
 * function first check if virtual EEPROM is already initialized, if not initialization will be called first.
 * Then all settings from \ref stVirtualEEPROM will be loaded in \ref MFM_settings
 *
 * @return 0 = successful, -1 = read error, -2 = wrong CRC
 */
const int reloadSettingsFromVirtualEEPROM(void)
{
  int item = 0;
  uint16_t crc;
  EE_Status status;

  bool readError = false;
  if( initAtFirstCall )
  {
    /* Unlock the Flash Program Erase controller */
    HAL_FLASH_Unlock();

    status = EE_Init(EE_FORCED_ERASE);
    initAtFirstCall = false; //reset

    /* lock the Flash Program Erase controller */
    HAL_FLASH_Lock();
  }

  do
  {

    switch( stVirtualEEPROM[item].virtualElementSize )
    {
      case VIRTUAL_ELEMENT_SIZE_8bits:

        status = EE_ReadVariable8bits(stVirtualEEPROM[item].virtualAddress, (uint8_t* )stVirtualEEPROM[item].pointerToItem);

        break;

      case VIRTUAL_ELEMENT_SIZE_16bits:

        status = EE_ReadVariable16bits(stVirtualEEPROM[item].virtualAddress, (uint16_t* )stVirtualEEPROM[item].pointerToItem);

        break;

      case VIRTUAL_ELEMENT_SIZE_32bits:

        status = EE_ReadVariable32bits(stVirtualEEPROM[item].virtualAddress, (uint32_t* )stVirtualEEPROM[item].pointerToItem);

        break;

      default:
        //nothing

        status = EE_NO_DATA;

        break;
    }

    if( status != EE_OK )
    {
      readError = true;
    }

  }
  while(stVirtualEEPROM[++item].virtualAddress != IDX_LAST);

  //check if error is found at reading, return error
  if( readError )
  {
    return -1;
  }

  //calculate new CRC
  crc = calculateCrcSettings();

  if( crc != MFM_settings.crc) //check CRC is not equal
  {
    return -2;  //wrong CRC
  }

  return 0; //Successful
}

/**
 * @fn const int saveSettingsToVirtualEEPROM(void)
 * @brief function to save setting from RAM to virtual EEPROM (flash)
 * function first check if virtual EEPROM is already initialized, if not initialization will be called first.
 * Then all settings from \ref MFM_settings will be loaded in \ref stVirtualEEPROM
 *
 * @return 0 = successful, -1 = write error
 */
const int saveSettingsToVirtualEEPROM(void)
{
  int item = 0;
  bool writeError = false;
  EE_Status status;

  /* Unlock the Flash Program Erase controller */
  HAL_FLASH_Unlock();

  if( initAtFirstCall )
  {
    status = EE_Init(EE_FORCED_ERASE);
    initAtFirstCall = false; //reset
  }

  MFM_settings.crc = calculateCrcSettings(); //calculate CRC to save

  do
  {

    switch( stVirtualEEPROM[item].virtualElementSize )
    {
      case VIRTUAL_ELEMENT_SIZE_8bits:

        status = EE_WriteVariable8bits(stVirtualEEPROM[item].virtualAddress, *((uint8_t* )stVirtualEEPROM[item].pointerToItem));

        break;

      case VIRTUAL_ELEMENT_SIZE_16bits:

        status = EE_WriteVariable16bits(stVirtualEEPROM[item].virtualAddress, *((uint16_t* )stVirtualEEPROM[item].pointerToItem));

        break;

      case VIRTUAL_ELEMENT_SIZE_32bits:

        status = EE_WriteVariable32bits(stVirtualEEPROM[item].virtualAddress, *((uint32_t* )stVirtualEEPROM[item].pointerToItem));

        break;

      default:
        //nothing

        status = EE_INVALID_ELEMENT;

        break;
    }

    switch( status ) //check result
    {
      case EE_OK:                 //result okay, do nothing

        break;

      case EE_CLEANUP_REQUIRED:   //result cleanup is required, trigger cleanup operation.

        EE_CleanUp(); //start clean up

        break;

      default:                    //other result, writing error

        writeError = true;        //set writeError flag

        break;
    }
  }
  while(stVirtualEEPROM[++item].virtualAddress != IDX_LAST); //next element, untill last

  /* lock the Flash Program Erase controller */
  HAL_FLASH_Lock();

  if( writeError ) //check on write error
  {
    return -1;
  }

  return 0; //successful
}

/**
 * @fn int getSensorStatus(int32_t)
 * @brief override function getSensorStatus(), can be override in application code
 *
 * @param sensorId : 1 to /ref NR_OF_SLOTS
 * @return -1 = sensorId out of range
 */
const int32_t getSensorStatus(int32_t sensorId)
{
  assert_param(sensorId >= 1 && sensorId <= NR_OF_SLOTS );

  if( sensorId < 1 || sensorId > NR_OF_SLOTS ) //Verify argument
  {
    return -1;
  }

  return (int32_t)MFM_settings.slotModuleSettings[ sensorId - 1 ].enabled;
}

/**
 * @fn int32_t setSensorStatus(int32_t, bool)
 * @brief override function setSensorStatus(), can be override in application code
 *
 * @param sensorId
 * @param status
 * @return
 */
const int32_t setSensorStatus(int32_t sensorId, bool status)
{
  assert_param(sensorId >= 1 && sensorId <= NR_OF_SLOTS );

  if( sensorId < 1 || sensorId > NR_OF_SLOTS ) //Verify argument
  {
    return -1;
  }

  MFM_settings.slotModuleSettings[ sensorId - 1 ].enabled = status;

  return (int32_t)MFM_settings.slotModuleSettings[ sensorId - 1 ].enabled;
}

/**
 * @fn int getSensorType(int32_t)
 * @brief override function getSensorType(), can be override in application code
 *
 * @param sensorId : 1 to /ref NR_OF_SLOTS
 * @return
 */
const int32_t getSensorType(int32_t sensorId)
{
  assert_param(sensorId >= 1 && sensorId <= NR_OF_SLOTS );

  if( sensorId < 1 || sensorId > NR_OF_SLOTS ) //Verify argument
  {
    return -1;
  }

  return (int32_t)MFM_settings.slotModuleSettings[ sensorId - 1 ].moduleType;
}

/**
 * @fn int setSensorType(int32_t)
 * @brief override function setSensorType(), can be override in application code
 *
 * @param sensorId : 1 to /ref NR_OF_SLOTS
 * @return
 */
const int32_t setSensorType(int32_t sensorId, uint16_t moduleType)
{
  assert_param(sensorId >= 1 && sensorId <= NR_OF_SLOTS );

  if( sensorId < 1 || sensorId > NR_OF_SLOTS ) //Verify argument
  {
    return -1;
  }

  MFM_settings.slotModuleSettings[ sensorId - 1 ].moduleType = moduleType;

  return (int32_t)MFM_settings.slotModuleSettings[ sensorId - 1 ].moduleType;
}

/**
 * @brief override function getLoraInterval().
 *
 * @return Interval time in minutes, force minimum of 5 minutes.
 */
const uint16_t getLoraInterval(void)
{
  uint16_t value = PARA_LORA_INTERVAL_MIN;

  if( value <= MFM_settings.intervalLora)
  {
    value = MFM_settings.intervalLora;
  }

  return value;
}

/**
 * @brief override function setLoraInterval(), can be override in application code.
 *
 * @return Interval time in minutes
 */
const int32_t setLoraInterval(uint16_t interval)
{
  assert_param(interval >= PARA_LORA_INTERVAL_MIN && interval <= PARA_LORA_INTERVAL_MAX );

  if( interval < PARA_LORA_INTERVAL_MIN || interval > PARA_LORA_INTERVAL_MAX ) //Verify argument
  {
    return -1;
  }

  MFM_settings.intervalLora = interval;

  return (int32_t) MFM_settings.intervalLora;
}

/**
 * @brief override function getNumberOfSamples(), can be override in application code.
 *
 * @param sensorId : 1 to /ref NR_OF_SLOTS
 * @return Number of samples 1-100
 */
const uint8_t getNumberOfSamples(int32_t sensorId)
{
  assert_param(sensorId >= 1 && sensorId <= NR_OF_SLOTS );

  if( sensorId < 1 || sensorId > NR_OF_SLOTS ) //Verify argument
  {
    return -1;
  }

  return (int)MFM_settings.slotModuleSettings[ sensorId - 1 ].numberOfSamples;
}

/**
 * @brief override function setNumberOfSamples(), can be override in application code.
 *
 * @param sensorId : 1 to /ref NR_OF_SLOTS
 * @return Number of samples 1-100
 */
const int32_t setNumberOfSamples(int32_t sensorId, uint8_t numberOfSamples )
{
  assert_param(sensorId >= 1 && sensorId <= NR_OF_SLOTS );
  assert_param(numberOfSamples >= PARA_MEASURE_TIME_MIN && numberOfSamples <= PARA_MEASURE_TIME_MAX );


  if( sensorId < 1 || sensorId > NR_OF_SLOTS ) //Verify argument
  {
    return -1;
  }

  if( numberOfSamples < PARA_MEASURE_SAMPLES_MIN || numberOfSamples > PARA_MEASURE_SAMPLES_MAX ) //Verify argument
  {
    return -2;
  }

  MFM_settings.slotModuleSettings[ sensorId - 1 ].numberOfSamples = (uint8_t)numberOfSamples;

  return (int32_t) MFM_settings.slotModuleSettings[ sensorId - 1 ].numberOfSamples;
}


/**
 * @fn const void setAlwaysOn(bool)
 * @brief override function to set always on parameter
 *
 * @param state : true is enabled, false is disabled
 */
const void setAlwaysOn(bool state)
{
  if( state != MFM_settings.alwaysOnSupplyEnabled ) //check change in value
  {
    MFM_settings.alwaysOnSupplyEnabled = state; //save new value
    vAlwaysStateChanged = true;                 //set change flag
  }
}

/**
 * @fn const bool getAlwaysOn(void)
 * @brief override function to get always on parameter
 *
 * @return state of always on, true = enabled, false = disabled
 */
const bool getAlwaysOn(void)
{

  return MFM_settings.alwaysOnSupplyEnabled;
}

/**
 * @fn const bool getAlwaysOn_changed(bool)
 * @brief function to detect a change in getAlwaysOn setting
 *
 * @param reset : true = reset flag, false = do not reset flag.
 * @return true = changed, false = not changed
 */
const bool getAlwaysOn_changed(bool reset)
{
  bool returnValue = vAlwaysStateChanged; //get change flag

  if( reset )                             //check reset flag is active
  {
    vAlwaysStateChanged = false;          //reset flag
  }
  return returnValue;
}
