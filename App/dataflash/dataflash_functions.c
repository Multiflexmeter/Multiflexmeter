/*
 * dataflash_functions.c
 *
 *  Created on: 23 aug. 2023
 *      Author: p.kwekkeboom
 */

#include "spi_driver.h"
#include "standardflash.h"
#include "helper_functions.h"
#include "dataflash_functions.h"

static uint8_t MID[] = {0x1F, 0x88, 0x01};
static uint8_t dataRead[MAXIMUM_BUFFER_SIZE] = {0};


static uint8_t writePageBuffer[PAGE_SIZE_DATAFLASH];
static uint8_t readPageBuffer[PAGE_SIZE_DATAFLASH];
static uint8_t block4kBuffer[BLOCK_4K_SIZE_DATAFLASH];

int8_t blockEraseDataflash( uint32_t address );

/**
 * @fn int init_dataflash(void)
 * @brief function to initialize the Dataflash I/O and read the Manufacturer ID
 *
 * @return 0 = success, -1 = failed.
 */
int8_t init_dataflash(void)
{
  // initialize the I/O
  SPI_ConfigureSingleSPIIOs();

  //read the MD to verify dataflash
  standardflashReadMID(dataRead);

  //check MID is okay
  if (compareByteArrays(dataRead, MID, 3))
  {
    printf("Dataflash: ReadMID Success.\n");
  }
  else
  {
    printf("Dataflash: ReadMID fail.\n");
    return -1;
  }

  return 0;
}

/**
 * @fn int8_t writePageInDataflash(uint32_t, uint8_t*, uint32_t)
 * @brief function to write a page in dataflash
 *
 * @param pageAddress : start address of page, must be align on page size \ref PAGE_SIZE_DATAFLASH
 * @param data : pointer to data buffer to write, no zero.
 * @param length : length of data to write, must be > 0
 * @return 0 = successful, -1 = data pointer is zero, -2 = length is zero, -3 = length is larger then one page size \ref PAGE_SIZE_DATAFLASH
 */
int8_t writePageInDataflash(uint32_t pageAddress, uint8_t * data, uint32_t length)
{
  assert_param(data != 0 ); //check pointer is not zero
  assert_param(length != 0 ); //check length is not zero
  assert_param(length <= PAGE_SIZE_DATAFLASH ); //check length is not too large

  if( data == 0 ) //check pointer is zero
  {
    return -1;
  }

  if( length == 0 ) //check length is zero
  {
    return -2;
  }

  if( length > PAGE_SIZE_DATAFLASH ) //check length is larger then 256
  {
    return -3;
  }

  //enable write
  standardflashWriteEnable();

  //write data
  standardflashBytePageProgram((uint32_t)pageAddress, data, length);

  //wait for ready
  standardflashWaitOnReady();

  return 0;
}

/**
 * @fn int8_t writeLogInDataflash(uint32_t, uint8_t*, uint32_t)
 * @brief function to write a log item "logId" in the dataflash ringbuffer.
 *
 * @param logId log nummer starts from 0 - 4294967296
 * @param data array of data
 * @param length of data array to write, limit to 256 (\ref PAGE_SIZE_DATAFLASH).
 * @return 0 = successful, -1 = data pointer is zero, -2 = length is zero, -3 = length is larger then one page size \ref PAGE_SIZE_DATAFLASH
 */
int8_t writeLogInDataflash(uint32_t logId, uint8_t * data, uint32_t length)
{
  assert_param(data != 0 ); //check pointer is not zero
  assert_param(length != 0 ); //check length is not zero
  assert_param(length <= PAGE_SIZE_DATAFLASH ); //check length is not too large

  if( data == 0 ) //check pointer is zero
  {
    return -1;
  }

  if( length == 0 ) //check length is zero
  {
    return -2;
  }

  if( length > PAGE_SIZE_DATAFLASH ) //check length is larger then 256
  {
    return -3;
  }

  uint64_t pageAddress = (logId * PAGE_SIZE_DATAFLASH);
  pageAddress %= LOG_MEMEORY_SIZE;

  //check logging in turnover
  if( logId >= NUMBER_PAGES_FOR_LOGGING )
  {
    //check pageAddress is first of new block
    if( (pageAddress % 0x1000) == 0)
    {
      //first erase next block
      blockEraseDataflash(pageAddress);
    }
  }

  return writePageInDataflash((uint32_t)pageAddress, data, length);
}

/**
 * @fn int8_t readPageFromDataflash(uint32_t, uint8_t*, uint32_t)
 * @brief function to read a page from flash
 *
 * @param pageAddress : start address of page, must be align on page size \ref PAGE_SIZE_DATAFLASH
 * @param data : pointer to data buffer to write, no zero.
 * @param length : length of data to write, must be > 0
 * @return 0 = successful, -1 = data pointer is zero, -2 = length is zero.
 */
