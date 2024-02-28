/**
  ******************************************************************************
  * @addtogroup     : App
  * @{
  * @file           : mainTask.c
  * @brief          : main task functions
  * @author         : P.Kwekkeboom
  * @date           : Sep 28, 2023
  * @}
  ******************************************************************************
  */

#include <ctype.h>
#include "main.h"
#include "sys_app.h"
#include "utilities.h"
#include "common/common.h"
#include "common/app_types.h"
#include "utilities_def.h"
#include "stm32_seq.h"
#include "stm32_timer.h"
#include "stm32_systime.h"
#include "timer_if.h"

#include "lora_app.h"
#include "LoRaMac.h"

#include "common/softwareVersion.h"
#include "IO/board_io.h"
#include "IO/board_io_functions.h"
#include "IO/led.h"
#include "FRAM/FRAM_functions.h"
#include "I2CMaster/SensorFunctions.h"
#include "measurement.h"
#include "BatMon_BQ35100/BatMon_functions.h"
#include "RTC_AM1805/RTC_functions.h"
#include "CommConfig.h"
#include "CommConfig_usr.h"
#include "MFMconfiguration.h"
#include "mainTask.h"

const char NO_VERSION[]="";

#define LORA_PERIODICALLY_CONFIRMED_MSG //comment if feature must be disabled.
#define LORA_PERIODICALLY_REQUEST_TIME //comment if feature must be disabled.
#define RTC_USED_FOR_SHUTDOWN_PROCESSOR //comment if feature must be disabled. //if enabled jumper on J11 1-2 must be placed.

/**
 * @def SEND_MEASUREMENT_IN_EQUAL_INTERVAL
 * @brief Feature used in test fase to have an equal interval between each sensor module slot.
 * Normally a burst of one minute interval between sensors in one round is used.
 * @note comment if feature must be disabled
 */
#define SEND_MEASUREMENT_IN_EQUAL_INTERVAL

#define LORA_REJOIN_NUMBER_OF_RETRIES   5
#define INTERVAL_NEXT_SENSOR_IN_ONE_ROUND  (60000) //1 minute

static volatile bool mainTaskActive;
static uint32_t mainTask_tmr;
static int mainTask_state;
static int loraJoinRetryCounter = 0;

static UTIL_TIMER_Object_t MainTimer;
static UTIL_TIMER_Time_t MainPeriodSleep = 60000;
static UTIL_TIMER_Time_t MainPeriodNormal = 10;
static bool enableListenUart;
static bool measureEOS_enabled;

static UTIL_TIMER_Object_t wait_Timer;
static volatile bool waiting = false;

static UTIL_TIMER_Object_t measurement_Timer;
static volatile bool startMeasure = true;

static UTIL_TIMER_Object_t rejoin_Timer;

static UTIL_TIMER_Object_t systemActive_Timer;
static UTIL_TIMER_Time_t period_1sec = 1000;
static volatile uint32_t systemActiveTime_sec;

static UTIL_TIMER_Object_t timeout_Timer;
static volatile bool timeout = false;

static uint8_t dataBuffer[50];
static struct_MFM_sensorModuleData stMFM_sensorModuleData;
static struct_MFM_baseData stMFM_baseData;
static uint8_t currentSensorModuleIndex = 0;
static uint8_t currentNumberOfSensorModule = 0;
static uint8_t numberOfActivesensorModules = 0;
static bool nextSensorInSameMeasureRound = 0;
static volatile bool loraTransmitReady = false;
static volatile bool loraReceiveReady = false;
static LmHandlerErrorStatus_t loraTransmitStatus;

static uint16_t sensorType;
static uint8_t sensorProtocol;

static uint8_t waitForBatteryMonitorDataCounter = 0;
static struct_FRAM_settings FRAM_Settings;
static struct_wakeupSource stWakeupSource;

const void setDevNonce(uint16_t devNonce)__attribute__((unused));
const void setJoinNonce(uint16_t JoinNonce)__attribute__((unused));
const void setDownFCounter(uint16_t counter)__attribute__((unused));
const void setUpFCounter(uint16_t counter)__attribute__((unused));

/**
 * @fn const void setNextPeriod(UTIL_TIMER_Time_t)
 * @brief function to set next trigger period of mainTask
 *
 * @param next
 */
const void setNextPeriod( UTIL_TIMER_Time_t next )
{
  UTIL_TIMER_Stop(&MainTimer);
  UTIL_TIMER_SetPeriod(&MainTimer, next);
  UTIL_TIMER_Start(&MainTimer);
}

/**
 * @fn const void executeAlwaysOn(void)
 * @brief helper function which read MFM configuarion for vAlwaysOn and execute the alwaysOn driving enable / disable.
 *
 */
const void executeAlwaysOn(void)
{
  //get configuration always on
  if( getAlwaysOn() )
  {
    enable_vAlwaysOn(); //enable vAlwaysOn supply
  }
  else
  {
    disable_vAlwaysOn(); //disable vAlwaysOn supply
  }
}

/**
 * @fn const void setWait(int)
 * @brief helper function to set a wait time
 *
 * @param periodMs
 */
const void setWait(int periodMs)
{
  waiting = true; //enable wait
  UTIL_TIMER_StartWithPeriod(&wait_Timer, periodMs); //set timer
}

/**
 * @fn const void setWait(int)
 * @brief helper function to set a wait time
 *
 * @param periodMs
 */
const void setNewMeasureTime(int periodMs)
{
  startMeasure = false; //reset
  UTIL_TIMER_StartWithPeriod(&measurement_Timer, periodMs); //set timer
}

/**
 * @fn const void setDelayReJoin(int)
 * @brief helper function to set set a delay rejoin
 *
 * @param periodMs
 */
const void setDelayReJoin(int periodMs)
{
  UTIL_TIMER_StartWithPeriod(&rejoin_Timer, periodMs); //set timer
}

/**
 * @fn void setTimeout(int)
 * @brief function to set a new timeout
 *
 * @param periodMs
 */
static void setTimeout(int periodMs)
{
  timeout = false; //reset
  UTIL_TIMER_StartWithPeriod(&timeout_Timer, periodMs); //set timer
}

/**
 * @fn const uint16_t getDevNonce(void)
 * @brief function to get the DevNonce counter
 *
 * @return devNonce counter
 */
const uint16_t getDevNonce(void)
{
  /* get DevNonce */
    LoRaMacNvmData_t *nvm;
    MibRequestConfirm_t mibReq;
    mibReq.Type = MIB_NVM_CTXS;
    LoRaMacMibGetRequestConfirm( &mibReq );
    nvm = ( LoRaMacNvmData_t * )mibReq.Param.Contexts;

    return nvm->Crypto.DevNonce;
}

/**
 * @fn const uint16_t getJoinNonce(void)
 * @brief function to get the JoinNonce Counter
 *
 * @return JoinNonce counter
 */
const uint16_t getJoinNonce(void)
{
  /* get DevNonce */
    LoRaMacNvmData_t *nvm;
    MibRequestConfirm_t mibReq;
    mibReq.Type = MIB_NVM_CTXS;
    LoRaMacMibGetRequestConfirm( &mibReq );
    nvm = ( LoRaMacNvmData_t * )mibReq.Param.Contexts;

    return nvm->Crypto.JoinNonce;
}

/**
 * @fn const uint16_t getDownFCounter(void)
 * @brief function to get the down frame counter
 *
 * @return
 */
const uint16_t getDownFCounter(void)
{
  /* get UplinkCounter */
    LoRaMacNvmData_t *nvm;
    MibRequestConfirm_t mibReq;
    mibReq.Type = MIB_NVM_CTXS;
    LoRaMacMibGetRequestConfirm( &mibReq );
    nvm = ( LoRaMacNvmData_t * )mibReq.Param.Contexts;

    return nvm->Crypto.LastDownFCnt;
}

/**
 * @fn const uint16_t getUpFCounter(void)
 * @brief function to get the up frame counter
 *
 * @return
 */
const uint16_t getUpFCounter(void)
{
  /* get UplinkCounter */
    LoRaMacNvmData_t *nvm;
    MibRequestConfirm_t mibReq;
    mibReq.Type = MIB_NVM_CTXS;
    LoRaMacMibGetRequestConfirm( &mibReq );
    nvm = ( LoRaMacNvmData_t * )mibReq.Param.Contexts;

    return nvm->Crypto.FCntList.FCntUp;
}

/**
 * @fn const void setDevNonce(uint16_t)
 * @brief function to set the DevNonce counter
 *
 * @param devNonce
 */
const void setDevNonce(uint16_t devNonce)
{
  /* get DevNonce */
    LoRaMacNvmData_t *nvm;
    MibRequestConfirm_t mibReq;
    mibReq.Type = MIB_NVM_CTXS;
    LoRaMacMibGetRequestConfirm( &mibReq );
    nvm = ( LoRaMacNvmData_t * )mibReq.Param.Contexts;

    nvm->Crypto.DevNonce = devNonce;
}

/**
 * @fn const void setJoinNonce(uint16_t)
 * @brief funcction to set the Join Nonce counter
 *
 * @param JoinNonce
 */
const void setJoinNonce(uint16_t JoinNonce)
{
  /* get DevNonce */
    LoRaMacNvmData_t *nvm;
    MibRequestConfirm_t mibReq;
    mibReq.Type = MIB_NVM_CTXS;
    LoRaMacMibGetRequestConfirm( &mibReq );
    nvm = ( LoRaMacNvmData_t * )mibReq.Param.Contexts;

    nvm->Crypto.JoinNonce = JoinNonce;
}

/**
 * @fn const void setDownFCounter(uint16_t)
 * @brief function to set the DownFCounter
 *
 * @param counter
 */
const void setDownFCounter(uint16_t counter)
{
  /* get UplinkCounter */
    LoRaMacNvmData_t *nvm;
    MibRequestConfirm_t mibReq;
    mibReq.Type = MIB_NVM_CTXS;
    LoRaMacMibGetRequestConfirm( &mibReq );
    nvm = ( LoRaMacNvmData_t * )mibReq.Param.Contexts;

    nvm->Crypto.LastDownFCnt = counter;
}

