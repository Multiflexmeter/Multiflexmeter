/**
  ******************************************************************************
  * @file           : RTC_functions.h
  * @brief          : Header for RTC_functions.c file.
  * @author         : P.Kwekkeboom
  * @date           : Oct 26, 2023
  ******************************************************************************
  */

#ifndef RTC_AM1805_RTC_FUNCTIONS_H_
#define RTC_AM1805_RTC_FUNCTIONS_H_

#include "am1805.h"

#define START_YEAR  2000

const void syncRTC_withSysTime(void);
const void syncSystemTime_withRTC(void);
const void convert_am1805time_to_dateTime(am1805_time_t * timeSrc, struct_dateTime * timeDst );
const void setWakeupAlarm( uint32_t seconds );
const bool getWakeupStatus(bool clear);
const void goIntoSleep(uint32_t sleepTime_sec, uint8_t waitTimeTicks);
const void disableSleep(void);

#endif /* RTC_AM1805_RTC_FUNCTIONS_H_ */
