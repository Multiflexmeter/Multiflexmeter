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

static uint8_t statusRegisterRtc;
static bool forceSleep;

/**
 * @fn bool tm_year2Centerybit(int)
 * @brief function to calculate centerybit for RTC from year starting from 1900.
 *
 * @param year : years from 1900
 * @return 0 : 1900, 2100. 1 : 2000
 */
bool tm_year2Centerybit(int yearsFrom1900)
{
  return ((yearsFrom1900 / 100)) % 2; //time from 1900, 0-99 =>0: 1900, 100-199 => 1 = 2000, 200-299 => 0 = 2100
}


/**
 * @fn bool year2Centerybit(int)
 * @brief function to calculate centerybit for RTC from year starting from 0.
 *
 * @param year : years from 0
 * @return 0 : 1900, 2100. 1 : 2000
 */
bool year2Centerybit(uint16_t year)
{
  if( year >= 1900 )
    year -= 1900;

  return tm_year2Centerybit(year - 1900); //time from 1900-1999 ->0: 1900, 2000-2099 => 1 = 2000, 2100-2199 => 0 = 2100
}

/**
 * @fn const void syncRTC_withSysTime(void)
 * @brief function to sync external RTC with internal systime
 *
 */
const void syncRTC_withSysTime(void)
{
#if VERBOSE_LEVEL == VLEVEL_H
  char timeStringNow[20] = {0};
#endif
  SysTime_t sysTime = {0};
  struct tm stTime = {0};
  am1805_time_t timeWrite = { 0 };
  am1805_time_t timeRead = { 0 };

  sysTime = SysTimeGet(); //get internal RTC value

  SysTimeLocalTime(sysTime.Seconds, &stTime); //convert to date time

  APP_LOG(TS_OFF, VLEVEL_H, "LocalTime: %02d-%02d-%02d %02d:%02d:%02d %d\r\n", stTime.tm_mday, stTime.tm_mon + 1, stTime.tm_year + 1900, stTime.tm_hour, stTime.tm_min, stTime.tm_sec );

  timeWrite.ui8Mode = AM1805_24HR_MODE;
  timeWrite.ui8Century = tm_year2Centerybit(stTime.tm_year);
  timeWrite.ui8Date = stTime.tm_mday;
  timeWrite.ui8Month = stTime.tm_mon + 1;
  timeWrite.ui8Year = stTime.tm_year % 100;
  timeWrite.ui8Hour = stTime.tm_hour;
  timeWrite.ui8Minute = stTime.tm_min;
  timeWrite.ui8Second = stTime.tm_sec;
  timeWrite.ui8Weekday = 0;
  timeWrite.ui8Hundredth = sysTime.SubSeconds/10;

  APP_LOG(TS_OFF, VLEVEL_H, "Sync RTC: %02d-%02d-%02d %02d:%02d:%02d %d\r\n", timeWrite.ui8Date, timeWrite.ui8Month, timeWrite.ui8Year, timeWrite.ui8Hour, timeWrite.ui8Minute, timeWrite.ui8Second, timeWrite.ui8Century );

  am1805_time_set(timeWrite, 1); //save date/time to external RTC

  am1805_time_get(&timeRead); //read back to verify, for debug
  APP_LOG(TS_OFF, VLEVEL_H, "read RTC: %02d-%02d-%02d %02d:%02d:%02d %d\r\n", timeRead.ui8Date, timeRead.ui8Month, timeRead.ui8Year, timeRead.ui8Hour, timeRead.ui8Minute, timeRead.ui8Second, timeRead.ui8Century );

#if VERBOSE_LEVEL == VLEVEL_H
  strftime(timeStringNow, sizeof(timeStringNow), "%d-%m-%Y %H:%M:%S", &stTime);
  APP_LOG(TS_OFF, VLEVEL_H, "TIME NOW: %s\r\n", timeStringNow );
#endif

  APP_LOG(TS_OFF, VLEVEL_H, "TIME: RTC sync\r\n" );

}