/**
 * @fn const void setUpFCounter(uint16_t)
 * @brief function to set the upFCounter
 *
 * @param counter
 */
const void setUpFCounter(uint16_t counter)
{
  /* get UplinkCounter */
    LoRaMacNvmData_t *nvm;
    MibRequestConfirm_t mibReq;
    mibReq.Type = MIB_NVM_CTXS;
    LoRaMacMibGetRequestConfirm( &mibReq );
    nvm = ( LoRaMacNvmData_t * )mibReq.Param.Contexts;

    nvm->Crypto.FCntList.FCntUp = counter;
}

/**
 * @fn const void executeBatteryMeasure(void)
 * @brief helper function to read the battery monitor data and print a debug line
 *
 */
static const void executeBatteryMeasure(void)
{

  batmon_measure(); //save battery monitor data

  APP_LOG(TS_OFF, VLEVEL_H, "Battery monitor data: %dmV, %dmA, %d%%, %u.%u%cC, Z:%umOhm, R:%umOhm\r\n",
      batmon_getMeasure().voltage,
      batmon_getMeasure().current,
      batmon_getMeasure().stateOfHealth,
      batmon_getMeasure().temperature/10, batmon_getMeasure().temperature%10, 176,
      batmon_getMeasure().MeasuredZ,
      batmon_getMeasure().ScaledR
      );
}

/**
 * @fn uint32_t getNextWake(UTIL_TIMER_Time_t period, uint32_t activeTime )
 * @brief helper function to calculate next wakeUp
 *
 * @param period : measurement interval
 * @param startTime : active time from last wake-up
 * @return
 */
static uint32_t getNextWake(UTIL_TIMER_Time_t period, uint32_t activeTime )
{
  uint32_t nextWake = period/1000; //LoraInterval from ms to sec

  if( nextWake >  activeTime )
  {
    nextWake -= activeTime;
  }

  //guard minimum sleep time
  if( nextWake < 30 )
  {
    nextWake = 30;
  }


  return nextWake;
}

/**
 * @fn UTIL_TIMER_Time_t getNextMeasureInterval(bool, UTIL_TIMER_Time_t, uint8_t)
 * @brief function to calculate the next measure interval based on measurement in sameRound, time and number of sensors
 *
 * @param sameRound : true if next measurement is in same round
 * @param time : interval time
 * @param numberOfSensors : number of active sensors
 * @return
 */
static UTIL_TIMER_Time_t getNextMeasureInterval(bool sameRound, UTIL_TIMER_Time_t time, uint8_t numberOfSensors)
{
  UTIL_TIMER_Time_t newInterval = time;
#ifndef SEND_MEASUREMENT_IN_EQUAL_INTERVAL

  if (sameRound ) //check if next interval is from an other sensor in the same round, then use short time of one minute.
  {
    newInterval = INTERVAL_NEXT_SENSOR_IN_ONE_ROUND; //next round, every minute transmit each sensor
  }
  else
  { //the round is finished, then set the remaining interval time, first check if there is time
    if( time > numberOfSensors * INTERVAL_NEXT_SENSOR_IN_ONE_ROUND )
    {
      newInterval -= numberOfSensors * INTERVAL_NEXT_SENSOR_IN_ONE_ROUND;
    }
    else
    { //multiple sensor measure time is larger then the interval setting, use interval setting directly.
      newInterval = time;
    }
  }
#else
  static bool debugPrintOnce = false;
  if( debugPrintOnce == false)
  {
    debugPrintOnce = true;
    APP_LOG(TS_OFF, VLEVEL_H, "************ WARNING ************\r\n");
    APP_LOG(TS_OFF, VLEVEL_H, "test DEFINE active: SEND_MEASUREMENT_IN_EQUAL_INTERVAL\r\n");
    APP_LOG(TS_OFF, VLEVEL_H, "*********************************\r\n");
  }
#endif
  return newInterval;
}

/**
 * @fn uint32_t getActiveTime(void)
 * @brief function gives the system active time in seconds.
 *
 * @return
 */
uint32_t getActiveTime(void)
{
  return systemActiveTime_sec;
}

/**
 * @fn const uint32_t getNextBatteryEOStime(void)
 * @brief
 *
 * @return next timestamp for EOS battery meaasurement
 */
const uint32_t getNextBatteryEOStime(uint32_t timestampNow)
{
  uint32_t timestampNext = (timestampNow / TM_SECONDS_IN_1DAY) * TM_SECONDS_IN_1DAY; //get this day 00:00

  //check current time is before 12:00
  if( (timestampNow % TM_SECONDS_IN_1DAY) < (TM_SECONDS_IN_1DAY>>1) )
  { //current time of the day before 12:00
    timestampNext+= (60L*60*12); //set this day 12:00
  }
  else
  { //current time of the day after 12:00
    timestampNext+= (60L*60*36); //set next day day 12:00. 12 + 24 hours from 00:00
  }

#if VERBOSE_LEVEL == VLEVEL_H
  char timeString[30];
  struct tm structTime;
  SysTimeLocalTime(timestampNext, &structTime);
  strftime(timeString, sizeof(timeString), "%H:%M:%S %d-%m-%y", &structTime);
  APP_LOG(TS_OFF, VLEVEL_H, "Next: %s, %u, delta: %u\r\n", timeString, timestampNext, timestampNext - timestampNow);
#endif

  return timestampNext;
}

/**
 * @fn const bool alarmNotYetTriggered(void)
 * @brief function to detect an alarm is active but not yet triggered.
 *
 * @return true = active alarm found, false = no active alarm found.
 */
const bool alarmNotYetTriggered(void)
{
  //detect if awake is by alarm, or by other f.e. USB
   uint32_t currentAlarm = get_current_alarm(); //get alarm time from RTC
   uint32_t currentTime = SysTimeGet().Seconds; //get current time from controller

#if VERBOSE_LEVEL == VLEVEL_H
   struct tm stTime;
   SysTimeLocalTime(currentAlarm, &stTime); //get alarm time
   APP_LOG(TS_OFF, VLEVEL_H, "Wake, no measure. Next alarm: %02d-%02d-%02d %02d:%02d;%02d\r\n", stTime.tm_mday, stTime.tm_mon + 1, stTime.tm_year + 1900, stTime.tm_hour, stTime.tm_min, stTime.tm_sec ); //print info, mday: 1-31, mon 0-11,
#endif

   //check current time is before valid time
   if( currentTime < UNIX_TIME_START_APP )
   {
     return false;
   }

   //check current alarm is before valid time
   if( currentAlarm < UNIX_TIME_START_APP )
   {
     return false;
   }

   //check current alarm is in future
   if( currentAlarm > currentTime )
   {
     APP_LOG(TS_OFF, VLEVEL_H, "Wake, no measure. Time: %u, next: %u, delta: %u\r\n", currentTime, currentAlarm, currentAlarm - currentTime ); //print info

     //check remaining time is smaller then interval
     if( (getLoraInterval() * TM_SECONDS_IN_1MINUTE) > (currentAlarm - currentTime) )
     {
       systemActiveTime_sec = (getLoraInterval() * TM_SECONDS_IN_1MINUTE) - (currentAlarm - currentTime); //set active time
       setNewMeasureTime( (currentAlarm - currentTime) * 1000L); //set remaining time in measure time
       return true; //valid alarm time found
     }
     else
     { //some mistake, alarm is more in future compare to the current interval, force a direct measurement
       systemActiveTime_sec = 0;
       return false;
     }
   }
   else
   {
     return false;
   }
}

/**
 * @fn const UNION_diagnosticStatusBits detectDiagnostics(void)
 * @brief function to get diagnostics
 *
 * @return
 */
const UNION_diagnosticStatusBits getDiagnostics(void)
{
  UNION_diagnosticStatusBits diagnosticStatusBits = {0};

  diagnosticStatusBits.bit.batteryLow = readInput_board_io(EXT_IOBAT_ALERT);
  diagnosticStatusBits.bit.usbConnected = readInput_board_io(EXT_IOUSB_CONNECTED);
  diagnosticStatusBits.bit.lightSensorActive = readInput_board_io(INT_IO_BOX_OPEN);

  APP_LOG(TS_OFF, VLEVEL_H, "Diagnostic: BAT: %d, USB: %d, BOX: %d\r\n", diagnosticStatusBits.bit.batteryLow, diagnosticStatusBits.bit.usbConnected, diagnosticStatusBits.bit.lightSensorActive);

  return diagnosticStatusBits;
}

/**
 * @fn const bool checkForceRejoin(void)
 * @brief helper function to check if a wake is within 5 seconds, this could be a forced reset
 *
 * @return
 */
const bool checkForceRejoin( bool enable)
{
  bool rejoin = false;

  uint32_t currentWakeupTime = SysTimeGet().Seconds; //read current time
  uint32_t lastWakeupTime = getLastWakeupTime(); //read previous wakeupTime

  //only check if enable is true
  if( enable )
  {
    if( currentWakeupTime <= lastWakeupTime + 5 ) //check if time is within 5 seconds.
    {
      rejoin = true;
    }
    setLastWakeupTime(currentWakeupTime); //save for next time, only when reset is detect
  }

  APP_LOG(TS_OFF, VLEVEL_H, "Check Rejoin by Reset: RST: %d, REJOIN: %d, prev: %u, now: %u, delta: \r\n", enable, currentWakeupTime <= lastWakeupTime + 5, lastWakeupTime, currentWakeupTime, currentWakeupTime - lastWakeupTime );

  if( rejoin )
  {
    APP_LOG(TS_OFF, VLEVEL_L, "\r\n----- FORCED REJOIN BY RESET -----\r\n\r\n" );
  }

  return rejoin;
}

