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

#define START_YEAR  2000

const void syncRTC_withSysTime(void);
const void syncSystemTime_withRTC(void);

#endif /* RTC_AM1805_RTC_FUNCTIONS_H_ */