/**
 * @fn const void syncSystemTime_withRTC(void)
 * @brief function to sync internal system with external RTC
 *
 */
const void syncSystemTime_withRTC(void)
{
#if VERBOSE_LEVEL == VLEVEL_H
  char timeStringNow[20] = {0};
#endif

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

#if VERBOSE_LEVEL == VLEVEL_H
  strftime(timeStringNow, sizeof(timeStringNow), "%d-%m-%Y %H:%M:%S", &stTime);
  APP_LOG(TS_OFF, VLEVEL_H, "TIME NOW: %s\r\n", timeStringNow );
#endif

  APP_LOG(TS_OFF, VLEVEL_H, "TIME: RTC -> internal\r\n" );
}

/**
 * @fn const void convert_am1805time_to_dateTime(am1805_time_t*, struct_dateTime*)
 * @brief function to convert structure "am1805_time_t" to "strucct_dateTime".
 *
 * @param timeSrc
 * @param timeDst
 */
const void convert_am1805time_to_dateTime(am1805_time_t * timeSrc, struct_dateTime * timeDst )
{
  timeDst->day = timeSrc->ui8Date;
  timeDst->month = timeSrc->ui8Month;
  timeDst->year = timeSrc->ui8Year + START_YEAR;
  timeDst->hour = timeSrc->ui8Hour;
  timeDst->minute = timeSrc->ui8Minute;
  timeDst->second = timeSrc->ui8Second;
  timeDst->century = timeSrc->ui8Century;  //0 = 1900/2100, 1 = 2000
}

/**
 * @fn bool getForceSleepStatus(void)
 * @brief function to get status of foceSleep
 *
 * @return boolean forcesleep
 */
const bool getForceSleepStatus(void)
{
  return forceSleep;
}

/**
 * @fn const bool setForceSleepStatus(void)
 * @brief function to set forceSleep status
 *
 * @return
 */
static const void setForceSleepStatus(bool status)
{
  forceSleep = status;
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
    timeWrite.ui8Century = year2Centerybit(time->year); //0 = 1900/2100, 1 = 2000
    timeWrite.ui8Date = time->day;
    timeWrite.ui8Month = time->month;
    timeWrite.ui8Year = time->year % 100;
    timeWrite.ui8Hour = time->hour;
    timeWrite.ui8Minute = time->minute;
    timeWrite.ui8Second = time->second;
    timeWrite.ui8Weekday = 0;
    timeWrite.ui8Hundredth = 0;

    am1805_time_set(timeWrite, 1); //write RTC

    am1805_time_get(&timeRead); //read back

    convert_am1805time_to_dateTime(&timeRead, time); //convert for result
  }

  else if (mode == 2) //read time from RTC
  { //read mode
    convert_am1805time_to_dateTime(&timeReadOriginal, time); //convert for result
  }

  else if (mode == 3) //sync RTC to system time
  {
    syncSystemTime_withRTC(); //sync

    am1805_time_get(&timeRead); //read back

    convert_am1805time_to_dateTime(&timeRead, time); //convert for result

  }

  else if (mode == 4) //sync RTC with system time
  {
    syncRTC_withSysTime(); //sync

    am1805_time_get(&timeRead); //read back

    convert_am1805time_to_dateTime(&timeRead, time); //convert for result

  }

  else if (mode == 5)
  {
    am1805_enable_wdi_ex2_interrupt(); //make sure ex2 interrupt is enabled
    setWakeupWdtAlarm(5); //set alarm on 5 seconds.
  }

  else if (mode == 6)
  {
    readStatusRegisterRtc();
    time->century =  getWakeupWdtStatus(true); //miss use century byte for status feedback, automatically clear WDT status (0x20, bit WDT).
  }

  else if (mode == 7)
  {
    setForceSleepStatus(true);
  }

}

/**
 * @fn const void setWakeupWdtAlarm(uint32_t)
 * @brief function to configure the wakeup alarm
 *
 * @param seconds
 */
