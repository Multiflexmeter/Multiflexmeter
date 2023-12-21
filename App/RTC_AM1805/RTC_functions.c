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

  timeWrite.ui8Mode = AM1805_24HR_MODE;
  timeWrite.ui8Century = ((stTime.tm_year / 100)) % 2; //0 = 2000, 1 = 2100
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
  timeDst->century = timeSrc->ui8Century;  //0 = 2000, 1 = 2100
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
#if VERBOSE_LEVEL == VLEVEL_H
  char timeStringNow[20] = {0};
  char timeStringWake[20] = {0};
#endif
  //check wait is between 0-7.
  if( waitTimeTicks > 7 )
  {
    waitTimeTicks = 7;
  }
  am1805_time_t time_RTC;

  //read current time
  am1805_time_get(&time_RTC);

  //convert to struct time
  struct tm struct_time = {0};
  struct_time.tm_year = 2000 + time_RTC.ui8Year + time_RTC.ui8Century * 100;
  struct_time.tm_mon = time_RTC.ui8Month;
  struct_time.tm_mday = time_RTC.ui8Date;
  struct_time.tm_hour = time_RTC.ui8Hour;
  struct_time.tm_min = time_RTC.ui8Minute;
  struct_time.tm_sec = time_RTC.ui8Second;

  //convert struct time to seconds timestamp
  time_t timestamp = mktime(&struct_time);

  //add seconds for sleep
  timestamp += sleepTime_sec;

  //convert new timestamp to struct time
  struct tm * sleepTime;
  sleepTime = gmtime(&timestamp);

  //set new alarm time
  am1805_time_t alarmTime;
  alarmTime = time_RTC; //copy orginal readed values
  alarmTime.ui8Century = ((sleepTime->tm_year / 100)) % 2; //overwrite century
  alarmTime.ui8Year = sleepTime->tm_year % 100;           //overwrite year
  alarmTime.ui8Month = sleepTime->tm_mon;                 //overwrite month
  alarmTime.ui8Date = sleepTime->tm_mday;                 //overwrite day
  alarmTime.ui8Hour = sleepTime->tm_hour;                 //overwrite year
  alarmTime.ui8Minute = sleepTime->tm_min;                //overwrite minutes
  alarmTime.ui8Second = sleepTime->tm_sec;                //overwrite seconds

  //setup the alarm
  am1805_alarm_set(alarmTime, ALARM_INTERVAL_MONTH, ALARM_IRQ_PULSE_1_64S, ALARM_PIN_PSW);

  APP_LOG(TS_OFF, VLEVEL_H, "Output CTRL: %x\r\n", am1805_get_output_control());

  am1805_ex2p_rising_edge_interrupt();

  am1805_enable_wdi_ex1_interrupt(); //low active, Batlow + Sensor module
  am1805_enable_wdi_ex2_interrupt(); //high active, USB + box sensor

  am1805_enable_pwgt(); //make sure I/O interface is disabled in sleep mode

#if VERBOSE_LEVEL == VLEVEL_H
  strftime(timeStringNow, sizeof(timeStringNow), "%H:%M:%S", &struct_time);
  strftime(timeStringWake, sizeof(timeStringWake), "%H:%M:%S", sleepTime);
  APP_LOG(TS_OFF, VLEVEL_H, "Sleep time; %u, NOW: %s, WAKE: %s\r\n", sleepTime_sec, timeStringNow, timeStringWake );
#endif

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
  am1805_countdown_set(CNTDWN_RANGE_SEC, sleepTime_sec, CNTDWN_IRQ_SINGLE_PULSED_1_64S, CNTDOWN_PIN_PSW_AND_nTIRQ_LOW);

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
  am1805_alarm_set(alarmTime, ALARM_INTERVAL_DISABLE, ALARM_IRQ_LEVEL, ALARM_PIN_PSW);
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

  uint16_t currentYear = START_YEAR + currentTime.ui8Year;

  am1805_time_t currentAlarm = am1805_read_current_alarm();

  if( currentTime.ui8Month == 12 && currentAlarm.ui8Month == 1 ) //detect next alarm is next year, increment
  {
    currentYear++;
  }

  struct tm stAlarm;

  stAlarm.tm_mday = currentAlarm.ui8Date;
  stAlarm.tm_hour = currentAlarm.ui8Hour;
  stAlarm.tm_min = currentAlarm.ui8Minute;
  stAlarm.tm_mon = currentAlarm.ui8Month;
  stAlarm.tm_sec = currentAlarm.ui8Second;
  stAlarm.tm_year = currentYear;


  timestamp = SysTimeMkTime(&stAlarm); //convert to unixtimestamp

  return timestamp;
}
