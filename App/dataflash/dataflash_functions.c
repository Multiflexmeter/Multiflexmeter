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

#define PAGE_SIZE_DATAFLASH  0x100
#define MAX_ADDRESS_OF_DATAFLASH  0xFFFFFF
#define RESERVED_MEMORY   (PAGE_SIZE_DATAFLASH * 32)
#define MAX_LOG_ADDRESS_OF_DATAFLASH  (MAX_ADDRESS_OF_DATAFLASH - RESERVED_MEMORY)

static uint8_t MID[] = {0x1F, 0x88, 0x01};
static uint8_t dataRead[MAXIMUM_BUFFER_SIZE] = {0};

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
  standardflashBytePageProgram((uint32_t)pageAddress, data, length);

  return 0;
}


