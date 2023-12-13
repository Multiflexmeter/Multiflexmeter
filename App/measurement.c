/**
  ******************************************************************************
  * @addtogroup     : App
  * @{
  * @file           : measurement.c
  * @brief          : user level measurement functions
  * @author         : P.Kwekkeboom
  * @date           : Aug 30, 2023
  * @copyright      : 2023 Dekimo Goes
  * @}
  ******************************************************************************
  */

#include <stdio.h>

#include "main.h"
#include "sys_app.h"
#include "timer_if.h"
#include "common/crc16.h"
#include "common/common.h"
#include "dataflash/dataflash_functions.h"
#include "measurement.h"

static STRUCT_logdata logdata;
static bool logReady = 0;

static uint32_t newLogId = 0;

/**
 * @fn int8_t searchLatestLogInDataflash(uint32_t*)
 * @brief function to search the latest log record.
 * a derivative of binary search algorithm is used.
 * Based on the first item in dataflash and latest item is filled the algorithm start searching for the highest.
 * When the item is found a check is done to make sure the next item is smaller or empty.
 *
 * @param logId destionation of found log record
 * @return 0 = succesfull found, 1 = empty dataflash
 */
int8_t searchLatestLogInDataflash( uint32_t * logId )
{
  uint32_t boundaryStart = 0;
  uint32_t boundaryEnd = NUMBER_PAGES_FOR_LOGGING;

  uint32_t highestMeasurementId;
  uint32_t firstRecordMeasurementId;
  uint32_t lastRecordMeasurementId;
  uint32_t newReadingId;

  UNION_logdata * pLog = (UNION_logdata *)&logdata;

  //read first record
  readLogFromDataflash(0, (uint8_t *) &logdata, sizeof(logdata));
  firstRecordMeasurementId = pLog->log.measurementId;

  //read latest record
  readLogFromDataflash(NUMBER_PAGES_FOR_LOGGING - 1, (uint8_t *) &logdata, sizeof(logdata));
  lastRecordMeasurementId = pLog->log.measurementId;

  //determine ringbuffer is in overflow
  if( firstRecordMeasurementId == 0xFFFFFFFFUL && lastRecordMeasurementId == 0xFFFFFFFFUL )
  {
    //empty dataflash
    *logId = 0;

    APP_LOG(TS_OFF, VLEVEL_H, "Empty dataflash.\r\n");

    return 1;
  }

  else if ( lastRecordMeasurementId == 0xFFFFFFFFUL )
  {
    //no overflow first line filled, last not yet
    highestMeasurementId = firstRecordMeasurementId;

    APP_LOG(TS_OFF, VLEVEL_H, "No overflow in dataflash.\r\n");
  }

  else if ( firstRecordMeasurementId == 0xFFFFFFFFUL )
  {
    //overflow, first line empty
    *logId = lastRecordMeasurementId;

    APP_LOG(TS_OFF, VLEVEL_H, "First line empty in dataflash, last line %u with ID %u.\r\n", NUMBER_PAGES_FOR_LOGGING - 1, lastRecordMeasurementId);

    return 0;
  }

  else
  {
    //overflow, first and last line filled.
    highestMeasurementId = firstRecordMeasurementId;

    APP_LOG(TS_OFF, VLEVEL_H, "Overflow in dataflash.\r\n");
  }

  while( boundaryStart <= boundaryEnd )
  {
    newReadingId = (boundaryStart + boundaryEnd) >> 1;

    //read page
    readLogFromDataflash(newReadingId, (uint8_t *) &logdata, sizeof(logdata));

    if( pLog->log.measurementId != 0xFFFFFFFF )
    {
      //page contain log, new value is further
      if (pLog->log.measurementId < highestMeasurementId)
      {
        //value on newReadingId is smaller, then decrease the end boundary
        boundaryEnd = newReadingId - 1;
      }

      else if (pLog->log.measurementId > highestMeasurementId)
      {
        //value larger then previous, then increase the start boundary.
        highestMeasurementId = pLog->log.measurementId;
        boundaryStart = newReadingId + 1;
      }

      else
      { //no change, item found
        int8_t timeout = NUMBER_OF_PAGES_IN_4K_BLOCK_DATAFLASH; //max of one block can be erased in between.

        //verify next item is not higher
        do
        {
          readLogFromDataflash(newReadingId + 1, (uint8_t *) &logdata, sizeof(logdata));
          if (pLog->log.measurementId != 0xFFFFFFFF && pLog->log.measurementId > highestMeasurementId)
          {
            //value larger then previous, then take over this value
            highestMeasurementId = pLog->log.measurementId;
            newReadingId++;
          }
          timeout--;
        } while( pLog->log.measurementId != 0xFFFFFFFF && pLog->log.measurementId >= highestMeasurementId && timeout >=0);


        *logId = highestMeasurementId;
        return 0;
      }
    }

    else
    {
      //page contain no data, new value is in upper half, decrease the end boundary
      boundaryEnd = newReadingId - 1;
    }


    APP_LOG(TS_OFF, VLEVEL_H, "Search between address %u and address %u, highest found ID %u, last read ID %u.\r\n", boundaryStart, boundaryEnd, highestMeasurementId, pLog->log.measurementId );
  }

  *logId = highestMeasurementId;

  return 0;

}