/**
 * @fn struct_wakeupSource getWakeupSource(void)
 * @brief functions detects the wakeupsource with the help of RTC status register, I/O pin status and backup domain reset status.
 *
 * @return wakeupSources \ref struct_wakeupSource
 */
struct_wakeupSource getWakeupSource(void)
{
  struct_wakeupSource wakeupSource = {0};
  readStatusRegisterRtc(); //read the RTC status registers

#if VERBOSE_LEVEL == VLEVEL_H
  APP_LOG(TS_OFF, VLEVEL_H, "**** RTC: status: ");
  APP_LOG(TS_OFF, VLEVEL_H, "0x%02x", getStatusRegisterRtc());
#endif

  //check alarm, indicates awake from alarm
  if (getWakeupAlarmStatus(0))
  {
    wakeupSource.byAlarm = true;
    APP_LOG(TS_OFF, VLEVEL_H, ", ALARM");
  }

  //check BAT flag is set, indicate battery disconnected.
  if (getWakeupBatStatus(0))
  {
    wakeupSource.byLowBattery = true;
    APP_LOG(TS_OFF, VLEVEL_H, ", BAT");
  }

  //check ex1 IRQ from RTC
  if( getWakeupEx1Status(0) )
  {
    //combination of several sources, USB, BAT_ALERT, SENSOR IRQ
    APP_LOG(TS_OFF, VLEVEL_H, ", WAKE EX1");

    //check USB
    if( readInput_board_io(EXT_IOUSB_CONNECTED) )
    {
      wakeupSource.byUSB = true;
      APP_LOG(TS_OFF, VLEVEL_H, ", USB");
    }

    //check SENSOR IRQ
    if( readInput_board_io(EXT_IOSENSOR_INTX))
    {
      wakeupSource.bySensorIrq = true;
      APP_LOG(TS_OFF, VLEVEL_H, ", SENSOR");
    }

    //check BAT_ALERT
    if( readInput_board_io(EXT_IOBAT_ALERT))
    {
      wakeupSource.bySensorIrq = true;
      APP_LOG(TS_OFF, VLEVEL_H, ", BAT_ALERT");
    }
  }

  //check ex2 IRQ from RTC
  if( getWakeupEx2Status(0) )
  {
    //must be Light sensor (box-open)
    APP_LOG(TS_OFF, VLEVEL_H, ", Ex2");

    if( readInput_board_io(INT_IO_BOX_OPEN) )
    {
      APP_LOG(TS_OFF, VLEVEL_H, ", BOX-OPEN");
    }
    wakeupSource.byLightSensor = true;
  }

  // method of maksing out all known sources, the remaining should be byReset button.
  // check if wakeupSource is known: byAlarm or byLightSensor or byLowBattery or bySensorIrq or byUSB or reset of backup domain.
  // the remaining wakeup should be byReset button.

  if( !wakeupSource.byAlarm &&        // no detect of wakeup byAlarm
      !wakeupSource.byLightSensor &&  // no detect of wakeup byLightSensor
      !wakeupSource.byLowBattery &&   // no detect of wakeup byLowBattery
      !wakeupSource.bySensorIrq &&    // no detect of wakeup bySensorIrq
      !wakeupSource.byUSB &&          // no detect of wakeup byUSB
      !getResetBackup()               // no detect of backup regsiter reset: if set indicates no power on backup domain: -> no battery power
      )
  { //now it should be by Reset
    wakeupSource.byReset = true; //s
    APP_LOG(TS_OFF, VLEVEL_H, ", ### RESET ###");
  }

  APP_LOG(TS_OFF, VLEVEL_H, " ****\r\n");

  return wakeupSource;
}

/**
 * @brief override function getSoftwareSensorboard(), needs to be override by real functions
 *
 * @return
 */
const char * getSoftwareSensorboard(int sensorModuleId)
{
  //check valid argument
  if( sensorModuleId < 0 ||  sensorModuleId >= sizeof(FRAM_Settings.modules) / sizeof(FRAM_Settings.modules[0]) )
  {
    return NO_VERSION;
  }

  //check on control character, not printable
  if( iscntrl( (int) FRAM_Settings.modules[sensorModuleId].version[0] ) )
  {
    return NO_VERSION;
  }

  //check on 0xFF, default value of not written flash
  if( FRAM_Settings.modules[sensorModuleId].version[0] == 0xFF )
  {
    return NO_VERSION;
  }

  return FRAM_Settings.modules[sensorModuleId].version;
}

/**
 * @brief override function getProtocolSensorboard(), needs to be override by real functions
 *
 * @return
 */
const uint8_t getProtocolSensorboard(int sensorModuleId)
{
  if( sensorModuleId < 0 ||  sensorModuleId >= sizeof(FRAM_Settings.modules) / sizeof(FRAM_Settings.modules[0]) )
  {
    return 0;
  }
  return FRAM_Settings.sensorModuleProtocol[sensorModuleId];
}

/**
 * @fn const printSensorModuleError(SensorError)
 * @brief helper function to print sensor module error status information to debug port.
 *
 * @param status
 */
static const void printSensorModuleError(SensorError status)
{
  APP_LOG(TS_OFF, VLEVEL_H, "Sensor module data: ERROR, " ); //print error

  switch (status)
  {

    case SENSOR_CRC_ERROR:
      APP_LOG(TS_OFF, VLEVEL_H, "CRC\r\n"); //print error
      break;

    case SENSOR_REGISTER_ERROR:
      APP_LOG(TS_OFF, VLEVEL_H, "register\r\n"); //print error
      break;

    case SENSOR_TIMEOUT:
      APP_LOG(TS_OFF, VLEVEL_H, "timeout\r\n"); //print error
      break;

    case SENSOR_BUFFER_ERROR:
      APP_LOG(TS_OFF, VLEVEL_H, "buffer\r\n"); //print error
      break;

    case SENSOR_ID_ERROR:
      APP_LOG(TS_OFF, VLEVEL_H, "ID\r\n"); //print error
      break;

    default:
      APP_LOG(TS_OFF, VLEVEL_H, "unknown\r\n"); //print error
      break;

    }
}

/**
 * @fn const void printEndLine(uint32_t, uint32_t)
 * @brief helper function to print EndLine to debug port
 *
 * @param TimeStampState : \ref VLEVEL_L, \ref VLEVEL_M, \ref VLEVEL_H
 * * @param VerboseLevel : \ref TS_OFF or \ref TS_ON
 */
static const void printEndLine(uint32_t TimeStampState, uint32_t VerboseLevel)
{
  APP_LOG(TimeStampState, VerboseLevel, "\r\n"); //print end of line
}

/**
 * @fn const void printSeparatorLine(uint32_t, uint32_t, const char, int, bool)
 * @brief helper function to print an separator line of character with length to debug port. Endline conditional.
 *
 * @param TimeStampState : \ref VLEVEL_L, \ref VLEVEL_M, \ref VLEVEL_H
 * @param VerboseLevel : \ref TS_OFF or \ref TS_ON
 * @param character : character to print as marking
 * @param length : number of characters
 * @param endLine : true = print EndLine, false = no EndLine is added.
 */
static const void printSeparatorLine(uint32_t TimeStampState, uint32_t VerboseLevel, const char character, int length, bool endLine)
{
  while(length-- > 0)
    APP_LOG(TimeStampState, VerboseLevel, "%c", character); //print character

  if( endLine )
    printEndLine(TimeStampState, VerboseLevel);
}

/**
 * @fn const void printHeader(uint32_t, uint32_t, const char, int, const char*)
 * @brief helper function to print an header line with pre and post filling of character to debug port.
 * Always with endLine
 *
 * @param TimeStampState : \ref VLEVEL_L, \ref VLEVEL_M, \ref VLEVEL_H
 * @param VerboseLevel : \ref TS_OFF or \ref TS_ON
 * @param character : character to print as marking
 * @param length : number of characters
 * @param header : the header to print
 */
static const void printHeader(uint32_t TimeStampState, uint32_t VerboseLevel, const char character, int length, const char * header)
{
  int lengthHeader = strlen(header);
  int halfLength = 0;

  if( length > (lengthHeader + 4)) //check length is at least 4 larger, for with space and devide by two
  {
    halfLength = length - (lengthHeader + 2);
    halfLength >>= 1; //devide by two
  }
  else
  {
    halfLength = 0;
  }

  printSeparatorLine( TimeStampState, VerboseLevel, character, length, true ); //print line with end
  printSeparatorLine( TimeStampState, VerboseLevel, character, halfLength, false ); //print first half, without end
  APP_LOG(TimeStampState, VerboseLevel, " %s ", header); //print header with surrounding withspace
  printSeparatorLine( TimeStampState, VerboseLevel, character, halfLength, true ); //print first half, with end
}

/**
 * @fn const printSensorModuleRoughData(uint8_t, uint8_t, uint8_t*)
 * @brief helper function to print sensor module rough data to debug port
 *
 * @param sensorModuleId
 * @param sensorModuleDataSize
 * @param data
 */
static const void printSensorModuleRoughData(uint8_t sensorModuleId, uint8_t sensorModuleDataSize, uint8_t * data )
{
  int length = 40;
  int lengthPre = 3;
  char character = '*';
  printHeader(TS_OFF, VLEVEL_H, character, length, "Sensor Data");
  printSeparatorLine(TS_OFF, VLEVEL_H, character, lengthPre, false);
  APP_LOG(TS_OFF, VLEVEL_H, " Sensor module data: %d, %d", sensorModuleId, sensorModuleDataSize); //print sensor type

  for(int i=0; i < sensorModuleDataSize; i++)
  {
    APP_LOG(TS_OFF, VLEVEL_H, ", 0x%02x", data[i] ); //print data
  }
  APP_LOG(TS_OFF, VLEVEL_H, "\r\n" ); //print end
}

/**
 * @fn const void printSensorModulePressure(structDataPressureSensor*)
 * @brief helper function to print senosr module data to debug port
 *
 * @param pSensorData
 */