int8_t readPageFromDataflash(uint32_t pageAddress, uint8_t * data, uint32_t length)
{
  assert_param(data != 0 ); //check pointer is not zero
  assert_param(length != 0 ); //check length is not zero

  if( data == 0 ) //check pointer is zero
  {
    return -1;
  }

  if( length == 0 ) //check length is zero
  {
    return -2;
  }

  standardflashReadArrayLowFreq(pageAddress, data, length);

  return 0;
}

/**
 * @fn int8_t readLogFromDataflash(uint32_t, uint8_t*, uint32_t)
 * @brief function to read a log from flash
 *
 * @param logId : number of log record to write
 * @param data : pointer to data buffer to write
 * @param length : length of data to write, must be > 0
 * @return < 0 is error, 0 is success.
 */
int8_t readLogFromDataflash(uint32_t logId, uint8_t * data, uint32_t length)
{
  assert_param(data != 0 ); //check pointer is not zero
  assert_param(length != 0 ); //check length is not zero

  if( data == 0 ) //check pointer is zero
  {
    return -1;
  }

  if( length == 0 ) //check length is zero
  {
    return -2;
  }

  uint64_t pageAddress = (logId * PAGE_SIZE_DATAFLASH);
  pageAddress %= LOG_MEMEORY_SIZE;

  return readPageFromDataflash(pageAddress, data, length);
}

/**
 * @fn int8_t blockEraseDataflash(uint32_t)
 * @brief function to erase a block of 4k
 *
 * @param address of memory in block
 * @return 0 is successful
 */
int8_t blockEraseDataflash( uint32_t address )
{
  //enable write
  standardflashWriteEnable();

  //block erase
  standardflashBlockErase4K( address );

  //wait ready
  standardflashWaitOnReady();

  return 0;
}

/**
 * @fn int8_t chipEraseDataflash(void)
 * @brief function to execute a complete chip erase
 *
 * @return 0 = successful
 */
const int8_t chipEraseDataflash(void)
{
  //enable write
  standardflashWriteEnable();

  //block erase
  standardflashChipErase1();

  //wait ready
  standardflashWaitOnReady();

  return 0;
}

/**
 * @fn int8_t clearLogInDataflash(uint32_t)
 * @brief function to clear a 4k block of the logId
 *
 * @param logId sequence number of log.
 * @return 0 is succesful
 */
int8_t clearLogInDataflash(uint32_t logId)
{
  uint64_t pageAddress = (logId * PAGE_SIZE_DATAFLASH);
  pageAddress %= LOG_MEMEORY_SIZE;

  blockEraseDataflash((uint32_t)pageAddress);

  return 0;
}

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

  UNION_logdata * pLog = (UNION_logdata *)readPageBuffer;

  //read first record
  readLogFromDataflash(0, readPageBuffer, sizeof(readPageBuffer));
  firstRecordMeasurementId = pLog->log.measurementId;

  //read latest record
  readLogFromDataflash(NUMBER_PAGES_FOR_LOGGING - 1, readPageBuffer, sizeof(readPageBuffer));
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
    readLogFromDataflash(newReadingId, readPageBuffer, sizeof(readPageBuffer));

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
          readLogFromDataflash(newReadingId + 1, readPageBuffer, sizeof(readPageBuffer));
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
 * @fn int8_t testDataflash(bool)
 * @brief test function to verify the whole dataflash.
 * Note: complete test takes a lot of time
 *
 * @param restoreOrinalData : true restores old data, false keep the data erased
 * @return
 */
