#if !defined(_CONFIG_H_)
#define _CONFIG_H_

#include "board.h"

/*
  Generic configuration
*/
// Whether to output debug information to the serial output
#define DEBUG
// The lowest DR (thus highest SF) the device will join and TX at
#define MIN_LORA_DR 0
// Whether to print the firmware its build date/time to the serial on power on
#define PRINT_BUILD_DATE_TIME
// Milliseconds before measurement times out to prevent hanging
#define SENSOR_JSN_TIMEOUT 200
// Minimum and Maximum send intervals are hardcoded limits
#define MIN_INTERVAL 900  // 15 minutes
#define MAX_INTERVAL 4270 // 2 hours

#endif // _CONFIG_H_