static const void printSensorModulePressure(structDataPressureSensor * pSensorData)
{
  int length = 40;
  int lengthPre = 3;
  char character = '*';
  uint8_t unitPress[] = "bar";
  uint8_t unitTemp[] = " C";
  unitTemp[0] = 176; //overwrite degree sign to ascii 176

  printSeparatorLine(TS_OFF, VLEVEL_H, character, length, true);
  printSeparatorLine(TS_OFF, VLEVEL_H, character, lengthPre, false);

  APP_LOG(TS_OFF, VLEVEL_H, " Sensor pressure data: %d.%02d %s, %d.%02d %s , %d.%02d %s, %d.%02d %s\r\n",
      (int)pSensorData->pressure1, getDecimal(pSensorData->pressure1, 2), unitPress,
      (int)pSensorData->temperature1, getDecimal(pSensorData->temperature1, 2), unitTemp,
      (int)pSensorData->pressure2, getDecimal(pSensorData->pressure2, 2), unitPress,
      (int)pSensorData->temperature2, getDecimal(pSensorData->temperature2, 2), unitTemp

      ); //print sensor data
}

/**
 * @fn const void printBaseData(struct_MFM_baseData*)
 * @brief function to print base data to debug port
 *
 * @param baseData
 */
static const void printBaseData(struct_MFM_baseData * baseData)
{
  int length = 40;
  int lengthPre = 3;
  char character = '*';

  printHeader(TS_OFF, VLEVEL_H, character, length, "Base Data");
  printSeparatorLine(TS_OFF, VLEVEL_H, character, lengthPre, false);
  APP_LOG(TS_OFF, VLEVEL_H, " Message Type: %d\r\n",  baseData->messageType);

  printSeparatorLine(TS_OFF, VLEVEL_H, character, lengthPre, false);
  APP_LOG(TS_OFF, VLEVEL_H, " EOS: %d\r\n",  baseData->batteryStateEos);

  if( baseData->messageType == 0x01)
  {
    printSeparatorLine(TS_OFF, VLEVEL_H, character, lengthPre, false);
    APP_LOG(TS_OFF, VLEVEL_H, " Temp BatteryGauge: %d\r\n",  baseData->temperatureGauge);
  }

  printSeparatorLine(TS_OFF, VLEVEL_H, character, lengthPre, false);
  APP_LOG(TS_OFF, VLEVEL_H, " Temp Controller: %d\r\n",  baseData->temperatureController);

  printSeparatorLine(TS_OFF, VLEVEL_H, character, lengthPre, false);
  APP_LOG(TS_OFF, VLEVEL_H, " Diagnostic: 0x%02x\r\n",  baseData->diagnosticBits);
  printSeparatorLine(TS_OFF, VLEVEL_H, character, length, true);
}

/**
 * @fn const void printCounters(void)
 * @brief helper function to print counters to debug port.
 *
 */
static const void printCounters(void)
{
  int length = 40;
  int lengthPre = 3;
  char character = '#';
  printHeader(TS_OFF, VLEVEL_H, character, length, "Dev Nonce");

  printSeparatorLine(TS_OFF, VLEVEL_H, character, lengthPre, false);
  APP_LOG(TS_OFF, VLEVEL_H, " DevNonce: %u\r\n", getDevNonce());
  printSeparatorLine(TS_OFF, VLEVEL_H, character, lengthPre, false);
  APP_LOG(TS_OFF, VLEVEL_H, " JoinNonce: %u\r\n", getJoinNonce());
  printSeparatorLine(TS_OFF, VLEVEL_H, character, lengthPre, false);
  APP_LOG(TS_OFF, VLEVEL_H, " DnFcnt: %u\r\n", getDownFCounter());
  printSeparatorLine(TS_OFF, VLEVEL_H, character, lengthPre, false);
  APP_LOG(TS_OFF, VLEVEL_H, " UpFcnt: %u\r\n", getUpFCounter());
  printSeparatorLine(TS_OFF, VLEVEL_H, character, length, true);
}

/**
 * @fn const void printFirmwareVersionInfo(void)
 * @brief function to print firmware info to debug port.
 *
 */
static const void printFirmwareVersionInfo(void)
{
  int length = 40;
  int lengthPre = 3;
  char character = 'X';
  printHeader(TS_OFF, VLEVEL_H, character, length, "FIRMWARE");

  printSeparatorLine(TS_OFF, VLEVEL_H, character, lengthPre, false);
  APP_LOG(TS_OFF, VLEVEL_H,  " MFM main version: %s\r\n", getSoftwareVersionMFM());

  printSeparatorLine(TS_OFF, VLEVEL_H, character, lengthPre, false);
  APP_LOG(TS_OFF, VLEVEL_H,  " MFM main protocol: %s\r\n", getProtocolVersionConfig());

  for(int i = 0; i<MAX_SENSOR_MODULE; i++)
  {
    printSeparatorLine(TS_OFF, VLEVEL_H, character, lengthPre, false);
    APP_LOG(TS_OFF, VLEVEL_H, " MFM SensorModule %d: %s, protocol: %d\r\n", i+1, getSoftwareSensorboard(i), getProtocolSensorboard(i));
  }

  printSeparatorLine(TS_OFF, VLEVEL_H, character, length, true);
}

#ifdef LORA_PERIODICALLY_CONFIRMED_MSG
/**
 * @fn const bool checkTxConfirmed(uint8_t, uint16_t)
 * @brief function to check for confirmed or unconfirmed message.
 * once every 24 measures, start at the 12th.
 *
 * @param numberOfModules : number of active sensor modules
 * @param counter : UpFCounter
 * @return
 */
static const bool checkTxConfirmed(uint8_t numberOfModules, uint16_t counter)
{
  uint8_t compare = numberOfModules == 0 ? 1 : numberOfModules; //when no modules are configured, use value equal to 1 module

  if( counter % (24 * compare) == 12 ) //once every 24 measures, start at the 12th.
  {
    return true;
  }
  else
  {
    return false;
  }
}
#endif


#ifdef LORA_PERIODICALLY_REQUEST_TIME
/**
 * @fn const bool checkPeriodicallyRequestTime(uint8_t, uint16_t)
 * @brief function to check if the periodically request time condition is valid, if so enable the request time.
 *
 * @param numberOfModules : number of active sensor modules
 * @param counter : UpFCounter
 */
static const bool checkPeriodicallyRequestTime(uint8_t numberOfModules, uint16_t counter)
{
  uint8_t compare = numberOfModules == 0 ? 1 : numberOfModules; //when no modules are configured, use value equal to 1 module

  if( counter % (24 * compare) == 12 ) //once every 24 measures, start at the 12th.
  {
    return true;
  }
  else
  {
    return false;
  }
}
#endif

/**
 * @fn void mainTask(void)
 * @brief periodically called mainTask for general functions and communication
 *
 */
