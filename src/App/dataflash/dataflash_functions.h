/*
 * dataflash_functions.h
 *
 *  Created on: 23 aug. 2023
 *      Author: p.kwekkeboom
 */

#ifndef DATAFLASH_DATAFLASH_FUNCTIONS_H_
#define DATAFLASH_DATAFLASH_FUNCTIONS_H_

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
#define NUMBER_OF_PAGES_IN_64K_BLOCK_DATAFLASH  ( BLOCK_64K_SIZE_DATAFLASH / PAGE_SIZE_DATAFLASH )

#define MAX_ADDRESS_OF_DATAFLASH      ( 0x7FFFFFUL )
#define NUMBER_PAGES_FOR_MEASUREMENTS      ( NUMBER_PAGES_DATAFLASH - NUMBER_OF_PAGES_IN_32K_BLOCK_DATAFLASH )
#define NUMBER_OF_RESERVED_PAGES      ( NUMBER_PAGES_DATAFLASH - NUMBER_PAGES_FOR_MEASUREMENTS )
#define MEASUREMENT_MEMEORY_SIZE              ( PAGE_SIZE_DATAFLASH * NUMBER_PAGES_FOR_MEASUREMENTS )
#define RESERVED_MEMORY               ( PAGE_SIZE_DATAFLASH * NUMBER_OF_RESERVED_PAGES )

int8_t init_dataflash(void);
int8_t writeMeasurementInDataflash(uint32_t logId, uint8_t * data, uint32_t length);
bool checkMeasurementMemoryTurnoverAndErase(uint32_t logId);
int8_t readPageFromDataflash(uint32_t pageAddress, uint8_t * data, uint32_t length);
int8_t readMeasurementFromDataflash(uint32_t logId, uint8_t * data, uint32_t length);
int8_t blockErase4kDataflash( uint32_t address );
int8_t blockErase32kDataflash( uint32_t address );
int8_t blockErase64kDataflash( uint32_t address );
const int8_t chipEraseDataflash(void);
int8_t testCompleteDataflash(bool restoreOrinalData );
const int8_t testDataflash(uint8_t test, uint32_t * status);

#endif /* DATAFLASH_DATAFLASH_FUNCTIONS_H_ */
