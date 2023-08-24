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

#define PAGE_SIZE_DATAFLASH         ( 0x100 )
#define BLOCK_4K_SIZE_DATAFLASH     ( 0x1000 )
#define BLOCK_32K_SIZE_DATAFLASH    ( 0x8000 )
#define BLOCK_64K_SIZE_DATAFLASH    ( 0x10000UL )

#define NUMBER_PAGES_DATAFLASH      ( 32768 )
#define NUMBER_4K_BLOCK_DATAFLASH   ( 2048 )
#define NUMBER_32K_BLOCK_DATAFLASH  ( 256 )
#define NUMBER_64K_BLOCK_DATAFLASH  ( 128 )

#define NUMBER_OF_PAGES_IN_4K_BLOCK_DATAFLASH  ( BLOCK_4K_SIZE_DATAFLASH / PAGE_SIZE_DATAFLASH )
#define NUMBER_OF_PAGES_IN_32K_BLOCK_DATAFLASH  ( BLOCK_32K_SIZE_DATAFLASH / PAGE_SIZE_DATAFLASH )
#define NUMBER_OF_PAGES_IN_62K_BLOCK_DATAFLASH  ( BLOCK_62K_SIZE_DATAFLASH / PAGE_SIZE_DATAFLASH )


#define MAX_ADDRESS_OF_DATAFLASH  0xFFFFFF
#define RESERVED_MEMORY   (PAGE_SIZE_DATAFLASH * 32)
#define MAX_LOG_ADDRESS_OF_DATAFLASH  (MAX_ADDRESS_OF_DATAFLASH - RESERVED_MEMORY)

static uint8_t MID[] = {0x1F, 0x88, 0x01};
static uint8_t dataRead[MAXIMUM_BUFFER_SIZE] = {0};


static uint8_t writePageBuffer[PAGE_SIZE_DATAFLASH];
static uint8_t readPageBuffer[PAGE_SIZE_DATAFLASH];
static uint8_t block4kBuffer[BLOCK_4K_SIZE_DATAFLASH];

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
 * @fn int8_t writeLogInDataflash(uint32_t, uint8_t*, uint32_t)
 * @brief function to write a log item "logId" in the dataflash ringbuffer.
 *
 * @param logId log nummer starts from 0 - 4294967296
 * @param data array of data
 * @param length of data array to write, limit to 256 (\ref PAGE_SIZE_DATAFLASH).
 * @return
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
  pageAddress %= MAX_LOG_ADDRESS_OF_DATAFLASH;

  //enable write
  standardflashWriteEnable();

  //write data
  standardflashBytePageProgram((uint32_t)pageAddress, data, length);

  //wait for ready
  standardflashWaitOnReady();

  return 0;
}

/**
 * @fn int8_t readLogFromDataflash(uint32_t, uint8_t*, uint32_t)
 * @brief function to read a log from flash
 *
 * @param logId
 * @param data
 * @param length
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
  pageAddress %= MAX_LOG_ADDRESS_OF_DATAFLASH;

  standardflashReadArrayLowFreq(pageAddress, data, length);

  return 0;
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
 * @fn int8_t clearLogInDataflash(uint32_t)
 * @brief function to clear a 4k block of the logId
 *
 * @param logId sequence number of log.
 * @return 0 is succesful
 */
int8_t clearLogInDataflash(uint32_t logId)
{
  uint64_t pageAddress = (logId * PAGE_SIZE_DATAFLASH);
  pageAddress %= MAX_LOG_ADDRESS_OF_DATAFLASH;

  blockEraseDataflash((uint32_t)pageAddress);

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
    readLogFromDataflash(blockNumber, block4kBuffer, sizeof(block4kBuffer));

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
      writeLogInDataflash((blockNumber * NUMBER_OF_PAGES_IN_4K_BLOCK_DATAFLASH) + i , writePageBuffer, sizeof(writePageBuffer));

      //read page
      readLogFromDataflash((blockNumber * NUMBER_OF_PAGES_IN_4K_BLOCK_DATAFLASH) + i , readPageBuffer, sizeof(readPageBuffer));

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
          writeLogInDataflash((blockNumber * NUMBER_OF_PAGES_IN_4K_BLOCK_DATAFLASH) + i , writePageBuffer, sizeof(writePageBuffer));

          //read page
          readLogFromDataflash((blockNumber * NUMBER_OF_PAGES_IN_4K_BLOCK_DATAFLASH) + i , readPageBuffer, sizeof(readPageBuffer));

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
