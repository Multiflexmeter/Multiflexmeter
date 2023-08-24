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
#define NUMBER_OF_PAGES_IN_62K_BLOCK_DATAFLASH  ( BLOCK_62K_SIZE_DATAFLASH / PAGE_SIZE_DATAFLASH )


#define MAX_ADDRESS_OF_DATAFLASH  0xFFFFFF
#define RESERVED_MEMORY   (PAGE_SIZE_DATAFLASH * 32)
#define MAX_LOG_ADDRESS_OF_DATAFLASH  (MAX_ADDRESS_OF_DATAFLASH - RESERVED_MEMORY)

int8_t init_dataflash(void);
int8_t writeLogInDataflash(uint32_t logId, uint8_t * data, uint32_t length);
int8_t readLogFromDataflash(uint32_t logId, uint8_t * data, uint32_t length);

int8_t testDataflash(bool restoreOrinalData );

#endif /* DATAFLASH_DATAFLASH_FUNCTIONS_H_ */