const void mainTask(void)
{
  static UNION_diagnosticStatusBits diagnosticsStatusBits = {0};
  static bool forceRejoinByReset = false;

  mainTask_tmr++; //count the number of executes

  //execute steps of maintask, then wait for next trigger.
  switch( mainTask_state )
  {
    case INIT_POWERUP: //init Powerup

      stWakeupSource = getWakeupSource(); //get the wakeup source

      if( getWakeupBatStatus(1) )
      {
        restoreLatestTimeFromMeasurement(); //time in RTC not valid, set time from last measurement
        setRequestTime(); //request a time sync to server
      }
      else
      {
        //get time from RTC and sync it with controller clock
        syncSystemTime_withRTC();
      }
      getWakeupAlarmStatus(1); //reset awake alarm, future use.

      disableSleep();

      forceRejoinByReset = checkForceRejoin(stWakeupSource.byReset); //check for forced rejoin, only enabled when byReset is active

//      init_board_io_device(IO_EXPANDER_SYS); //done in MX_LoRaWAN_Init() -> SystemApp_Init();
      initLedTimer(); //init LED timer
      init_vAlwaysOn();
      executeAlwaysOn(); //execute Always on config value.

      initBatMon(); //initialize I2C peripheral for battery monitor

      MainPeriodSleep = getLoraInterval() * TM_SECONDS_IN_1MINUTE * 1000; //set default

      restoreFramSettingsStruct(&FRAM_Settings, sizeof(FRAM_Settings)); //read settings from FRAM

      printFirmwareVersionInfo(); //print firmware versions, after restore FRAM

      APP_LOG(TS_OFF, VLEVEL_H, "Restore diagnostic: BAT: %d, USB: %d, BOX: %d\r\n", FRAM_Settings.diagnosticBits.bit.batteryLow, FRAM_Settings.diagnosticBits.bit.usbConnected, FRAM_Settings.diagnosticBits.bit.lightSensorActive);

      mainTask_state = INIT_SLEEP; //Wake-up by alarm or normal power-up.

      printCounters();

      //check a forcedRejoinByReset is active
      if( forceRejoinByReset == true )
      {
        setForceInitSensor(true); //also activate a forced sensor init
        triggerReJoin();          //trigger a direct rejoin
        setOrangeLedOnOf(true);
      }

      //no forced Rejoin by reset active, then check wakeup source is not a valid alarm
      else if( alarmNotYetTriggered() )
      {
        mainTask_state = CHECK_USB_CONNECTED; //other wake-up, USB or other (not implemented) go to wait state
      }

      break;

    case INIT_SLEEP: //init after Sleep

#ifndef RTC_USED_FOR_SHUTDOWN_PROCESSOR
      if( UTIL_TIMER_IsRunning(&measurement_Timer) == 0)
      {
        systemActiveTime_sec = 0; //reset //only when no RTC is used, overwrite boottime.
#endif

        diagnosticsStatusBits = getDiagnostics(); //read current diagnostics
        FRAM_Settings.diagnosticBits.uint32 |= diagnosticsStatusBits.uint32; //OR the new reads with previous value from

        measureEOS_enabled = getBatteryEos().measureActive; //get saved status of previous round from battery backup registers

        if( measureEOS_enabled ) //only if measureEOS is enabled this round
        {
          APP_LOG(TS_OFF, VLEVEL_H, "Measure battery EOS\r\n" ); //print info
          batmon_enable(); //enable battery monitor, takes a while until batmon is ready.
        }

        if( enableListenUart )
        {
          uartListen(); //activate the config uart to process command, temporary consturction //todo change only listen when USB is attachted.
        }

#ifndef RTC_USED_FOR_SHUTDOWN_PROCESSOR
        loraJoinRetryCounter = 0; //reset, not needed for shutdown, because variable is always 0.
#endif

        mainTask_state = CHECK_LORA_JOIN;

#ifndef RTC_USED_FOR_SHUTDOWN_PROCESSOR
        }
#endif
      break;

    case CHECK_LORA_JOIN:

      if( LmHandlerJoinStatus() == LORAMAC_HANDLER_SET  )
      {
        //normal flow, start measuring
        mainTask_state = SWITCH_ON_VSYS;
      }

      else if( loraJoinRetryCounter > LORA_REJOIN_NUMBER_OF_RETRIES ) //check number of retries exceeded.
      {
        //go further normal way, so save a  measure in dataflash
        mainTask_state = SWITCH_ON_VSYS;
      }

      else if( waiting == false )
      {
        loraJoinRetryCounter++;
        triggerSendTxData(); //trigger Lora transmit, also triggers a join
        setWait(10000); //set wait timeout 10s, for possible next join
      }

      break;

    case SWITCH_ON_VSYS:

      enableVsys(); //enable supply for I/O expander
      MX_I2C2_Init(); //initialize I2C for BUS, otherwise it could not work because power was down.
      init_board_io_device(IO_EXPANDER_BUS_INT);
      init_board_io_device(IO_EXPANDER_BUS_EXT);

      mainTask_state = SWITCH_SENSOR_SLOT;

      break;

    case SWITCH_SENSOR_SLOT:

      numberOfActivesensorModules = 0;
      //check if at least one sensor module is enabled
      for( int i=0; i < MAX_SENSOR_MODULE; i++)
      {
        if( getSensorStatus(i + 1) == true )
        {
          numberOfActivesensorModules++; //count the number of active sensor Modules
        }

        if( getForceInitSensor() ) //check if init sensor is needed
        {
          FRAM_Settings.sensorModuleSettings[i].item.sensorModuleInitRequest = true; //enable request for each sensor.
        }
      }
      setForceInitSensor( false ); //reset after processed.

      FRAM_Settings.numberOfActiveSensorModules = numberOfActivesensorModules;
      currentSensorModuleIndex = FRAM_Settings.currentSensorModuleIndex; //get latest value.
      currentNumberOfSensorModule = FRAM_Settings.numberOfSensorModule; //get latest value.

      if( FRAM_Settings.numberOfActiveSensorModules > 0 ) //check sensorModule is enabled, found one module or more.
      {
        if( currentSensorModuleIndex < 0 || currentSensorModuleIndex >= MAX_SENSOR_MODULE )
        {
          currentSensorModuleIndex = 0; //force to first.
        }
        slotPower(currentSensorModuleIndex, true); //enable slot sensorModuleId (0-5)

        if( currentNumberOfSensorModule < 0 || currentNumberOfSensorModule > FRAM_Settings.numberOfActiveSensorModules) //validate not out of range, must be 1 until numberOfActivesensorModules
        {
          currentNumberOfSensorModule = 0; //force to first
        }

        setWait(10); //set wait time 10ms

        //check if sensor init is needed
        if( FRAM_Settings.sensorModuleSettings[currentSensorModuleIndex].item.sensorModuleInitRequest  )
        { //first execute init of sensor
          mainTask_state = CHECK_SENSOR_INIT_AVAILABLE; //next state
        }
        else
        { //no init needed

          mainTask_state = START_SENSOR_MEASURE; //next state
        }
      }

      else
      {
        APP_LOG(TS_OFF, VLEVEL_H, "No sensor module slot enabled\r\n" ); //print no sensor slot enabled


        if( measureEOS_enabled )
        {
          mainTask_state = WAIT_BATTERY_GAUGE_IS_ALIVE;
        }

        else
        {
          mainTask_state = SAVE_DATA;
        }

      }

#ifdef LORA_PERIODICALLY_CONFIRMED_MSG
      if (checkTxConfirmed(FRAM_Settings.numberOfActiveSensorModules, getUpFCounter()))
      {
        setTxConfirmed(LORAMAC_HANDLER_CONFIRMED_MSG);
      }
      else
      {
        setTxConfirmed(LORAMAC_HANDLER_UNCONFIRMED_MSG);
      }
#endif

#ifdef LORA_PERIODICALLY_REQUEST_TIME
      if( checkPeriodicallyRequestTime(FRAM_Settings.numberOfActiveSensorModules, getUpFCounter()) )
      {
        setRequestTime();
      }
#endif

      //check next battery measurement interval is active. Set flag in battery backup registers to measure next round the EOS from powerup.
      if( FRAM_Settings.nextIntervalBatteryEOS <= SysTimeGet().Seconds || FRAM_Settings.nextIntervalBatteryEOS == -1 || FRAM_Settings.nextIntervalBatteryEOS > SysTimeGet().Seconds + 2 * TM_SECONDS_IN_1DAY )
      {
        saveBatteryEos(true, (uint8_t)getBatteryEos().EOS, getBatteryEos().voltage); //request next interval EOS battery, also save previous values.
        FRAM_Settings.nextIntervalBatteryEOS = getNextBatteryEOStime(SysTimeGet().Seconds); //set new interval
        APP_LOG(TS_OFF, VLEVEL_H, "Next interval measure battery EOS\r\n" ); //print info
      }

      printCounters();

      break;

    case CHECK_SENSOR_INIT_AVAILABLE: //check if init command is available on sensor module.

      if( waiting == false ) //check wait time is expired
      {
        CommandStatus newStatus = sensorInitStatus(currentSensorModuleIndex);
        APP_LOG(TS_OFF, VLEVEL_H, "Sensor init status: %d, %d\r\n", currentSensorModuleIndex, newStatus ); //print sensor type

        if( newStatus == COMMAND_NOTAVAILABLE || newStatus == COMMAND_ERROR )
        {
          APP_LOG(TS_OFF, VLEVEL_H, "Sensor init: not available\r\n");
          mainTask_state = START_SENSOR_MEASURE; //skip sensor init -> start measure
        }

        else
        {
          setWait(10); //set wait time 10ms
          mainTask_state = START_SENSOR_INIT;
        }
      }

      break;

    case START_SENSOR_INIT:

      if( waiting == false ) //check wait time is expired
      {
        uint8_t result = sensorInitStart(currentSensorModuleIndex);

        APP_LOG(TS_OFF, VLEVEL_H, "Sensor init start: module: %d, result: %d\r\n", currentSensorModuleIndex, result ); //print sensor type

        setWait(50);  //set wait time of sensor
        setTimeout(10000); //10sec
        mainTask_state = WAIT_SENSOR_INIT_READY; //next state
      }

      break;

    case WAIT_SENSOR_INIT_READY:

      if( waiting == false ) //check wait time is expired
      {
        CommandStatus newStatus = sensorInitStatus(currentSensorModuleIndex);
        APP_LOG(TS_OFF, VLEVEL_H, "Sensor measure status: %d, %d\r\n", currentSensorModuleIndex, newStatus ); //print sensor type

        if( newStatus != COMMNAND_ACTIVE || timeout == true) //measurement ready or timeout
        {
          if( newStatus == COMMAND_DONE )
          {
            APP_LOG(TS_OFF, VLEVEL_H, "Sensor init: done\r\n");
            FRAM_Settings.sensorModuleSettings[currentSensorModuleIndex].item.sensorModuleInitRequest = false;
          }

          else if (newStatus == COMMAND_FAILED )
          {
            APP_LOG(TS_OFF, VLEVEL_H, "Sensor init: FAILED\r\n");
          }

          if( timeout == true )
          {
            APP_LOG(TS_OFF, VLEVEL_H, "Sensor init: timeout\r\n");
          }

          mainTask_state = START_SENSOR_MEASURE;
        }

        else
        {
          setWait(50);  //set wait time
        }

      }

      break;

    case START_SENSOR_MEASURE: //start measure

      if( waiting == false ) //check wait time is expired
      {
        uint8_t result = 0;
        uint8_t numberOfSamples = getNumberOfSamples(currentSensorModuleIndex + 1); //get configured number of samples

        result = sensorSetSamples(currentSensorModuleIndex, numberOfSamples); //write samples to sensor module
        APP_LOG(TS_OFF, VLEVEL_H, "Sensor module %d, result: %d, samples: %d\r\n", currentSensorModuleIndex, result, numberOfSamples); //print info

        memset(stMFM_sensorModuleData.sensorModuleData, 0x00, sizeof(stMFM_sensorModuleData.sensorModuleData));
        stMFM_sensorModuleData.sensorModuleSlotId = currentSensorModuleIndex; //save slotId

        memset(dataBuffer, 0x00, sizeof(dataBuffer));
        sensorFirmwareVersion(currentSensorModuleIndex, dataBuffer, sizeof(dataBuffer));
        strncpy(FRAM_Settings.modules[currentSensorModuleIndex].version, (char*)dataBuffer, sizeof(FRAM_Settings.modules[currentSensorModuleIndex].version)); //copy data to save to FRAM

        APP_LOG(TS_OFF, VLEVEL_H, "Sensor module firmware: %d, %s\r\n", currentSensorModuleIndex, dataBuffer ); //print VERSION

        sensorProtocol = 0; //reset first
        result = sensorProtocolVersion(currentSensorModuleIndex, &sensorProtocol);
        APP_LOG(TS_OFF, VLEVEL_H, "Sensor module protocol version: %d, %d\r\n", currentSensorModuleIndex, result == SENSOR_OK ? sensorProtocol : -1); //print protocol version
        stMFM_sensorModuleData.sensorModuleProtocolId = sensorProtocol; //save value
        FRAM_Settings.sensorModuleProtocol[currentSensorModuleIndex] = sensorProtocol; //save value to FRAM

        sensorType = 0; //reset first
        result = sensorReadType(currentSensorModuleIndex, &sensorType);
        APP_LOG(TS_OFF, VLEVEL_H, "Sensor module type: %d, %d\r\n", currentSensorModuleIndex, sensorType ); //print sensor type
        stMFM_sensorModuleData.sensorModuleTypeId = sensorType; //save value
        if( getSensorType(currentSensorModuleIndex + 1) != sensorType )
        {
          setSensorType(currentSensorModuleIndex + 1, sensorType); //save to configuration
          saveSettingsToVirtualEEPROM();
        }

        uint16_t measureTime = 0;
        result = sensorReadSetupTime(currentSensorModuleIndex, &measureTime); //get measureTime for sensorModule
        APP_LOG(TS_OFF, VLEVEL_H, "Sensor module measure time: %d, %u\r\n", currentSensorModuleIndex, measureTime ); //print sensor measure time

        if( measureTime == 65535) //check error value
        {
          measureTime = 100; //use default wait time
        }

        setWait(measureTime);  //set wait time of sensor
        setTimeout(10000 + measureTime); //+10sec timeout

        APP_LOG(TS_OFF, VLEVEL_H, "Sensor wait %ums, samples: %d\r\n", measureTime, getNumberOfSamples(currentSensorModuleIndex + 1) ); //print measure time

        sensorStartMeasurement(currentSensorModuleIndex); //start measure

        mainTask_state = WAIT_FOR_SENSOR_DATA; //next state
      }

      break;

    case WAIT_FOR_SENSOR_DATA:

      if( waiting == false ) //check wait time is expired
      {
        CommandStatus newStatus = sensorMeasurementStatus(currentSensorModuleIndex);
        APP_LOG(TS_OFF, VLEVEL_H, "Sensor measure status: %d, %d\r\n", currentSensorModuleIndex, newStatus ); //print sensor type

        if( (newStatus != COMMNAND_ACTIVE && newStatus != NO_ACTIVE_COMMAND) || timeout == true) //measurement ready or timeout
        {
          if( timeout == true )
          {
            APP_LOG(TS_OFF, VLEVEL_H, "Sensor measure: timeout\r\n");
          }

          if (measureEOS_enabled) //only if measureEOS is enabled this round
          {
            setTimeout(10000); //10sec
            mainTask_state = WAIT_BATTERY_GAUGE_IS_ALIVE; //next state
          }
          else
          {
            mainTask_state = READ_SENSOR_DATA;//skip battery
          }
        }
        else
        {
          setWait(50);  //set wait time 50ms
        }
      }

      break;

    case WAIT_BATTERY_GAUGE_IS_ALIVE:

      if (waiting == false)
      {
        bool gaugeReadyOrTimeout = false;
        if (batmon_isInitComplet()  ) //wait battery monitor is ready
        {
          APP_LOG(TS_OFF, VLEVEL_H, "Battery monitor: init complete\r\n");

          batmon_enable_gauge(); //enable gauging
          gaugeReadyOrTimeout = true;
        }
        else if( timeout == true ) //timeout
        {
          //do not enable gauge, go further reading sensor
          APP_LOG(TS_OFF, VLEVEL_H, "Battery monitor: timeout, init\r\n");
          gaugeReadyOrTimeout = true;
        }

        if( gaugeReadyOrTimeout == true )
        {
          if( FRAM_Settings.numberOfActiveSensorModules > 0  ) //check sensorModule is enabled, found one module or more.
          {
            mainTask_state = READ_SENSOR_DATA;
          }
          else
          {
            setTimeout(10000); //10sec
            mainTask_state = WAIT_GAUGE_IS_ACTIVE;
          }
        }

        setWait(200); //set wait 200ms
      }

      break;

    case READ_SENSOR_DATA: //read senor module measurement

      {
        SensorError newstatus = sensorReadMeasurement(currentSensorModuleIndex, dataBuffer, sizeof(stMFM_sensorModuleData.sensorModuleData) + 1 + 2);

        static_assert( sizeof(dataBuffer) >=  sizeof(stMFM_sensorModuleData.sensorModuleData) + sizeof(stMFM_sensorModuleData.sensorModuleDataSize) + sizeof(uint16_t));

        if( dataBuffer[0] <= sizeof(stMFM_sensorModuleData.sensorModuleData) )
        {
          stMFM_sensorModuleData.sensorModuleDataSize = dataBuffer[0];
          memcpy(&stMFM_sensorModuleData.sensorModuleData, dataBuffer+1, dataBuffer[0] ); //copy data
        }
        else
        {
          APP_LOG(TS_OFF, VLEVEL_H, "Sensormodule datasize too large, data is skipped\r\n");
        }

        if( newstatus == SENSOR_OK )
        {

          printSensorModuleRoughData( currentSensorModuleIndex, stMFM_sensorModuleData.sensorModuleDataSize, stMFM_sensorModuleData.sensorModuleData);

          if( sensorType == MFM_PREASURE_RS485 || sensorType == MFM_PREASURE_ONEWIRE)
          {
            printSensorModulePressure((structDataPressureSensor*)&stMFM_sensorModuleData.sensorModuleDataSize);
          }
        }
        else
        { //error
          stMFM_sensorModuleData.sensorModuleTypeId = 0; //reset
          stMFM_sensorModuleData.sensorModuleProtocolId = 0; //reset
          stMFM_sensorModuleData.sensorModuleDataSize = 0; //reset

          printSensorModuleError( newstatus ); //print error status to debug port.
        }

        slotPower(currentSensorModuleIndex, false); //disable slot sensorModuleId (0-5)

        waitForBatteryMonitorDataCounter = 0; //reset

        if (measureEOS_enabled) //only if measureEOS is enabled this round
        {
          setTimeout(10000); //10sec
          mainTask_state = WAIT_GAUGE_IS_ACTIVE; //next state
        }
        else
        {
          mainTask_state = SAVE_DATA; //Skip battery state
        }
      }

      break;

    case WAIT_GAUGE_IS_ACTIVE:
      if( waiting == false )
      {
        if( batmon_isGaugeActive() )
        {
          APP_LOG(TS_OFF, VLEVEL_H, "Battery monitor: gauge active\r\n");

          mainTask_state = WAIT_BATMON_DATA;
        }
        else if( timeout == true )
        {
          APP_LOG(TS_OFF, VLEVEL_H, "Battery monitor: timeout, gauge active\r\n");

          mainTask_state = WAIT_BATMON_DATA;
        }

        setWait(200); //set wait 200ms
      }

      break;

    case WAIT_BATMON_DATA:

      if( waiting == false ) //check wait time is expired
      {
        executeBatteryMeasure(); //do a battery measurement for battery supply, battery current and temperature. R and Z value not valid.

        if( batmon_getMeasure().voltage > 0 || waitForBatteryMonitorDataCounter > 10)
        {
          saveBatteryEos(false, batmon_getMeasure().stateOfHealth, batmon_getMeasure().voltage);
          mainTask_state = SAVE_DATA; //next state
        }
        else
        {
          waitForBatteryMonitorDataCounter++;
          setWait(100);  //set wait time 100msec
        }
      }


      break;

    case SAVE_DATA:

        if (measureEOS_enabled) //only if measureEOS is enabled this round
        {
          stMFM_baseData.messageType = 0x01;
          stMFM_baseData.batteryStateEos = batmon_getMeasure().stateOfHealth;
          stMFM_baseData.temperatureGauge = convertTemperatureGaugeToByte(batmon_getMeasure().temperature); //use gauge temperature
          stMFM_baseData.temperatureController = getTemperature(); //use controller temperature
        }
        else
        {
          stMFM_baseData.messageType = 0x02;
          stMFM_baseData.batteryStateEos = getBatteryEos().EOS; //get EOS value from battery backup registers
          stMFM_baseData.temperatureGauge = 0; //not available, set to zero
          stMFM_baseData.temperatureController = getTemperature(); //use controller temperature
        }

        stMFM_baseData.diagnosticBits = FRAM_Settings.diagnosticBits.byte[0]; //save the first 8 bits

        printBaseData(&stMFM_baseData);

        writeNewMeasurement(0, &stMFM_sensorModuleData, &stMFM_baseData);

        setOrangeLedOnOf(true); //enable led
        mainTask_state = SEND_LORA_DATA; //next state

      break;

    case SEND_LORA_DATA:

        if( waiting == false )
        {
          if( LmHandlerJoinStatus() == LORAMAC_HANDLER_SET              //check join is active
              ||                                                        //or
              loraJoinRetryCounter > LORA_REJOIN_NUMBER_OF_RETRIES )    //no join and number of retries exceeded.
          {
            loraTransmitReady = false; //reset before new transmit
            loraReceiveReady = false; //reset before new transmit

            triggerSendTxData(); //trigger Lora transmit
            mainTask_state = NEXT_SENSOR_MODULE; //next state
          }

          else
          { //join is not active,
            loraJoinRetryCounter++;
            triggerSendTxData(); //trigger Lora transmit, also trig
            setWait(10000);  //set wait time 10sec
          }

        }

      break;

    case NEXT_SENSOR_MODULE:

        if( FRAM_Settings.numberOfActiveSensorModules > 0 ) //check sensorModule is enabled, found one module or more.
        {
          APP_LOG(TS_OFF, VLEVEL_H, "Sensor module old: %d, %d\r\n", currentSensorModuleIndex, currentNumberOfSensorModule ); //print sensor module index
          int escape = MAX_SENSOR_MODULE;
          do
          {
            currentSensorModuleIndex++; //increment sensor ID
            currentSensorModuleIndex %= MAX_SENSOR_MODULE; //limit from 0 to 5.

          }while (getSensorStatus(currentSensorModuleIndex + 1) == false && escape--);

          currentNumberOfSensorModule++;
          currentNumberOfSensorModule %= FRAM_Settings.numberOfActiveSensorModules; //limit from 0 to numberOfActiveSensorModules.

          nextSensorInSameMeasureRound = currentNumberOfSensorModule ? true : false; //check if next is first, then not the same round

          for( int i = 0; i< (sizeof( FRAM_Settings.modules) / sizeof( FRAM_Settings.modules[0])); i++ )
          {
            FRAM_Settings.modules[i].nullTerminator = 0; //force null terminators
          }
          FRAM_Settings.currentSensorModuleIndex = currentSensorModuleIndex; //copy to save.
          FRAM_Settings.numberOfSensorModule = currentNumberOfSensorModule; //copy to save
        }

        APP_LOG(TS_OFF, VLEVEL_H, "Sensor module new: %d, %d, %d\r\n", currentSensorModuleIndex, currentNumberOfSensorModule, nextSensorInSameMeasureRound ); //print sensor module index

        setTimeout(10000); //10sec
        mainTask_state = WAIT_LORA_TRANSMIT_READY;

      break;

    case WAIT_LORA_TRANSMIT_READY:

      //wait on transmit ready flag
      if( loraTransmitReady == true || timeout == true)
      {
        bool transmitPossibleSuccess = false;
        if( timeout == true )
        {
          APP_LOG(TS_OFF, VLEVEL_H, "Lora transmit ready: timeout\r\n");
        }

        if (measureEOS_enabled) //only if measureEOS is enabled this round
        {
          executeBatteryMeasure(); //do a battery measurement for battery supply, battery current (of TX) and temperature. R and Z value not valid.
        }

        UTIL_TIMER_Time_t newLoraInterval = getLoraInterval() * TM_SECONDS_IN_1MINUTE * 1000;
        UTIL_TIMER_Time_t forcedInterval = 0;

        switch( loraTransmitStatus )
        {
          case LORAMAC_HANDLER_DUTYCYCLE_RESTRICTED:

            forcedInterval = getForcedLoraInterval();//get restricted dutycyle
            newLoraInterval = MAX(forcedInterval, newLoraInterval); //override forced interval based on Maximum of these.
            transmitPossibleSuccess = true;

            break;

          case LORAMAC_HANDLER_PAYLOAD_LENGTH_RESTRICTED:

            break;

          case LORAMAC_HANDLER_NVM_DATA_UP_TO_DATE:
          case LORAMAC_HANDLER_SUCCESS:

            transmitPossibleSuccess = true;

            break;

          case LORAMAC_HANDLER_ERROR: //transmission not started, no JOIN
          case LORAMAC_HANDLER_BUSY_ERROR:
          case LORAMAC_HANDLER_NO_NETWORK_JOINED:
          case LORAMAC_HANDLER_COMPLIANCE_RUNNING: //only for LORAMAC_VERSION == 0x01000300
          case LORAMAC_HANDLER_CRYPTO_ERROR:
          default:
            //failed, repeat again once
            triggerSaveNvmData2Fram(); //save NVM data, for succesfull it's saved

            break;
        }

        //check if transmit is success, then erase diagnostic.
        if( transmitPossibleSuccess == true )
        {
          FRAM_Settings.diagnosticBits.uint32 = 0; //reset status.
        }
        saveFramSettingsStruct(&FRAM_Settings, sizeof(FRAM_Settings)); //save FRAM data after last change

        MainPeriodSleep = newLoraInterval;

#ifndef RTC_USED_FOR_SHUTDOWN_PROCESSOR
        setNewMeasureTime(newLoraInterval); //set new interval to trigger new measurement
#endif

        /* check join is active */
        if( LmHandlerJoinStatus() == LORAMAC_HANDLER_SET)
        {
          if( getRejoinAtNextInterval() ) //check value is set, then reset is when join is active.
          {
            clearRejoinAtNextInterval();
          }
        }
        else
        {
          //nothing
        }

        if (measureEOS_enabled) //only if measureEOS is enabled this round
        {
          mainTask_state = GAUGE_DISABLE; //go to next state.
        }
        else
        {
          mainTask_state = SWITCH_OFF_VSYS; //Skip battery gauge
        }
      }

      break;

    case GAUGE_DISABLE:

      batmon_disable_gauge();
      mainTask_state = SWITCH_OFF_VSYS; //go to next state.

      break;

    case SWITCH_OFF_VSYS: //switch off for low power oparation

      deinit_IO_Expander(IO_EXPANDER_BUS_EXT);
      deinit_IO_Expander(IO_EXPANDER_BUS_INT);
      disableVsys();
      loraJoinRetryCounter = 0; //reset
      setTimeout(10000); //10sec
      mainTask_state = WAIT_LORA_RECEIVE_READY; //next state

      break;

    case WAIT_LORA_RECEIVE_READY:

      if( loraReceiveReady == true || !LoRaMacIsBusy() || timeout == true )
      {
        setOrangeLedOnOf(false); //disable led
        if( timeout == true )
        {
          APP_LOG(TS_OFF, VLEVEL_H, "Lora receive: timeout\r\n");
        }

        printCounters();

        if (measureEOS_enabled) //only if measureEOS is enabled this round
        {
          setTimeout(10000); //10sec
          mainTask_state = WAIT_BATTERY_MONITOR_READY; //go to next state.
        }
        else
        {
          mainTask_state = CHECK_USB_CONNECTED; //Skip battery gauge
        }

        setWait(100);  //set wait time 100msec
      }

      break;

    case WAIT_BATTERY_MONITOR_READY:

      if( waiting == false ) //check wait time is expired
      {
        //wait battery monitor saved data internally
        if( batmon_isReady() || timeout == true )
        {
          if( timeout == true )
          {
            APP_LOG(TS_OFF, VLEVEL_H, "Battery monitor isReady(): timeout\r\n");
          }

          executeBatteryMeasure();//do a battery measurement for battery supply, battery current, temperature, R and Z impedances ( the gauges must be stopped to get valid R and Z impedances).
          batmon_disable(); //switch off battery monitor
          mainTask_state = CHECK_USB_CONNECTED;
          setWait(100);  //set wait time 100ms
        }
        else
        {
          setWait(1000);  //set wait time 1000ms
        }
      }

      break;

    case CHECK_USB_CONNECTED:
#ifndef DEBUG_USB_ATTACHED
      // if USB attached, not going off mode
      if( getInput_board_io(EXT_IOUSB_CONNECTED) )
      {
        if( getForceSleepStatus() == true )
        {
          APP_LOG(TS_OFF, VLEVEL_H, "USB connected, force to sleep\r\n" );
          mainTask_state = WAIT_FOR_SLEEP;
        }
        else
        {
          APP_LOG(TS_OFF, VLEVEL_H, "USB connected, no off mode.\r\n" );

          MainPeriodSleep = getNextMeasureInterval(nextSensorInSameMeasureRound, MainPeriodSleep, FRAM_Settings.numberOfActiveSensorModules);
          uint32_t nextWakeTime = getNextWake( MainPeriodSleep, systemActiveTime_sec);
          setNewMeasureTime(nextWakeTime * 1000L); //set measure time
          setAlarmTime( calcAlarmTime(nextWakeTime)); //set new alarm time in RTC, only used for reset condition.

          mainTask_state = WAIT_USB_DISCONNECT; //go to next state
        }
      }
      else
#endif
      {
        APP_LOG(TS_OFF, VLEVEL_H, "No USB found\r\n" );
        mainTask_state = WAIT_FOR_SLEEP;
      }

      break;

    case WAIT_USB_DISCONNECT:

      //check measure time expired
      if( startMeasure || getForceMeasurement() )
      {
        setForceMeasurement( false ); //reset status.
        systemActiveTime_sec = 0; //reset //only when not in off mode
        mainTask_state = INIT_SLEEP;
      }
      //check USB disconnected
      else if( !getInput_board_io(EXT_IOUSB_CONNECTED) )
      {
        APP_LOG(TS_OFF, VLEVEL_H, "USB disconnected, enter off mode.\r\n" );
        setWait(100);  //set wait time 100ms
        mainTask_state = WAIT_FOR_SLEEP;
      }
      //check if system must be forced to sleep
      else if( getForceSleepStatus() == true )
      {
        APP_LOG(TS_OFF, VLEVEL_H, "Forced to sleep, enter off mode.\r\n" );
        setWait(100);  //set wait time 100ms
        mainTask_state = WAIT_FOR_SLEEP;
      }

      break;

    case WAIT_FOR_SLEEP:

      if( waiting == false ) //check wait time is expired
      {
        //make sure diagnostic is read before sleep and saved to FRAM
        diagnosticsStatusBits = getDiagnostics(); //read current diagnostics
        FRAM_Settings.diagnosticBits.uint32 |= diagnosticsStatusBits.uint32; //OR the new reads with previous value from
        saveFramSettingsStruct(&FRAM_Settings, sizeof(FRAM_Settings)); //save FRAM data after last change

        control_supercap(false); //disable supercap before sleep

#ifdef RTC_USED_FOR_SHUTDOWN_PROCESSOR

        UTIL_TIMER_Time_t sleepTime; //in ms

        if( FRAM_Settings.numberOfActiveSensorModules > 0 ) //set sleep time when sensor is active, found one module or more.
        {
          sleepTime = getNextMeasureInterval(nextSensorInSameMeasureRound, MainPeriodSleep, FRAM_Settings.numberOfActiveSensorModules); //get new sleep time
        }
        else //set sleep time very long when no sensor is active.
        {
          sleepTime = TM_SECONDS_IN_1DAY * 1000;
        }

        goIntoSleep(getNextWake( sleepTime, systemActiveTime_sec), 1);
        //will stop here
#else
        pause_mainTask();

        APP_LOG(TS_OFF, VLEVEL_H, "WAIT: %u\r\n",  getNextWake( MainPeriodSleep, systemActiveTime_sec) );

        mainTask_state = INIT_SLEEP; //go back to init after sleep, for next measure
#endif
      }

      break;

    default:

      break;
  }

  update_board_io(); //periodically read

  if( getAlwaysOn_changed(true)) //check vAlwaysOn setting is changed
  {
    APP_LOG(TS_OFF, VLEVEL_H, "Setting Supply V-always changed: %d\r\n", getAlwaysOn() );
    executeAlwaysOn(); //execute vAlwaysOn enable/disable
  }

  //check boolean mainTaskActive, then set short period for triggering, if not set long period for triggering.
  if( mainTaskActive )
  {
    if( wait_Timer.Timestamp > MainPeriodNormal )
    {
      APP_LOG(TS_OFF, VLEVEL_H, "MainTask sleep tick: %u, time: %ums, state %d\r\n", wait_Timer.Timestamp, TIMER_IF_Convert_Tick2ms(wait_Timer.Timestamp), mainTask_state );
      setNextPeriod(TIMER_IF_Convert_Tick2ms(wait_Timer.Timestamp));
    }

    else
    {
      setNextPeriod(MainPeriodNormal);
    }

  }
  else
  {
    //check if measurement_Timer is running, otherwise force run.
    if( UTIL_TIMER_IsRunning(&measurement_Timer) == 0)
    { //not running, force next Period.
      setNextPeriod(MainPeriodSleep);
      setNewMeasureTime(MainPeriodSleep);
    }
  }

  control_supercap( getInput_board_io(INT_IO_3V3_DETECT) == GPIO_PIN_SET ? false : true); //enable the supercap without 3V3_detect (indirect USB/VBUS connected), disable the supercap when USB is found
  uartKeepListen( getInput_board_io(EXT_IOUSB_CONNECTED) ); //if USB is connected, keep listen to UART.

  //exit, wait on next trigger.

}

