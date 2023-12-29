/**
  ******************************************************************************
  * @file           : board_io_test.h
  * @brief          : Header for board_io_test.c file.
  * @author         : P.Kwekkeboom
  * @date           : Nov 22, 2023
  ******************************************************************************
  */

#ifndef IO_BOARD_IO_TEST_H_
#define IO_BOARD_IO_TEST_H_

const void testOutput_board_io(uint8_t item, bool state);
const int8_t testInput_board_io(uint8_t item);
const void testSystemChecks( int mode, int32_t value );
const int8_t checkSpiPullupsVsystem(void);

#endif /* IO_BOARD_IO_TEST_H_ */
