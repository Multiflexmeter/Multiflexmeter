/*
 * dataflash_functions.h
 *
 *  Created on: 23 aug. 2023
 *      Author: p.kwekkeboom
 */

#ifndef DATAFLASH_DATAFLASH_FUNCTIONS_H_
#define DATAFLASH_DATAFLASH_FUNCTIONS_H_

int8_t init_dataflash(void);
int8_t writeLogInDataflash(uint32_t logId, uint8_t * data, uint32_t length);

#endif /* DATAFLASH_DATAFLASH_FUNCTIONS_H_ */
