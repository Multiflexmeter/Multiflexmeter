/**
  ******************************************************************************
  * @addtogroup     : App
  * @{
  * @file           : logging.c
  * @brief          : user level log functions
  * @author         : P.Kwekkeboom
  * @date           : Aug 30, 2023
  * @copyright      : 2023 Dekimo Goes
  * @}
  ******************************************************************************
  */

#include "main.h"
#include "sys_app.h"
#include "timer_if.h"
#include "../common/crc16.h"
#include "../dataflash/dataflash_functions.h"

static STRUCT_logdata logdata;
static bool logReady = 0;

static uint32_t newLogId = 0;

/**
 * @fn int8_t restoreLatestLogId(void)
 * @brief unction to read the latest log ID from backup memory or dataflash
 * result is stored locally in "logging.c"
 *
 * @return 0 = successful, -1 failed
 */
int8_t restoreLatestLogId(void)
{
  uint32_t readLatestId = 0;
  int8_t result;

  //todo read from backup register

  result = searchLatestLogInDataflash( & readLatestId );

  if( result < 0 ) //check on error
  {
    assert_param(1);
    APP_LOG(TS_OFF, VLEVEL_H, "Restore logging ID failes\r\n" );
    return -1;
  }

  else if ( result > 0 ) //check on empty dataflash
  {
    newLogId = 0;
    logReady = true;
  }

  else //okay, used read id.
  {
    newLogId = readLatestId + 1;
    logReady = true;
  }

  APP_LOG(TS_OFF, VLEVEL_H, "New logging ID: %u\r\n", newLogId);

  return 0;
}

/**
 * @fn int8_t restoreLatestTimeFromLog(void)
 * @brief function to load system time from latest dataflash value
 *
 * @return 0 = successful.
 */
int8_t restoreLatestTimeFromLog(void)
{
  assert_param( logReady == false ); //check logging is possible
  assert_param( newLogId == 0 ); //check previous logging exist.

  //check log is ready
  if( logReady == false )
  {
    return -1;
  }

  //check current log ID is not zero, then no previous registrations
  if( newLogId == 0 )
  {
    return -2;
  }

  //read the previous logId.
  if( readLogFromDataflash(newLogId - 1, (uint8_t*)&logdata, sizeof(logdata)) == 0)
  {
    //check the ID is not zero and not 0xFFFFFFFF
    if( logdata.measurementId > 0 && logdata.measurementId != 0xFFFFFFFF )
    {
      //check saved timestamp is larger than current time
      if( logdata.timestamp > SysTimeGet().Seconds )
      {
        SysTime_t newTime;
        newTime.Seconds = logdata.timestamp;
        newTime.SubSeconds = 0;
        SysTimeSet( newTime );
      }
      else
      { //current time is newer, do not restore old time
        return -5;
      }
    }
    else
    { //no valid ID
      return -4;
    }
  }
  else
  { //failed to read
    return -3;
  }

  return 0;
}

/**
 * @fn int8_t writeNewLog(uint8_t, uint8_t*, uint8_t)
 * @brief function to write a new log
 *
 * @param sensorModuleType
 * @param sensorData
 * @param dataLength
 * @return 0 = successful
 */
int8_t writeNewLog( uint8_t sensorModuleType, uint8_t * sensorData, uint8_t dataLength )
{
  int8_t result;

  assert_param( logReady == false ); //check logging is possible
  assert_param( sensorData == 0 ); //check pointer is not zero
  assert_param( dataLength > sizeof(logdata.sensorModuleData) ); //check maximum supported datasize.

  if (sensorData == 0)
  {
    return -1;
  }

  if (dataLength > sizeof(logdata.sensorModuleData))
  {
    return -2;
  }

  if( logReady == false )
  {
    APP_LOG(TS_OFF, VLEVEL_H, "Logging is not possible\r\n");
    return -3;
  }

  logdata.measurementId = newLogId; //set new log ID.
  logdata.timestamp = SysTimeGet().Seconds; //get system time, if time not yet in sync start from 0, otherwise unix timestamp

  logdata.sensorModuleType = sensorModuleType; //set sensor module type
  memcpy(logdata.sensorModuleData, sensorData, dataLength); //copy sensor data

  //check if not all bytes are used in sensorModuleData buffer
  if( dataLength < sizeof(logdata.sensorModuleData) )
  {
    memset(&logdata.sensorModuleData[dataLength], 0xFF, sizeof(logdata.sensorModuleData) - dataLength); //fill in remaining empty sensor data.
  }

  logdata.sensorModuleDatasize = dataLength; //set used datasize

  logdata.crc = calculateCRC_CCITT(logdata.sensorModuleData, logdata.sensorModuleDatasize); //calculate CRC on sensordata

  memset( logdata.spare, 0xFF, sizeof(logdata.spare)); //set 0xFF (blank) in spare array

  result = writeLogInDataflash(logdata.measurementId, (uint8_t*)&logdata, sizeof(logdata)); //write new log to dataflash

  //check result
  if( result == 0 ) //success
  {
    APP_LOG(TS_OFF, VLEVEL_H, "Log ID %u written to dataflash\r\n", newLogId );
    newLogId++;
  }

  else //failed
  {
    assert_param(1);
    APP_LOG(TS_OFF, VLEVEL_H, "Restore logging ID failes\r\n" );
    return -5;//failed to write log
  }

  return 0;
}

/**
 * @fn uint32_t getLatestLogId(void)
 * @brief function to return the latest Log ID
 *
 * @return latest log ID.
 */
uint32_t getLatestLogId(void)
{
  return newLogId;
}

/**
 * @fn const uint16_t getNumberOfMeasures(void)
 * @brief override function to return the number of log items
 * todo skip erased remaining block which is empty.
 *
 * @return  number of log items
 */
const uint16_t getNumberOfMeasures(void)
{
  uint32_t latestId = newLogId; //newLogId is the next ID, start with 0, so when newLogId = 1, there is 1 log item.

  if( latestId > NUMBER_PAGES_FOR_LOGGING )
  {
    return NUMBER_PAGES_FOR_LOGGING;
  }
  else
  {
    return latestId;
  }
}

/**
 * @fn const uint8_t eraseCompleteLog(void)
 * @brief override function to erase complete log
 * todo only erase the log memory blocks
 *
 * @return
 */
const uint8_t eraseCompleteLog(void)
{
  chipEraseDataflash(); //erase chip
  newLogId = 0; //reset LogId.

  return 0;
}