const void setWakeupWdtAlarm( uint32_t seconds )
{
  am1805_watchdog_set( seconds * 1000, 1 ); //set watchdog timer, convert seconds to ms, use mode "1" => generate an interrupt on FOUT/nIRQ
}



/**
 * @fn uint8_t getStatusRegisterRtc(void)
 * @brief function to return last read status register
 *
 * @return
 */
uint8_t getStatusRegisterRtc(void)
{
  return statusRegisterRtc;
}

/**
 * @fn uint8_t readStatusRegisterRtc(void)
 * @brief function to read the RTC status register, force reset all except CB
 *
 * @return
 */
uint8_t readStatusRegisterRtc(void)
{
  statusRegisterRtc = am1805_get_status(0x7f);

  return getStatusRegisterRtc();
}

/**
 * @fn const bool getBitStatusRegisterRtc(uint8_t, bool)
 * @brief function to get bit of status register RTC
 *
 * @param mask bitmask
 * @param clear : true = clear, false = do not clear
 * @return
 */
const bool getBitStatusRegisterRtc(uint8_t mask, bool clear)
{
  uint8_t result = statusRegisterRtc & mask;

  if(clear )
  {
    statusRegisterRtc &= ~mask; //clear
  }

  return (result ? true : false);
}


/**
 * @fn const bool getWakeupWdtStatus(bool)
 * @brief get wakeup status
 *
 * @param clear
 * @return
 */
const bool getWakeupWdtStatus(bool clear)
{
  return getBitStatusRegisterRtc(AM1805_mask_WDT, clear);
}

/**
 * @fn const bool getWakeupAlarmStatus(bool)
 * @brief get wakeup alarm status
 * Set when the Alarm function is enabled and all selected Alarm registers match their respective counters.
 *
 * @param clear
 * @return
 */
const bool getWakeupAlarmStatus(bool clear)
{
  return getBitStatusRegisterRtc(AM1805_mask_ALM, clear);
}

/**
 * @fn const bool getWakeupBatStatus(bool)
 * @brief get wakeup battery status
 * Set when the system switches to the VBAT Power state
 *
 * @param clear locally
 * @return
 */
const bool getWakeupBatStatus(bool clear)
{
  return getBitStatusRegisterRtc(AM1805_mask_BAT, clear);
}

/**
 * @fn const bool getWakeupEx1Status(bool)
 * @brief get wakeup EX1 status
 * Set when the EX1 interrupt is triggered by the EXTI input.
 *
 * @param clear
 * @return
 */
const bool getWakeupEx1Status(bool clear)
{
  return getBitStatusRegisterRtc(AM1805_mask_EX1, clear);
}

/**
 * @fn const bool getWakeupEx2Status(bool)
 * @brief get wakeup EX2 status
 * Set when the EX2 interrupt is triggered by the WDI input.
 *
 * @param clear
 * @return
 */
const bool getWakeupEx2Status(bool clear)
{
  return getBitStatusRegisterRtc(AM1805_mask_EX2, clear);
}

/**
 * @fn const am1805_time_t calcAlarmTime(uint32_t)
 * @brief function to calculate new alarm time based the current RTC time and on number of seconds in sleep.
 *
 * @param sleepTime_sec : time to be in sleep
 * @return : new alarm time
 */