int8_t testDataflash(bool restoreOrinalData )
{
  assert_param( sizeof(writePageBuffer) == sizeof(readPageBuffer)); //check buffer size is equal

  uint16_t blockNumber;
  int i;
  bool tDectectNotEmpty = false;
  bool tDectectError = false;

  for (blockNumber = 0; blockNumber < NUMBER_4K_BLOCK_DATAFLASH; blockNumber++)
  {
    tDectectNotEmpty = false; //reset at new block

    //fill page buffer with empth value dataflash
    memset(writePageBuffer, 0xff, sizeof(writePageBuffer));

    //read block from dataflash
    readPageFromDataflash(blockNumber, block4kBuffer, sizeof(block4kBuffer));

    //do it for a each page in a block
    for (i = 0; i < NUMBER_OF_PAGES_IN_4K_BLOCK_DATAFLASH; i++)
    {
      //verify if a page contains only blank fields (0xFF)
      if (memcmp(&block4kBuffer[i*PAGE_SIZE_DATAFLASH], writePageBuffer, sizeof(writePageBuffer)) == 0)
      {
        APP_LOG(TS_OFF, VLEVEL_H, "Page %d is empty.\r\n", (blockNumber * NUMBER_OF_PAGES_IN_4K_BLOCK_DATAFLASH) + i );
      }
      else
      {
        APP_LOG(TS_OFF, VLEVEL_M, "Page %d is not empty.\r\n", (blockNumber * NUMBER_OF_PAGES_IN_4K_BLOCK_DATAFLASH) + i );
        tDectectNotEmpty = true;
      }
    }

    if ( tDectectNotEmpty == false )
    {
      APP_LOG(TS_OFF, VLEVEL_H, "Block %d is empty.\r\n", blockNumber);
    }
    else
    {
      APP_LOG(TS_OFF, VLEVEL_M, "Block %d is not empty.\r\n", blockNumber);

      //block erase
      blockEraseDataflash((uint32_t)blockNumber*BLOCK_4K_SIZE_DATAFLASH);
    }

    //set a test value
    memset(writePageBuffer, 0xAA, sizeof(writePageBuffer));

    //do it for a each page in a block
    for (i = 0; i < NUMBER_OF_PAGES_IN_4K_BLOCK_DATAFLASH; i++)
    {
      //write page
      writePageInDataflash(((blockNumber * NUMBER_OF_PAGES_IN_4K_BLOCK_DATAFLASH) + i) * PAGE_SIZE_DATAFLASH , writePageBuffer, sizeof(writePageBuffer));

      //read page
      readPageFromDataflash(((blockNumber * NUMBER_OF_PAGES_IN_4K_BLOCK_DATAFLASH) + i) * PAGE_SIZE_DATAFLASH, readPageBuffer, sizeof(readPageBuffer));

      //verify page
      if (memcmp(readPageBuffer, writePageBuffer, sizeof(writePageBuffer)) == 0)
      {
        APP_LOG(TS_OFF, VLEVEL_H, "Test page %d is okay.\r\n", (blockNumber * NUMBER_OF_PAGES_IN_4K_BLOCK_DATAFLASH) + i );
      }
      else
      {
        APP_LOG(TS_OFF, VLEVEL_M, "Error in test Page %d\r\n", (blockNumber * NUMBER_OF_PAGES_IN_4K_BLOCK_DATAFLASH) + i );
        tDectectError = true;
      }
    }

    APP_LOG(TS_OFF, VLEVEL_M, "Testing block %d\r\n", blockNumber );

    //block erase
    blockEraseDataflash((uint32_t)blockNumber*BLOCK_4K_SIZE_DATAFLASH);

    APP_LOG(TS_OFF, VLEVEL_H, "Test page %d is okay.\r\n", (blockNumber * NUMBER_OF_PAGES_IN_4K_BLOCK_DATAFLASH) + i );

    //check if original data need to be restored.
    if( restoreOrinalData == true && tDectectNotEmpty == true )
    {
      //copy back original data
      for (i = 0; i < NUMBER_OF_PAGES_IN_4K_BLOCK_DATAFLASH; i++)
      {
        //fill page buffer with empth value dataflash
        memset(writePageBuffer, 0xff, sizeof(writePageBuffer));

        //verify if a page contains only blank fields (0xFF)
        if (memcmp(&block4kBuffer[i*PAGE_SIZE_DATAFLASH], writePageBuffer, sizeof(writePageBuffer)) == 0)
        {
          APP_LOG(TS_OFF, VLEVEL_H, "Restore page %d is empty. No need to write it back\r\n", (blockNumber * NUMBER_OF_PAGES_IN_4K_BLOCK_DATAFLASH) + i );
        }
        else
        {
          APP_LOG(TS_OFF, VLEVEL_H, "Page %d is not empty. Restore data.\r\n", (blockNumber * NUMBER_OF_PAGES_IN_4K_BLOCK_DATAFLASH) + i );

          //copy data to write buffer
          memcpy(writePageBuffer, &block4kBuffer[i*PAGE_SIZE_DATAFLASH], sizeof(writePageBuffer));

          //write page
          writePageInDataflash(((blockNumber * NUMBER_OF_PAGES_IN_4K_BLOCK_DATAFLASH) + i) * PAGE_SIZE_DATAFLASH , writePageBuffer, sizeof(writePageBuffer));

          //read page
          readPageFromDataflash(((blockNumber * NUMBER_OF_PAGES_IN_4K_BLOCK_DATAFLASH) + i) * PAGE_SIZE_DATAFLASH , readPageBuffer, sizeof(readPageBuffer));

          //verify page
          if (memcmp(&readPageBuffer, writePageBuffer, sizeof(writePageBuffer)) == 0)
          {
            APP_LOG(TS_OFF, VLEVEL_H, "Restore page %d is okay.\r\n", (blockNumber * NUMBER_OF_PAGES_IN_4K_BLOCK_DATAFLASH) + i );
          }
          else
          {
            APP_LOG(TS_OFF, VLEVEL_M, "Error at restore data in Page %d\r\n", (blockNumber * NUMBER_OF_PAGES_IN_4K_BLOCK_DATAFLASH) + i );
            tDectectError = true;
          }
        }
      }
    }
    else
    {
      //nothing
    }

  }

  if( tDectectError )
  {
    return -1;
  }

  return 0;

}