/**
 * @fn const void trigger_mainTask_timer(void)
 * @brief function to trigger the MainTask to schedule.
 *
 */
static const void trigger_mainTask_timer(void)
{
  mainTaskActive = true;
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_Main), CFG_SEQ_Prio_0); //trigger task for the scheduler
}

/**
 * @fn const void trigger_mainTask(void)
 * @brief function to trigger the mainTask function when the task is stopped or in pause
 *
 */
static const void trigger_mainTask(void *context)
{
  trigger_mainTask_timer(); //trigger task for the scheduler
}

/**
 * @fn const void trigger_wait(void*)
 * @brief function to trigger after wait timer is finished.
 *
 * @param context
 */
static const void trigger_wait(void *context)
{
  waiting = false;
}

/**
 * @fn const void trigger_measure(void*)
 * @brief function to trigger after measure timer is finished.
 *
 * @param context
 */
static const void trigger_measure(void *context)
{
  startMeasure = true;
  uartListen(); //todo remove, used for test.
  resume_mainTask();
}

/**
 * @fn const void trigger_delayedReJoin(void*)
 * @brief function to trigger delayed rejoin
 *
 * @param context
 */
static const void trigger_delayedReJoin(void *context)
{
  triggerReJoin();
}

/**
 * @fn const void trigger_systemActiveTime(void*)
 * @brief function to trigger system active time
 *
 * @param context
 */