const am1805_time_t calcAlarmTime(uint32_t sleepTime_sec)
{
#if VERBOSE_LEVEL == VLEVEL_H
  char timeStringNow[20] = {0};
  char timeStringWake[20] = {0};
#endif
  am1805_time_t time_RTC;

  //read current time
  am1805_time_get(&time_RTC);

  APP_LOG(TS_OFF, VLEVEL_H, "Time RTC: NOW: %02d-%02d-%02d %02d:%02d:%02d %d\r\n", time_RTC.ui8Date, time_RTC.ui8Month, time_RTC.ui8Year, time_RTC.ui8Hour, time_RTC.ui8Minute, time_RTC.ui8Second, time_RTC.ui8Century );

  //convert to struct time
  //RTC Year from 0 to 99, mktime expect number since 1900
  //RTC Month from 1 to 12, mktime expect 0 to 11
  //RTC Date from 1 to 31, mktime expect 1 to 31
  //RTC Hours from 0 to 23, mktime expect 0 to 23
  //RTC Minutes from 0 to 59, mktime expect 0 to 59
  //RTC Seconds from 0 to 59, mktime expect 0 to 59
  struct tm struct_time = {0};
  if( time_RTC.ui8Century )
  {
    struct_time.tm_year = time_RTC.ui8Year + 100;
  }
  else
  {
    struct_time.tm_year = time_RTC.ui8Year + 200;
  }

  struct_time.tm_mon = time_RTC.ui8Month > 0 ? time_RTC.ui8Month - 1 :time_RTC.ui8Month;
  struct_time.tm_mday = time_RTC.ui8Date;
  struct_time.tm_hour = time_RTC.ui8Hour;
  struct_time.tm_min = time_RTC.ui8Minute;
  struct_time.tm_sec = time_RTC.ui8Second;


  APP_LOG(TS_OFF, VLEVEL_H, "Time; NOW: %02d-%02d-%04d %02d:%02d:%02d\r\n", struct_time.tm_mday, struct_time.tm_mon + 1, struct_time.tm_year + 1900, struct_time.tm_hour, struct_time.tm_min, struct_time.tm_sec );

  //convert struct time to seconds timestamp
  time_t timestamp = mktime(&struct_time);

  APP_LOG(TS_OFF, VLEVEL_H, "Sleep time; NOW: %u, WAKE: %u\r\n", timestamp, sleepTime_sec );

  //add seconds for sleep
  timestamp += sleepTime_sec;

  //convert new timestamp to struct time
  struct tm * sleepTime;
  sleepTime = gmtime(&timestamp);

  APP_LOG(TS_OFF, VLEVEL_H, "New Alarm gmtime; %02d-%02d-%04d %02d:%02d:%02d\r\n", sleepTime->tm_mday, sleepTime->tm_mon + 1, sleepTime->tm_year+ 1900, sleepTime->tm_hour, sleepTime->tm_min, sleepTime->tm_sec );

  //set new alarm time
  //gmtime Year number since 1900, RTC expect 0-99
  //gmtime Month from 0 to 11, RTC expect 1 to 12
  //gmtime Date from 1 to 31, RTC expect 1 to 31
  //gmtime Hours from 0 to 23, RTC expect 0 to 23
  //gmtime Minutes from 0 to 59, RTC expect 0 to 59
  //gmtime Seconds from 0 to 59, RTC expect 0 to 59
  am1805_time_t alarmTime;
  alarmTime = time_RTC; //copy original readed values
  alarmTime.ui8Century = tm_year2Centerybit(sleepTime->tm_year); //0 = 1900/2100, 1 = 2000 //overwrite century, not used in am1804_alarm_set() function
  alarmTime.ui8Year = sleepTime->tm_year % 100;           //overwrite year, not used in am1804_alarm_set() with mode ALARM_INTERVAL_MONTH or higher
  alarmTime.ui8Month = sleepTime->tm_mon + 1;             //overwrite month + 1
  alarmTime.ui8Date = sleepTime->tm_mday;                 //overwrite day
  alarmTime.ui8Hour = sleepTime->tm_hour;                 //overwrite year
  alarmTime.ui8Minute = sleepTime->tm_min;                //overwrite minutes
  alarmTime.ui8Second = sleepTime->tm_sec;                //overwrite seconds

  APP_LOG(TS_OFF, VLEVEL_H, "New Alarm RTC; %02d-%02d-%04d %02d:%02d:%02d\r\n", alarmTime.ui8Date, alarmTime.ui8Month, alarmTime.ui8Year, alarmTime.ui8Hour, alarmTime.ui8Minute, alarmTime.ui8Second );

#if VERBOSE_LEVEL == VLEVEL_H
  strftime(timeStringNow, sizeof(timeStringNow), "%d-%m-%Y %H:%M:%S", &struct_time);
  strftime(timeStringWake, sizeof(timeStringWake), "%d-%m-%Y %H:%M:%S", sleepTime);
  APP_LOG(TS_OFF, VLEVEL_H, "Sleep time; %u, NOW: %s, WAKE: %s\r\n", sleepTime_sec, timeStringNow, timeStringWake );
#endif

  return alarmTime;
}

