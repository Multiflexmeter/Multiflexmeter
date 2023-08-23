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
    printf("ReadMID Success.\n");
  }
  else
  {
    printf("ReadMID fail.\n");
    return -1;
  }

  return 0;
}
