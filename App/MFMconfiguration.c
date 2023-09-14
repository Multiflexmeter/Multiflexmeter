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
#include "MFMconfiguration.h"

static struct_MFMSettings MFM_settings;

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
 * @brief override function getLoraInterval(), can be override in application code.
 *
 * @return Interval time in minutes
 */
const uint16_t getLoraInterval(void)
{
  return MFM_settings.intervalLora;
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
 * @brief override function getMeetTijd(), can be override in application code.
 *
 * @param sensorId : 1 to /ref NR_OF_SLOTS
 * @return Measure time in milliseconds
 */
const uint16_t getMeasureTime(int32_t sensorId)
{
  assert_param(sensorId >= 1 && sensorId <= NR_OF_SLOTS );

  if( sensorId < 1 || sensorId > NR_OF_SLOTS ) //Verify argument
  {
    return -1;
  }

  return (int)MFM_settings.slotModuleSettings[ sensorId - 1 ].measureTime;
}

/**
 * @brief override function getMeetTijd(), can be override in application code.
 *
 * @param sensorId : 1 to /ref NR_OF_SLOTS
 * @return Measure time in milliseconds
 */
const int32_t setMeasureTime(int32_t sensorId, uint16_t measureTime )
{
  assert_param(sensorId >= 1 && sensorId <= NR_OF_SLOTS );
  assert_param(measureTime >= PARA_MEASURE_TIME_MIN && measureTime <= PARA_MEASURE_TIME_MAX );


  if( sensorId < 1 || sensorId > NR_OF_SLOTS ) //Verify argument
  {
    return -1;
  }

  if( measureTime < PARA_MEASURE_TIME_MIN || measureTime > PARA_MEASURE_TIME_MAX ) //Verify argument
  {
    return -2;
  }

  MFM_settings.slotModuleSettings[ sensorId - 1 ].measureTime = measureTime;

  return (int32_t) MFM_settings.slotModuleSettings[ sensorId - 1 ].measureTime;
}


/**
 * @fn const void setAlwaysOn(bool)
 * @brief override function to set always on parameter
 *
 * @param state : true is enabled, false is disabled
 */
const void setAlwaysOn(bool state)
{
  MFM_settings.alwaysOnSupplyEnabled = state;
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