/**
 * @fn const void setAlarmTime(am1805_time_t)
 * @brief function to configure a new alarm time
 *
 * @param newAlarmTime
 */
const void setAlarmTime( am1805_time_t newAlarmTime )
{
  am1805_alarm_set(newAlarmTime, ALARM_INTERVAL_MONTH, ALARM_IRQ_PULSE_1_64S, ALARM_PIN_INTERNAL_FLAG);
}

/**
 * @fn const void goIntoSleep(uint32_t, uint8_t)
 * @brief function to enable sleep mode of Real Time Clock.
 * Also disables the supply of this controller.
 * @warning this functions shutdown the processor supply
 *
 * @param sleepTime_sec : time to be in sleep
 * @param waitTimeTicks : value between 0-7, 0 = noDelay, 1-7 periods of 7.8ms. The value is limited automatically to 7.
 */
const void goIntoSleep(uint32_t sleepTime_sec, uint8_t waitTimeTicks)
{
  APP_LOG(TS_OFF, VLEVEL_H, "Sleep: SEC: %u, WAIT: %u\r\n", sleepTime_sec, waitTimeTicks);

  //check wait is between 0-7.
  if( waitTimeTicks > 7 )
  {
    waitTimeTicks = 7;
  }

  //setup the alarm
  setAlarmTime(calcAlarmTime(sleepTime_sec));

  APP_LOG(TS_OFF, VLEVEL_H, "Output CTRL: %x\r\n", am1805_get_output_control());

  am1805_ex2p_rising_edge_interrupt();

  am1805_enable_wdi_ex1_interrupt(); //low active, Batlow + Sensor module
  am1805_enable_wdi_ex2_interrupt(); //high active, USB + box sensor

  am1805_enable_pwgt(); //make sure I/O interface is disabled in sleep mode

  APP_LOG(TS_OFF, VLEVEL_H, "WDIN: %d, EXIN: %d\r\n", am1805_get_wdin_status(), am1805_get_exin_status()); //print input status WDI + EXT1

  APP_LOG(TS_OFF, VLEVEL_H, "Status: %x\r\n", am1805_get_status(0x7f)); //read status register, reset all flags
  APP_LOG(TS_OFF, VLEVEL_H, "Status: %x\r\n", am1805_get_status(0x7f)); //read again status register, reset all flags

  APP_LOG(TS_OFF, VLEVEL_H, "IRQ mask: %x\r\n", am1805_get_interrupt_mask()); //read IRQ mask register

  APP_LOG(TS_OFF, VLEVEL_H, "Sleep: %x\r\n", am1805_get_sleep_control()); //read sleep control register

  APP_LOG(TS_OFF, VLEVEL_H, "OSC: %x\r\n", am1805_get_osc_control()); //read oscillator control register

  APP_LOG(TS_OFF, VLEVEL_H, "Output CTRL: %x\r\n", am1805_get_output_control());
  APP_LOG(TS_OFF, VLEVEL_H, "Control 1: %x\r\n", am1805_get_control1()); //read control1 register
  APP_LOG(TS_OFF, VLEVEL_H, "Control 2: %x\r\n", am1805_get_control2()); //read control2 register


  //enable the sleepmode
  uint32_t sleepStatus = am1805_sleep_set(waitTimeTicks, SLEEP_MODE_nRST_LOW_AND_PSW_HIGH);


  switch( sleepStatus )
  {
    case SLEEP_RETURN_ACCEPTED_AFTER_RETRY:
      APP_LOG(TS_OFF, VLEVEL_H, "SLEEP: Retry, Cleared IRQ first\r\n" );
      //no break;
    case SLEEP_RETURN_ACCEPTED:
      APP_LOG(TS_OFF, VLEVEL_H, "SLEEP: ACTIVE\r\n" );
      break;
    case SLEEP_RETURN_ILLEGAL_INPUT:
      APP_LOG(TS_OFF, VLEVEL_H, "SLEEP: ERROR, illegal input\r\n" );
      break;
    case SLEEP_RETURN_DECLINED_ACTIVE_IRQ:
      APP_LOG(TS_OFF, VLEVEL_H, "SLEEP: ERROR, IRQ already active\r\n" );
      break;
    case SLEEP_RETURN_DECLINED_NO_SLEEP_IRQ:
      APP_LOG(TS_OFF, VLEVEL_H, "SLEEP: ERROR, sleep IRQ not enabled\r\n" );
      break;
    default:
      APP_LOG(TS_OFF, VLEVEL_H, "SLEEP: ERROR, wrong return value\r\n" );
      break;
  }

  //todo if brownout enabled, turn it off.
  while( 1 ); //keep waiting to turn off.

}