static const void trigger_systemActiveTime(void *context)
{
  systemActiveTime_sec++; //increment every second
}

/**
 * @fn const void trigger_timeout(void*)
 * @brief function to trigger after timeout timer is finished.
 *
 * @param context
 */
static const void trigger_timeout(void *context)
{
  timeout = true;
}

/**
 * @fn const void init_mainTask(void)
 * @brief function to initialize the mainTask
 *
 */
const void init_mainTask(void)
{

#if VERBOSE_LEVEL == VLEVEL_H

  char timeStringNow[20] = {0};
  struct tm stTime;
  SysTimeLocalTime(SysTimeGet().Seconds, &stTime); //get system time
  strftime(timeStringNow, sizeof(timeStringNow), "%d-%m-%Y %H:%M:%S", &stTime); //make date/time string
  APP_LOG(TS_OFF, VLEVEL_H, "SysTime: sec:%d, subSec: %d, %s\r\n", SysTimeGet().Seconds, SysTimeGet().SubSeconds, timeStringNow); //print values for debug

#endif

  mainTask_state = INIT_POWERUP; //reset state for powerup
  mainTaskActive = true; //start the main task
  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_Main), UTIL_SEQ_RFU, mainTask); //register the task at the scheduler

  UTIL_TIMER_Create(&MainTimer, MainPeriodNormal, UTIL_TIMER_ONESHOT, trigger_mainTask, NULL); //create timer
  UTIL_TIMER_Start(&MainTimer); //start timer

  UTIL_TIMER_Create(&wait_Timer, 0, UTIL_TIMER_ONESHOT, trigger_wait, NULL); //create timer

  UTIL_TIMER_Create(&measurement_Timer, 0, UTIL_TIMER_ONESHOT, trigger_measure, NULL); //create timer

  UTIL_TIMER_Create(&rejoin_Timer, 0, UTIL_TIMER_ONESHOT, trigger_delayedReJoin, NULL); //create timer

  UTIL_TIMER_Create(&systemActive_Timer, period_1sec, UTIL_TIMER_PERIODIC, trigger_systemActiveTime, NULL); //create timer
  UTIL_TIMER_Start(&systemActive_Timer); //start timer

  UTIL_TIMER_Create(&timeout_Timer, 0, UTIL_TIMER_ONESHOT, trigger_timeout, NULL); //create timer

}


