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
#define UNIX_TIME_START_APP 1700000000L

const void syncRTC_withSysTime(void);
const void syncSystemTime_withRTC(void);
const void convert_am1805time_to_dateTime(am1805_time_t * timeSrc, struct_dateTime * timeDst );
const bool getForceSleepStatus(void);
const void setWakeupWdtAlarm( uint32_t seconds );
uint8_t getStatusRegisterRtc(void);
uint8_t readStatusRegisterRtc(void);
const bool getWakeupWdtStatus(bool clear);
const bool getWakeupAlarmStatus(bool clear);
const bool getWakeupBatStatus(bool clear);
const bool getWakeupEx1Status(bool clear);
const bool getWakeupEx2Status(bool clear);
const void goIntoSleep(uint32_t sleepTime_sec, uint8_t waitTimeTicks);
const void disableSleep(void);
const uint32_t get_current_alarm(void);

#endif /* RTC_AM1805_RTC_FUNCTIONS_H_ */