/**
 * @fn const void goIntoSleep_with_countdown(uint32_t, uint8_t)
 * @brief function to enable sleep mode of Real Time Clock.
 * Also disables the supply of this controller.
 * @warning this functions shutdown the processor supply
 * @note this function has a low resolution (1 minute) for values above 256 seconds
 *
 * @param sleepTime_sec : time to be in sleep
 * @param waitTimeTicks : value between 0-7, 0 = noDelay, 1-7 periods of 7.8ms. The value is limited automatically to 7.
 */
const void goIntoSleep_with_countdown(uint32_t sleepTime_sec, uint8_t waitTimeTicks)
{
  //check wait is between 0-7.
  if( waitTimeTicks > 7 )
  {
    waitTimeTicks = 7;
  }

  //setup the sleeptime
  am1805_countdown_set(CNTDWN_RANGE_SEC, sleepTime_sec, CNTDWN_IRQ_SINGLE_PULSED_1_64S, CNTDOWN_PIN_nTIRQ_LOW);

  //enable the sleepmode
  uint32_t sleepStatus = am1805_sleep_set(waitTimeTicks, SLEEP_MODE_nRST_LOW_AND_PSW_HIGH);

  switch( sleepStatus )
  {
    case SLEEP_RETURN_ACCEPTED:
      APP_LOG(TS_OFF, VLEVEL_H, "SLEEP: ACTIVE %u seconds\r\n", sleepTime_sec );
      break;
    case SLEEP_RETURN_ILLEGAL_INPUT:
      APP_LOG(TS_OFF, VLEVEL_H, "SLEEP: ERROR, illegal input\r\n" );
      break;
    case SLEEP_RETURN_DECLINED_ACTIVE_IRQ:
      APP_LOG(TS_OFF, VLEVEL_H, "SLEEP: ERROR, IRQ already active\r\n" );
      break;
    case SLEEP_RETURN_DECLINED_NO_SLEEP_IRQ:
      APP_LOG(TS_OFF, VLEVEL_H, "SLEEP: ERROR, sleep IRQ not enabled\r\n" );
      break;
    default:
      APP_LOG(TS_OFF, VLEVEL_H, "SLEEP: ERROR, wrong return value\r\n" );
      break;
  }

  //todo if brownout enabled, turn it off.
  while( 1 ); //keep waiting to turn off.

}

/**
 * @fn const void disableSleep(void)
 * @brief function to disable the sleepmode.
 *
 */
