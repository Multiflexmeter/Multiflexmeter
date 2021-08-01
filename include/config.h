#if !defined(_CONFIG_H_)
#define _CONFIG_H_

// Include the correct board configuration settings
#if defined(BOARD_MFM_V3)
#include "board_config/mfm_v3.h"
#elif defined(BOARD_MFM_V3_M1284P)
#include "board_config/mfm_v3_m1284p.h"
#endif

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
#define MIN_INTERVAL 20   // 15 minutes
#define MAX_INTERVAL 4270 // 2 hours

#endif // _CONFIG_H_
