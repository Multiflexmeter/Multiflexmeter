/**
  ******************************************************************************
  * @file           : board_io_functions.h
  * @brief          : Header for board_io_functions.c file.
  * @author         : P.Kwekkeboom
  * @date           : Nov 22, 2023
  ******************************************************************************
  */

#ifndef IO_BOARD_IO_FUNCTIONS_H_
#define IO_BOARD_IO_FUNCTIONS_H_

typedef enum
{
  sensorSlot1,
  sensorSlot2,
  sensorSlot3,
  sensorSlot4,
  sensorSlot5,
  sensorSlot6,
  MAX_SENSOR_SLOT,
} ENUM_slotId;


const bool getLigthSensorStatus(void);
const void enableVsys(void);
const void disableVsys(void);

const void FRAM_EnableChipSelect(void);
const void FRAM_DisableChipSelect(void);
const void SD_EnableChipSelect(void);
const void SD_DisableChipSelect(void);
const void dataflash_EnableChipSelect(void);
const void dataflash_DisableChipSelect(void);




#endif /* IO_BOARD_IO_FUNCTIONS_H_ */