const void disableSleep(void)
{
  am1805_time_t alarmTime = {0};
  am1805_countdown_set(CNTDWN_RANGE_SEC, 0, CNTDWN_IRQ_SINGLE_PULSED_1_64S, CNTDOWN_DISABLE_CNT_DOWN_TMR);
  am1805_alarm_set(alarmTime, ALARM_INTERVAL_DISABLE, ALARM_IRQ_LEVEL, ALARM_PIN_INTERNAL_FLAG);
}

/**
 * @fn const uint32_t get_current_alarm(uint16_t, uint8_t)
 * @brief function to get the current alarm time
 *
 * @param currentYear
 * @param currentMonth
 * @return
 */
const uint32_t get_current_alarm(void)
{
  uint32_t timestamp;

  am1805_time_t currentTime;
  am1805_time_get(&currentTime);
  APP_LOG(TS_OFF, VLEVEL_H, "Time reg: %02d-%02d-%02d %02d:%02d;%02d\r\n", currentTime.ui8Date, currentTime.ui8Month, currentTime.ui8Year, currentTime.ui8Hour, currentTime.ui8Minute, currentTime.ui8Second ); //print info

  uint16_t currentYear = START_YEAR + currentTime.ui8Year;

  am1805_time_t currentAlarm = am1805_read_current_alarm();
  APP_LOG(TS_OFF, VLEVEL_H, "Alarm reg: %02d-%02d-%02d %02d:%02d;%02d\r\n", currentAlarm.ui8Date, currentAlarm.ui8Month, currentAlarm.ui8Year, currentAlarm.ui8Hour, currentAlarm.ui8Minute, currentAlarm.ui8Second ); //print info

  if( currentTime.ui8Month == 12 && currentTime.ui8Date == 31 && currentAlarm.ui8Date == 1 ) //detect next alarm is next year, increment
  {
    currentYear++;
  }

  APP_LOG(TS_OFF, VLEVEL_H, "Year: %04d\r\n", currentYear ); //print info

  //convert to struct time
  //SysTimeMkTime() Year number since 1900, RTC expect 0-99
  //SysTimeMkTime() Month from 0 to 11, RTC expect 1 to 12
  //SysTimeMkTime() Date from 1 to 31, RTC expect 1 to 31
  //SysTimeMkTime() Hours from 0 to 23, RTC expect 0 to 23
  //SysTimeMkTime() Minutes from 0 to 59, RTC expect 0 to 59
  //SysTimeMkTime() Seconds from 0 to 59, RTC expect 0 to 59
  struct tm stAlarm = {0};
  stAlarm.tm_year = currentYear > 1900 ? currentYear - 1900 : currentYear; //alarm does not contain a year
  stAlarm.tm_mon = currentTime.ui8Month -1; //alarm does not contain a month, use month of calender. Convert from calender 1-12, to struct tm 0-11.
  stAlarm.tm_mday = currentAlarm.ui8Date;
  stAlarm.tm_hour = currentAlarm.ui8Hour;
  stAlarm.tm_min = currentAlarm.ui8Minute;
  stAlarm.tm_sec = currentAlarm.ui8Second;


  APP_LOG(TS_OFF, VLEVEL_H, "Alarm: %02d-%02d-%04d %02d:%02d;%02d\r\n", stAlarm.tm_mday, stAlarm.tm_mon + 1, stAlarm.tm_year + 1900, stAlarm.tm_hour, stAlarm.tm_min, stAlarm.tm_sec ); //print info

  timestamp = SysTimeMkTime(&stAlarm); //convert to unixtimestamp


  APP_LOG(TS_OFF, VLEVEL_H, "Alarm timestamp: %u\r\n", timestamp ); //print info


  struct tm localtime = {0};
  SysTimeLocalTime(timestamp, &localtime);

  APP_LOG(TS_OFF, VLEVEL_H, "SysTimeLocalTime: %02d-%02d-%02d %02d:%02d;%02d\r\n", localtime.tm_mday, localtime.tm_mon + 1, localtime.tm_year + 1900, localtime.tm_hour, localtime.tm_min, localtime.tm_sec ); //print info

  return timestamp;
}