/**
 * @fn const void stop_mainTask(void)
 * @brief function to stop the mainTask
 *
 */
const void stop_mainTask(bool resume)
{
  mainTask_state = INIT_SLEEP; //reset state for STOP2 mode
  mainTaskActive = false;
  enableListenUart = false;

  if( resume )
  {
    UTIL_TIMER_Stop(&MainTimer);
    UTIL_TIMER_SetPeriod(&MainTimer, MainPeriodSleep);
    UTIL_TIMER_Start(&MainTimer);
  }
}

/**
 * @fn const void pause_mainTask(void)
 * @brief function to pause the mainTask
 *
 */
const void pause_mainTask(void)
{
  mainTaskActive = false;
}

/**
 * @fn const void resume_mainTask(void)
 * @brief function to resume the mainTask after stop or pause
 *
 */
const void resume_mainTask(void)
{
  trigger_mainTask_timer();
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if( GPIO_Pin == MCU_IRQ_Pin )
  {
    trigger_mainTask_timer();
    enableListenUart = true;
    UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LoRaStoreContextEvent), CFG_SEQ_Prio_0);
  }
}

/**
 * @fn const void setNewTxInterval_usr(void)
 * @brief override function to disable periodically set timer from lora_app.c SendTxData() function.
 * empty function.
 *
 */
const void setNewTxInterval_usr(LmHandlerErrorStatus_t status)
{
  loraTransmitReady = true;
}

/**
 * @fn const void rxDataUsrCallback(LmHandlerAppData_t*)
 * @brief override function to handle uplink data in user application
 * FPort 0x69, command 0x55 will force a rejoin
 *
 * @param appData : received data
 */
const void rxDataUsrCallback(LmHandlerAppData_t *appData)
{
  //check buffer is not NULL, no data
  if (appData->Buffer == NULL)
  {
    APP_LOG(TS_OFF, VLEVEL_H, "Lora receive: buffer NULL\r\n" );
    return;
  }

  //check Lora reserved ports
  if( appData->Port ==0 || appData->Port > 223 )
  {
     APP_LOG(TS_OFF, VLEVEL_H, "Lora receive: reserved port\r\n" );
     return;
   }

  //check Lora_app used ports, see OnRxData() in LoRaWAN\App\lora_app.c
  if( appData->Port > 0 && appData->Port <=3)
  {
    APP_LOG(TS_OFF, VLEVEL_H, "Lora receive: used lora_app port\r\n");
    return;
  }

  //check buffersize is at least 2 for MFM protocol
  if( appData->BufferSize > 2 )
  {
     APP_LOG(TS_OFF, VLEVEL_H, "Lora receive: More data as expected\r\n");
     return;
   }

  uint8_t command = appData->Buffer[0]; //MFM command byte
  uint8_t optionalByte = appData->Buffer[1]; //reserved for optional byte for command

  UNUSED(optionalByte); //not yet used, prevent warning.

  switch ( appData->Port )
  {

    case 0x69: //MFM command

      switch (command)
      {
        case 0x55: //command for rejoin in 5 minutes

          //check command buffer matches
          if(  appData->BufferSize == 1 )
          {
            //trigger rejoin
            APP_LOG(TS_OFF, VLEVEL_H, "Lora receive: Rejoin received\r\n" ); //print no sensor slot enabled
#ifdef RTC_USED_FOR_SHUTDOWN_PROCESSOR
            setRejoinAtNextInterval(); //set a rejoin for next interval

            if( getInput_board_io(EXT_IOUSB_CONNECTED) ) //check USB is connected, then also set delayedReojoin.
            {
              setDelayReJoin(measurement_Timer.Timestamp > 1000 ? measurement_Timer.Timestamp - 1000 : 1);
            }
#else
            setDelayReJoin(10000); //set a delay ReJoin after 10 seconds. At Join the NVM is read. NVM needs to be saved before new join starts.
#endif
          }
          else
          {
            APP_LOG(TS_OFF, VLEVEL_H, "Lora receive: More data as expected\r\n");
          }

          break;

        default:

          //nothing
          APP_LOG(TS_OFF, VLEVEL_H, "Lora receive: No action defined\r\n");

          break;
      }

      break;

    default:

      //nothing

      break;
  }
}

/**
 * @fn const void rxDataReady(void)
 * @brief override function to signal lora rxData is processed
 *
 */
const void rxDataReady(void)
{
  loraReceiveReady = true;
}