/**
 * @fn int8_t restoreLatestLogId(void)
 * @brief function to read the latest log ID from backup memory or dataflash
 * result is stored locally in "measurement.c"
 *
 * @return 1= successful from backup register, 0 = successful from dataflash search, -1 failed
 */
int8_t restoreLatestLogId(void)
{
  uint32_t readLatestId = 0;
  uint32_t readLatestIdFromBackupRegister = 0;
  int8_t result;

  UNION_logdata * pLog = (UNION_logdata *)&logdata;

  APP_LOG(TS_OFF, VLEVEL_H, "Reset cause: %x\r\n", getResetSource() );

  if (getResetBackup())
  {
    APP_LOG(TS_OFF, VLEVEL_H, "VBACKUP: reset detect\r\n");
  }

  //check no power on reset
  if( getResetBackup() == false )
  {
    readLatestIdFromBackupRegister = readBackupRegister( BACKUP_REGISTER_LATEST_LOG ); //get value from backup register

    //verify dataflash read
    readLogFromDataflash(readLatestIdFromBackupRegister - 1, (uint8_t *) &logdata, sizeof(logdata));
    if( pLog->log.measurementId == readLatestIdFromBackupRegister - 1)
    {
      newLogId = readLatestIdFromBackupRegister;
      logReady = true;

      APP_LOG(TS_OFF, VLEVEL_H, "New measurument ID from backup register: %u\r\n", newLogId);

      return 1;
    }

    else
    {
      APP_LOG(TS_OFF, VLEVEL_H, "Mismatch in backup register\r\n" );
    }
  }

  else
  {
    //nothing
    APP_LOG(TS_OFF, VLEVEL_H, "PowerOnReset\r\n" );
  }


  //Power on reset or backup register corrupted
  result = searchLatestLogInDataflash( &readLatestId );

  if( result < 0 ) //check on error
  {
    assert_param(1);
    APP_LOG(TS_OFF, VLEVEL_H, "Restore measurement ID failes\r\n" );
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

  writeBackupRegister(BACKUP_REGISTER_LATEST_LOG, newLogId); //save new value in backup register

  APP_LOG(TS_OFF, VLEVEL_H, "New measurement ID by searching: %u\r\n", newLogId);

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

#if VERBOSE_LEVEL == VLEVEL_H

         char timeStringNow[20] = {0};
         struct tm stTime;
         SysTimeLocalTime(SysTimeGet().Seconds, &stTime); //get system time
         strftime(timeStringNow, sizeof(timeStringNow), "%d-%m-%Y %H:%M:%S", &stTime); //make date/time string
         APP_LOG(TS_OFF, VLEVEL_H, "Time: restored from log: %s\r\n", timeStringNow);

#endif
      }
      else
      { //current time is newer, do not restore old time
        APP_LOG(TS_OFF, VLEVEL_H, "Time: current time is newer, time not restored\r\n");
        return -5;
      }
    }
    else
    { //no valid ID
      APP_LOG(TS_OFF, VLEVEL_H, "Time: ERROR, no valid dataflash ID \r\n");
      return -4;
    }
  }
  else
  { //failed to read
    APP_LOG(TS_OFF, VLEVEL_H, "Time: ERROR, failed to read from dataflash\r\n");
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
int8_t writeNewLog( uint8_t MFM_protocol, struct_MFM_sensorModuleData * sensorModuleData, struct_MFM_baseData * MFM_data)
{
  int8_t result;
  bool turnoverAndErased = false;

  static_assert (sizeof(struct_MFM_sensorModuleData) == MAX_SENSOR_MODULE_DATA, "Size struct_MFM_sensorModuleData is not correct");
  static_assert (sizeof(struct_MFM_baseData) == MAX_BASE_MODULE_DATA, "Size struct_MFM_baseData is not correct");
  static_assert (sizeof(STRUCT_logdata) == MAX_SIZE_LOGDATA, "Size STRUCT_logdata is not correct");

  assert_param( logReady == true ); //check logging is possible
  assert_param( sensorModuleData != 0 ); //check pointer is not zero
  assert_param( MFM_data != 0 ); //check pointer is not zero
  assert_param( sensorModuleData->sensorModuleDataSize <= sizeof(sensorModuleData->sensorModuleData) ); //check maximum supported datasize.

  if (sensorModuleData == 0)
  {
    APP_LOG(TS_OFF, VLEVEL_H, "LOG: sensor module data pointer is zero\r\n");
    return -1;
  }

  if (sensorModuleData->sensorModuleDataSize > sizeof(sensorModuleData->sensorModuleData))
  {
    APP_LOG(TS_OFF, VLEVEL_H, "LOG: sensor module data too large\r\n");
    return -2;
  }

  if( logReady == false )
  {
    APP_LOG(TS_OFF, VLEVEL_H, "LOG: Logging is not possible\r\n");
    return -3;
  }

  if (MFM_data == 0)
  {
    APP_LOG(TS_OFF, VLEVEL_H, "LOG: MFM_data pointer is zero\r\n");
    return -4;
  }

  logdata.measurementId = newLogId; //set new log ID.
  logdata.timestamp = SysTimeGet().Seconds; //get system time, if time not yet in sync start from 0, otherwise unix timestamp

  logdata.protocolMFM = MFM_protocol;

  memcpy(&logdata.sensorModuleData, sensorModuleData, sizeof(logdata.sensorModuleData)); //copy sensor module data.

  //check if not all bytes are used in sensorModuleData buffer
  if( sensorModuleData->sensorModuleDataSize < sizeof(logdata.sensorModuleData) )
  {
    memset(&logdata.sensorModuleData.sensorModuleData[sensorModuleData->sensorModuleDataSize], 0xFF, sizeof(logdata.sensorModuleData.sensorModuleData) - sensorModuleData->sensorModuleDataSize); //fill in remaining empty sensor data.
  }

  logdata.sensorModuleData_crc = calculateCRC_CCITT(logdata.sensorModuleData.sensorModuleData, logdata.sensorModuleData.sensorModuleDataSize); //calculate CRC on sensordata

  memset( logdata.spare, 0xFF, sizeof(logdata.spare)); //set 0xFF (blank) in spare array

  turnoverAndErased = checkLogTurnoverAndErase(logdata.measurementId); //check dataflash ringbuffer is turnover and a block of 4k is erased.
  if( turnoverAndErased == true )
  {
    writeBackupRegister(BACKUP_REGISTER_OLDEST_LOG, readBackupRegister(BACKUP_REGISTER_OLDEST_LOG) + NUMBER_OF_PAGES_IN_4K_BLOCK_DATAFLASH);  //increment oldest pointer
  }

  result = writeLogInDataflash(logdata.measurementId, (uint8_t*)&logdata, sizeof(logdata)); //write new log to dataflash

  //check result
  if( result == 0 ) //success
  {
    APP_LOG(TS_OFF, VLEVEL_H, "Log ID %u written to dataflash\r\n", newLogId );
    newLogId++;
    writeBackupRegister(BACKUP_REGISTER_LATEST_LOG, newLogId);
  }

  else //failed
  {
    assert_param(1);
    APP_LOG(TS_OFF, VLEVEL_H, "Restore measurement ID failes\r\n" );
    return -5;//failed to write log
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
int8_t writeNewLog_old( uint8_t sensorModuleSlotId, uint8_t sensorModuleType, uint8_t protocol, uint8_t * sensorData, uint8_t dataLength )
{
  int8_t result;
  bool turnoverAndErased = false;

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

  logdata.sensorModuleData.sensorModuleSlotId = sensorModuleSlotId; //set sensor module slot ID
  logdata.sensorModuleData.sensorModuleTypeId = sensorModuleType; //set sensor module type
  logdata.sensorModuleData.sensorModuleProtocolId = protocol; //set sensor module protocol
  memcpy(logdata.sensorModuleData.sensorModuleData, sensorData, dataLength); //copy sensor data

  //check if not all bytes are used in sensorModuleData buffer
  if( dataLength < sizeof(logdata.sensorModuleData) )
  {
    memset(&logdata.sensorModuleData.sensorModuleData[dataLength], 0xFF, sizeof(logdata.sensorModuleData.sensorModuleData) - dataLength); //fill in remaining empty sensor data.
  }

  logdata.sensorModuleData.sensorModuleDataSize = dataLength; //set used datasize

  logdata.sensorModuleData_crc = calculateCRC_CCITT(logdata.sensorModuleData.sensorModuleData, logdata.sensorModuleData.sensorModuleDataSize); //calculate CRC on sensordata

  memset( logdata.spare, 0xFF, sizeof(logdata.spare)); //set 0xFF (blank) in spare array

  turnoverAndErased = checkLogTurnoverAndErase(logdata.measurementId); //check dataflash ringbuffer is turnover and a block of 4k is erased.
  if( turnoverAndErased == true )
  {
    writeBackupRegister(BACKUP_REGISTER_OLDEST_LOG, readBackupRegister(BACKUP_REGISTER_OLDEST_LOG) + NUMBER_OF_PAGES_IN_4K_BLOCK_DATAFLASH);  //increment oldest pointer
  }

  result = writeLogInDataflash(logdata.measurementId, (uint8_t*)&logdata, sizeof(logdata)); //write new log to dataflash

  //check result
  if( result == 0 ) //success
  {
    APP_LOG(TS_OFF, VLEVEL_H, "Log ID %u written to dataflash\r\n", newLogId );
    newLogId++;
    writeBackupRegister(BACKUP_REGISTER_LATEST_LOG, newLogId);
  }

  else //failed
  {
    assert_param(1);
    APP_LOG(TS_OFF, VLEVEL_H, "Restore measurement ID failes\r\n" );
    return -5;//failed to write log
  }

  return 0;
}

/**
 * @fn int8_t readLog(uint32_t, uint8_t*, uint32_t)
 * @brief function to read log from dataflash
 *
 * @param logId
 * @param buffer
 * @param bufferLength
 * @return
 */
int8_t readLog( uint32_t logId, uint8_t * buffer, uint32_t bufferLength )
{
  assert_param( buffer == 0);
  assert_param( bufferLength == 0);

  if (buffer == 0)
  {
    return -1;
  }

  if (bufferLength == 0 )
  {
    return -2;
  }

  return readLogFromDataflash(logId, buffer, bufferLength);
}

/**
 * @fn int32_t printLog(uint32_t, uint8_t*, uint32_t)
 * @brief function to print a log to a given buffer
 *
 * @param logId
 * @param buffer
 * @param bufferLength
 * @return
 */
int32_t printLog( uint32_t logId, uint8_t * buffer, uint32_t bufferLength )
{
  readLog(logId, (uint8_t*)&logdata, sizeof(logdata));

  int length = 0;

  length += snprintf((char*) buffer + length, bufferLength - length, "%lu;%lu;%u;", logdata.measurementId, logdata.timestamp, logdata.sensorModuleData.sensorModuleTypeId);

  if( length >= bufferLength )
    return -1;

  for (int i = 0; i < logdata.sensorModuleData.sensorModuleDataSize; i++)
  {
    length += snprintf((char*) buffer + length, bufferLength - length, "0x%02x", logdata.sensorModuleData.sensorModuleData[i]);

    if( length >= bufferLength )
        return -1;

    if (i < (logdata.sensorModuleData.sensorModuleDataSize - 1))
    {
      length += snprintf((char*) buffer + length, bufferLength, ",");
    }

    if( length >= bufferLength )
        return -1;
  }
  length += snprintf((char*) buffer + length, bufferLength - length, "\r\n");

  if( length >= bufferLength )
      return -1;

  return length;
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
 * @fn uint32_t getOldestLogId(void)
 * @brief function to return the oldest Log ID
 *
 * @return Oldest log ID.
 */
uint32_t getOldestLogId(void)
{
  if( newLogId < NUMBER_PAGES_FOR_LOGGING )
  {
    return 0;
  }
  else
  {
    //todo return real logging items, skip erased block,
    return NUMBER_PAGES_FOR_LOGGING;
  }
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
  uint32_t oldestId = readBackupRegister(BACKUP_REGISTER_OLDEST_LOG);
  if( latestId > NUMBER_PAGES_FOR_LOGGING )
  {
    if( latestId >= oldestId)
    {
      return latestId - oldestId;
    }
    else
    {
      APP_LOG(TS_OFF, VLEVEL_H, "latestId is smaller then oldest, not possible!\r\n" );
      return NUMBER_PAGES_FOR_LOGGING;
    }
  }
  else
  {
    return latestId;
  }
}

/**
 * @fn int8_t eraseCompleteLog(void)
 * @brief function to erase all logs depending on reserverd memory of logs it choose the right block size to erase.
 *
 * @return >= 0 okay, < 0 error not erased.
 */
const uint8_t eraseCompleteLog( void )
{
  int returnValue = -1;

  if( NUMBER_PAGES_FOR_LOGGING == NUMBER_PAGES_DATAFLASH ) //complete flash is used, chip erase can be executed
  {
    chipEraseDataflash();
    returnValue = 0;
  }
  else if( (NUMBER_PAGES_FOR_LOGGING % NUMBER_OF_PAGES_IN_64K_BLOCK_DATAFLASH) == 0 )
  {
    for(int i = 0; i< NUMBER_PAGES_FOR_LOGGING/NUMBER_OF_PAGES_IN_64K_BLOCK_DATAFLASH; i++)
    {
      blockErase64kDataflash( i * PAGE_SIZE_DATAFLASH * NUMBER_OF_PAGES_IN_64K_BLOCK_DATAFLASH );
    }
    returnValue = 0;
  }
  else if( (NUMBER_PAGES_FOR_LOGGING % NUMBER_OF_PAGES_IN_32K_BLOCK_DATAFLASH) == 0 )
  {
    for(int i = 0; i< NUMBER_PAGES_FOR_LOGGING/NUMBER_OF_PAGES_IN_32K_BLOCK_DATAFLASH; i++)
    {
      blockErase32kDataflash( i * PAGE_SIZE_DATAFLASH * NUMBER_OF_PAGES_IN_32K_BLOCK_DATAFLASH );
    }
    returnValue = 0;
  }
  else if( (NUMBER_PAGES_FOR_LOGGING % NUMBER_OF_PAGES_IN_4K_BLOCK_DATAFLASH) == 0 )
  {
    for(int i = 0; i< NUMBER_PAGES_FOR_LOGGING/NUMBER_OF_PAGES_IN_4K_BLOCK_DATAFLASH; i++)
    {
      blockErase4kDataflash( i * PAGE_SIZE_DATAFLASH * NUMBER_OF_PAGES_IN_4K_BLOCK_DATAFLASH );
    }
    returnValue = 0;
  }
  else
  {
    //error
    APP_LOG(TS_OFF, VLEVEL_L, "Log erase not possible\r\n");
    returnValue = -1;
  }

  if( returnValue >= 0 )
  {
    writeBackupRegister(BACKUP_REGISTER_LATEST_LOG, 0);
    writeBackupRegister(BACKUP_REGISTER_OLDEST_LOG, 0);
    newLogId = 0;
  }

  return returnValue;

}

/**
 * @fn int8_t eraseCompleteLog(void)
 * @brief function to erase all logs depending on reserverd memory of logs it choose the right block size to erase.
 *
 * @return >= 0 okay, < 0 error not erased.
 */
const int8_t eraseCompleteLogBlockByBlock( uint32_t * startAddress )
{
  int returnValue = -1;

  if( NUMBER_PAGES_FOR_LOGGING == NUMBER_PAGES_DATAFLASH ) //complete flash is used, chip erase can be executed
  {
    chipEraseDataflash();
    *startAddress += NUMBER_PAGES_DATAFLASH * PAGE_SIZE_DATAFLASH;
    returnValue = 0;
  }
  else if( (NUMBER_PAGES_FOR_LOGGING % NUMBER_OF_PAGES_IN_64K_BLOCK_DATAFLASH) == 0 )
  {

    blockErase64kDataflash( *startAddress );
    *startAddress += NUMBER_OF_PAGES_IN_64K_BLOCK_DATAFLASH * PAGE_SIZE_DATAFLASH;

    returnValue = 0;
  }
  else if( (NUMBER_PAGES_FOR_LOGGING % NUMBER_OF_PAGES_IN_32K_BLOCK_DATAFLASH) == 0 )
  {

    blockErase32kDataflash( *startAddress );
    *startAddress += NUMBER_OF_PAGES_IN_32K_BLOCK_DATAFLASH  * PAGE_SIZE_DATAFLASH;

    returnValue = 0;
  }
  else if( (NUMBER_PAGES_FOR_LOGGING % NUMBER_OF_PAGES_IN_4K_BLOCK_DATAFLASH) == 0 )
  {

    blockErase4kDataflash( *startAddress );
    *startAddress += NUMBER_OF_PAGES_IN_4K_BLOCK_DATAFLASH  * PAGE_SIZE_DATAFLASH;

    returnValue = 0;
  }
  else
  {
    //error
    APP_LOG(TS_OFF, VLEVEL_L, "Log erase not possible\r\n");
    returnValue = -1;
  }

  if( returnValue >= 0 )
  {
    if( *startAddress >= NUMBER_PAGES_FOR_LOGGING * PAGE_SIZE_DATAFLASH )
    {
      writeBackupRegister(BACKUP_REGISTER_LATEST_LOG, 0);
      writeBackupRegister(BACKUP_REGISTER_OLDEST_LOG, 0);
      newLogId = 0;
      returnValue = 1;
    }
  }


  return returnValue;
}

const uint getProgressFromAddress( uint32_t address )
{
  return (address * 100) / (NUMBER_PAGES_FOR_LOGGING * PAGE_SIZE_DATAFLASH);
}
