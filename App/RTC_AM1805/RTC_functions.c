/**
  ******************************************************************************
  * @addtogroup     : App
  * @{
  * @file           : RTCfunctions.c
  * @brief          : RTC functions
  * @author         : P.Kwekkeboom
  * @date           : Oct 26, 2023
  * @}
  ******************************************************************************
  */

#include "main.h"
#include "sys_app.h"

#include "timer_if.h"

#include "../common/common.h"
#include "am1805.h"
#include "RTC_functions.h"


/**
 * @fn const void syncRTC_withSysTime(void)
 * @brief function to sync external RTC with internal systime
 *
 */
const void syncRTC_withSysTime(void)
{
  SysTime_t sysTime = {0};
  struct tm stTime = {0};
  am1805_time_t timeWrite = { 0 };
  am1805_time_t timeRead = { 0 };

  sysTime = SysTimeGet(); //get internal RTC value

  SysTimeLocalTime(sysTime.Seconds, &stTime); //convert to date time

  timeWrite.ui8Mode = AM1805_24HR_MODE;
  timeWrite.ui8Century = ((stTime.tm_year / 100)) % 2;
  timeWrite.ui8Date = stTime.tm_mday;
  timeWrite.ui8Month = stTime.tm_mon + 1;
  timeWrite.ui8Year = stTime.tm_year % 100;
  timeWrite.ui8Hour = stTime.tm_hour;
  timeWrite.ui8Minute = stTime.tm_min;
  timeWrite.ui8Second = stTime.tm_sec;
  timeWrite.ui8Weekday = 0;
  timeWrite.ui8Hundredth = sysTime.SubSeconds/10;

  am1805_time_set(timeWrite, 1); //save date/time to external RTC
  am1805_time_get(&timeRead); //read back to verify, for debug

}

/**
 * @fn const void syncSystemTime_withRTC(void)
 * @brief function to sync internal system with external RTC
 *
 */
const void syncSystemTime_withRTC(void)
{
  am1805_time_t timeRead = { 0 };
  SysTime_t sysTime = {0};
  struct tm stTime = {0};

  am1805_time_get(&timeRead);

  stTime.tm_year = timeRead.ui8Year + 100;
  stTime.tm_mon = timeRead.ui8Month - 1;
  stTime.tm_mday = timeRead.ui8Date;

  stTime.tm_hour = timeRead.ui8Hour;
  stTime.tm_min = timeRead.ui8Minute;
  stTime.tm_sec = timeRead.ui8Second;

  sysTime.Seconds = SysTimeMkTime(&stTime);
  sysTime.SubSeconds = timeRead.ui8Hundredth*10; //multiply hundredth with 10 to get subseconds in milliseconds

  SysTimeSet(sysTime); //save date/time to internal RTC
  sysTime = SysTimeGet(); //read back to verify, for debug

}

/**
 * @fn const void testRTC(int, struct_dateTime*)
 * @brief
 * @todo make compatible for years from 2100.
 *
 * @param mode : 1 = write RTC, 2 = read RTC, 3 = sync RTC to systemtime, 4 = sync systemtime to RTC
 * @param time
 */
const void testRTC( int mode, struct_dateTime * time )
{
  am1805_time_t timeReadOriginal = { 0 };
  am1805_time_t timeRead = { 0 };
  am1805_time_t timeWrite = { 0 };

  am1805_time_get(&timeReadOriginal); //read time

  if( mode == 1 ) //write mode
  {
    timeWrite.ui8Mode = AM1805_24HR_MODE;
    timeWrite.ui8Century = ((time->year / 100)) % 2 + 1 ;
    timeWrite.ui8Date = time->day;
    timeWrite.ui8Month = time->month;
    timeWrite.ui8Year = time->year % 100;
    timeWrite.ui8Hour = time->hour;
    timeWrite.ui8Minute = time->minute;
    timeWrite.ui8Second = time->second;
    timeWrite.ui8Weekday = 0;
    timeWrite.ui8Hundredth = 0;

    am1805_time_set(timeWrite, 1);

    am1805_time_get(&timeRead);

    time->day = timeRead.ui8Date;
    time->month = timeRead.ui8Month;
    time->year = timeRead.ui8Year + START_YEAR;
    time->hour = timeRead.ui8Hour;
    time->minute = timeRead.ui8Minute;
    time->second = timeRead.ui8Second;
    time->century = timeRead.ui8Century;
  }

  else if (mode == 2)
  { //read mode
    time->day = timeReadOriginal.ui8Date;
    time->month = timeReadOriginal.ui8Month;
    time->year = timeReadOriginal.ui8Year + START_YEAR;
    time->hour = timeReadOriginal.ui8Hour;
    time->minute = timeReadOriginal.ui8Minute;
    time->second = timeReadOriginal.ui8Second;
    time->century = timeReadOriginal.ui8Century;
  }

  else if (mode == 3)
  {
    syncSystemTime_withRTC();

    am1805_time_get(&timeRead);

    time->day = timeRead.ui8Date;
    time->month = timeRead.ui8Month;
    time->year = timeRead.ui8Year + START_YEAR;
    time->hour = timeRead.ui8Hour;
    time->minute = timeRead.ui8Minute;
    time->second = timeRead.ui8Second;
    time->century = timeRead.ui8Century;

  }

  else if (mode == 4)
  {
    syncRTC_withSysTime();

    am1805_time_get(&timeRead);

    time->day = timeRead.ui8Date;
    time->month = timeRead.ui8Month;
    time->year = timeRead.ui8Year + START_YEAR;
    time->hour = timeRead.ui8Hour;
    time->minute = timeRead.ui8Minute;
    time->second = timeRead.ui8Second;
    time->century = timeRead.ui8Century;
  }

}
