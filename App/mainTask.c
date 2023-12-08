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

#include "main.h"
#include "sys_app.h"
#include "utilities.h"
#include "common/common.h"
#include "common/app_types.h"
#include "utilities_def.h"
#include "stm32_seq.h"
#include "stm32_timer.h"
#include "stm32_systime.h"

#include "lora_app.h"
#include "LoRaMac.h"

#include "IO/board_io.h"
#include "IO/board_io_functions.h"
#include "IO/led.h"
#include "FRAM/FRAM_functions.h"
#include "I2CMaster/SensorFunctions.h"
#include "logging/logging.h"
#include "BatMon_BQ35100/BatMon_functions.h"
#include "RTC_AM1805/RTC_functions.h"
#include "CommConfig.h"
#include "MFMconfiguration.h"
#include "mainTask.h"

#define LORA_PERIODICALLY_CONFIRMED_MSG //comment if feature must be disabled.
#define RTC_USED_FOR_SHUTDOWN_PROCESSOR //comment if feature must be disabled. //if enabled jumper on J11 1-2 must be placed.

static volatile bool mainTaskActive;
static uint32_t mainTask_tmr;
static int mainTask_state;
static int loraJoinRetryCounter = 0;

static UTIL_TIMER_Object_t MainTimer;
static UTIL_TIMER_Time_t MainPeriodSleep = 60000;
static UTIL_TIMER_Time_t MainPeriodNormal = 10;
static bool enableListenUart;

static UTIL_TIMER_Object_t wait_Timer;
static volatile bool waiting = false;

static UTIL_TIMER_Object_t measurement_Timer;
static volatile bool startMeasure = true;

static UTIL_TIMER_Object_t rejoin_Timer;

static UTIL_TIMER_Object_t systemActive_Timer;
static UTIL_TIMER_Time_t period_1sec = 1000;
static volatile uint32_t systemActiveTime_sec;

static uint8_t dataBuffer[100];
static uint8_t sensorModuleId = 0;
static bool sensorModuleEnabled = false;
static uint8_t numberOfsensorModules = 0;
static volatile bool loraTransmitReady = false;
static volatile bool loraReceiveReady = false;
static LmHandlerErrorStatus_t loraTransmitStatus;

static uint16_t sensorType;
static uint8_t sensorProtocol;

static uint8_t waitForBatteryMonitorDataCounter = 0;

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
 * @fn const uint16_t getDevNonce(void)
 * @brief function to get the DevNonce counter
 *
 * @return devNonce counter
 */
static const uint16_t getDevNonce(void)
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
 * @fn void mainTask(void)
 * @brief periodically called mainTask for general functions and communication
 *
 */
const void mainTask(void)
{

  mainTask_tmr++; //count the number of executes

  //execute steps of maintask, then wait for next trigger.
  switch( mainTask_state )
  {
    case INIT_POWERUP: //init Powerup

      APP_LOG(TS_OFF, VLEVEL_H, "RTC: status: ");
      APP_LOG(TS_OFF, VLEVEL_H, "0x%02x", am1805_get_status(0));

      //check BAT flag is set, indicate battery disconnected.
      if( getWakeupBatStatus(1) )
      {
        APP_LOG(TS_OFF, VLEVEL_H, ", BAT");

        restoreLatestTimeFromLog(); //time in RTC not valid, set time from last log
        setRequestTime(); //request a time sync to server
      }
      else
      {
        //get time from RTC and sync it with controller clock
        syncSystemTime_withRTC();
      }

      //check alarm, indicates awake from alarm
      if( getWakeupAlarmStatus(1) )
      {
        APP_LOG(TS_OFF, VLEVEL_H, ", ALARM");
      }

      APP_LOG(TS_OFF, VLEVEL_H, "\r\n");

      disableSleep();
//      init_board_io_device(IO_EXPANDER_SYS); //done in MX_LoRaWAN_Init() -> SystemApp_Init();
      initLedTimer(); //init LED timer
      init_vAlwaysOn();
      executeAlwaysOn(); //execute Always on config value.

      initBatMon(); //initialize and enable battery gauge

      mainTask_state = INIT_SLEEP;
      break;

    case INIT_SLEEP: //init after Sleep

      if( UTIL_TIMER_IsRunning(&measurement_Timer) == 0)
      {

#ifndef RTC_USED_FOR_SHUTDOWN_PROCESSOR
        systemActiveTime_sec = 0; //reset //only when no RTC is used, overwrite boottime.
#endif

        batmon_enable(); //enable battery monitor, takes a while until batmon is ready.

        if( enableListenUart )
        {
          uartListen(); //activate the config uart to process command, temporary consturction //todo change only listen when USB is attachted.
        }

        setWait(100); //set wait timeout 250ms

        mainTask_state = WAIT_BATTERY_GAUGE_INIT;
        }
      break;

    case WAIT_BATTERY_GAUGE_INIT:

     if( waiting == false  )
     {
       if( batmon_isInitComplet()) //wait battery monitor is ready
       {
         APP_LOG(TS_OFF, VLEVEL_H, "Battery monitor: init complete\r\n");

         batmon_enable_gauge(); //enable gauging
         mainTask_state = WAIT_GAUGE_ENABLED;
       }

       setWait(100); //set wait timeout 1s
     }

      break;

    case WAIT_GAUGE_ENABLED:
      if( waiting == false )
      {
        if( batmon_isGaugeActive() )
        {
          APP_LOG(TS_OFF, VLEVEL_H, "Battery monitor: gauge active\r\n");
          mainTask_state = ENABLE_VSYS;
        }

        setWait(100); //set wait timeout 1s
      }

      break;

    case ENABLE_VSYS:

      enableVsys(); //enable supply for I/O expander
      init_board_io_device(IO_EXPANDER_BUS_INT);
      init_board_io_device(IO_EXPANDER_BUS_EXT);

      mainTask_state = CHECK_SENSOR_SLOT;

      break;

    case CHECK_SENSOR_SLOT:

      numberOfsensorModules = 0;
      sensorModuleEnabled = false;
      //check if at least one sensor module is enabled
      for( int i=0; i < MAX_SENSOR_MODULE; i++)
      {
        if( getSensorStatus(i + 1) == true )
        {
          numberOfsensorModules++;
          sensorModuleEnabled = true; //a module found, copy
        }
      }

      if( sensorModuleEnabled )
      {
        mainTask_state = ENABLE_SLOTPOWER; //at least one active sensor module slot

        getFramSetting(FRAM_SETTING_MODEMID, (void*)&sensorModuleId, true); //read out FRAM setting module ID

        if( sensorModuleId < 0 || sensorModuleId >= MAX_SENSOR_MODULE )
        {
          sensorModuleId = 0; //force to first.
        }

#ifdef LORA_PERIODICALLY_CONFIRMED_MSG
        /* get DevNonce for set confirmed / unconfirmed messages */
        if( getDevNonce() % (24 * numberOfsensorModules) == 12 ) //once every 24 measures, start at the 12th.
        {
          setTxConfirmed(LORAMAC_HANDLER_CONFIRMED_MSG);
        }
        else
        {
          setTxConfirmed(LORAMAC_HANDLER_UNCONFIRMED_MSG);
        }
#endif

        if( getDevNonce() % (24 * numberOfsensorModules) == 12 ) //once every 24 measures, start at the 12th.
        {
          setRequestTime();
        }

      }
      else
      {
        APP_LOG(TS_OFF, VLEVEL_H, "No sensor module slot enabled\r\n" ); //print no sensor slot enabled
        mainTask_state = STOP_MAINTASK; //no sensor slot is active
      }

      break;

    case ENABLE_SLOTPOWER: //enable sensor supply

      if( startMeasure == true )
      {
        slotPower(sensorModuleId, true); //enable slot sensorModuleId (0-5)
        setWait(10); //set wait time 10ms
        mainTask_state = START_SENSOR_MEASURE; //next state
      }

      else
      {
        //keep waiting.
      }

      break;

    case START_SENSOR_MEASURE: //start measure

      if( waiting == false ) //check wait time is expired
      {
        sensorStartMeasurement(sensorModuleId); //start measure

        memset(dataBuffer, 0x00, sizeof(dataBuffer));
        sensorFirmwareVersion(sensorModuleId, dataBuffer, sizeof(dataBuffer));

        APP_LOG(TS_OFF, VLEVEL_H, "Sensor module firmware: %d, %s\r\n", sensorModuleId, dataBuffer ); //print VERSION

        uint8_t result;
        sensorProtocol = 0; //reset first
        result = sensorProtocolVersion(sensorModuleId, &sensorProtocol);
        APP_LOG(TS_OFF, VLEVEL_H, "Sensor module protocol version: %d, %d\r\n", sensorModuleId, result == SENSOR_OK ? sensorProtocol : -1); //print protocol version

        sensorType = 0; //reset first
        result = sensorReadType(sensorModuleId, &sensorType);
        APP_LOG(TS_OFF, VLEVEL_H, "Sensor module type: %d, %d\r\n", sensorModuleId, sensorType ); //print sensor type

        uint16_t sensorSetupTime = 0;
        result = sensorReadSetupTime(sensorModuleId, &sensorSetupTime);
        APP_LOG(TS_OFF, VLEVEL_H, "Sensor module setup time: %d, %u\r\n", sensorModuleId, sensorSetupTime ); //print sensor setup time

        setWait(250);  //set wait time 250ms

        mainTask_state = WAIT_FOR_SENSOR_DATA; //next state
      }

      break;

    case WAIT_FOR_SENSOR_DATA:

      if( waiting == false ) //check wait time is expired
      {
        MeasurementStatus newStatus = sensorMeasurementStatus(sensorModuleId);
        APP_LOG(TS_OFF, VLEVEL_H, "Sensor measure status: %d, %d\r\n", sensorModuleId, newStatus ); //print sensor type

        if( newStatus != MEASUREMENT_ACTIVE )
        {
          mainTask_state = READ_SENSOR_DATA; //next state
        }
        else
        {
          setWait(100);  //set wait time 100ms
        }
      }

      break;

    case READ_SENSOR_DATA: //read senor module measurement

      {
        SensorError newstatus = sensorReadMeasurement(sensorModuleId, dataBuffer, sizeof(dataBuffer));
        if( newstatus == SENSOR_OK )
        {
          APP_LOG(TS_OFF, VLEVEL_H, "Sensor module data: %d, %d, 0x%02x", sensorModuleId, newstatus, dataBuffer[0] ); //print sensor type

          for(int i=0; i < dataBuffer[0]; i++)
          {
            APP_LOG(TS_OFF, VLEVEL_H, ", 0x%02x", dataBuffer[i+1] ); //print data
          }
          APP_LOG(TS_OFF, VLEVEL_H, "\r\n" ); //print end

          if( sensorType == MFM_PREASURE_RS485 || sensorType == MFM_PREASURE_ONEWIRE)
          {
            uint8_t unitPress[] = "bar";
            uint8_t unitTemp[] = " C";
            unitTemp[0] = 176; //overwrite degree sign to ascii 176

            structDataPressureSensor * pSensorData = (structDataPressureSensor*)&dataBuffer[0];
            APP_LOG(TS_OFF, VLEVEL_H, "Sensor pressure data: %d.%02d %s, %d.%02d %s , %d.%02d %s, %d.%02d %s\r\n",
                (int)pSensorData->pressure1, getDecimal(pSensorData->pressure1, 2), unitPress,
                (int)pSensorData->temperature1, getDecimal(pSensorData->temperature1, 2), unitTemp,
                (int)pSensorData->pressure2, getDecimal(pSensorData->pressure2, 2), unitPress,
                (int)pSensorData->temperature2, getDecimal(pSensorData->temperature2, 2), unitTemp


                ); //print sensor data
          }
        }
        else
        {
          APP_LOG(TS_OFF, VLEVEL_H, "Sensor module data: ERROR, " ); //print error
          switch (newstatus)
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
            memset(dataBuffer, 0x00, sizeof(dataBuffer));
        }

        slotPower(sensorModuleId, false); //disable slot sensorModuleId (0-5)

        waitForBatteryMonitorDataCounter = 0; //reset

        mainTask_state = WAIT_BATMON_DATA; //next state
      }

      break;

    case WAIT_BATMON_DATA:

      if( waiting == false ) //check wait time is expired
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

        if( batmon_getMeasure().voltage > 0 || waitForBatteryMonitorDataCounter > 10)
        {
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

      writeNewLog(sensorModuleId, sensorType, sensorProtocol, &dataBuffer[1], dataBuffer[0]); //write log data to dataflash.
      mainTask_state = SEND_LORA_DATA; //next state

      break;

    case SEND_LORA_DATA:

      loraTransmitReady = false; //reset before new transmit
      loraReceiveReady = false; //reset before new transmit

      triggerSendTxData(); //trigger Lora transmit
      mainTask_state = NEXT_SENSOR_MODULE; //next state

      break;

    case NEXT_SENSOR_MODULE:

      {
        int escape = MAX_SENSOR_MODULE;
        do
        {
          sensorModuleId++; //increment sensor ID
          sensorModuleId %= MAX_SENSOR_MODULE; //limit from 0 to 5.

        }while (getSensorStatus(sensorModuleId + 1) == false && escape--);


        setFramSetting(FRAM_SETTING_MODEMID, (void*)&sensorModuleId, true); //save last sensor Moudle ID


        mainTask_state = WAIT_LORA_TRANSMIT_READY;
      }

      break;

    case WAIT_LORA_TRANSMIT_READY:

      //wait on transmit ready flag
      if( loraTransmitReady == true )
      {
        UTIL_TIMER_Time_t newLoraInterval = getLoraInterval() * TM_SECONDS_IN_1MINUTE * 1000;
        UTIL_TIMER_Time_t forcedInterval = 0;

        switch( loraTransmitStatus )
        {
          case LORAMAC_HANDLER_DUTYCYCLE_RESTRICTED:

            forcedInterval = getForcedLoraInterval();//get restricted dutycyle
            newLoraInterval = MAX(forcedInterval, newLoraInterval); //override forced interval based on Maximum of these.

            break;

          case LORAMAC_HANDLER_PAYLOAD_LENGTH_RESTRICTED:

            break;

          case LORAMAC_HANDLER_NVM_DATA_UP_TO_DATE:

            break;

          case LORAMAC_HANDLER_SUCCESS:

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

        MainPeriodSleep = newLoraInterval;
        setNewMeasureTime(newLoraInterval); //set new interval to trigger new measurement

        //check JOIN is not active
        if (LmHandlerJoinStatus() == LORAMAC_HANDLER_RESET) // JOIN is not active
        { //retry it after ..
          setWait(10000);  //set wait time 10sec
          mainTask_state = CHECK_LORA_JOINED; //next state
        }
        else
        {
          mainTask_state = SWITCH_OFF_VSYS; //increment again, to skip retry for join.
        }
      }

      break;

    case CHECK_LORA_JOINED:

      if( waiting == false ) //check wait time is expired
      {
        if( LmHandlerJoinStatus() == LORAMAC_HANDLER_SET || loraJoinRetryCounter > 5 )
        {
          mainTask_state = SWITCH_OFF_VSYS; //next state
        }
        else
        { //try again
          loraJoinRetryCounter++;
          triggerSendTxData(); //trigger Lora transmit
          mainTask_state = WAIT_LORA_TRANSMIT_READY; //previous state
        }
      }

      break;

    case SWITCH_OFF_VSYS: //switch off for low power oparation

      deinit_IO_Expander(IO_EXPANDER_BUS_EXT);
      deinit_IO_Expander(IO_EXPANDER_BUS_INT);
      disableVsys();
      loraJoinRetryCounter = 0; //reset
      mainTask_state = CHECK_LORA_REJOIN; //next state

      break;

    case CHECK_LORA_REJOIN:

      //check rejoin is not active
      if( !UTIL_TIMER_IsRunning(&rejoin_Timer))
      {
        mainTask_state = WAIT_LORA_RECEIVE_READY;
      }

      break;

    case WAIT_LORA_RECEIVE_READY:

      if( loraReceiveReady == true || !LoRaMacIsBusy() )
      {
        batmon_disable_gauge();
        mainTask_state = WAIT_BATTERY_MONITOR_READY;
        setWait(100);  //set wait time 100msec
      }

      break;

    case WAIT_BATTERY_MONITOR_READY:

      if( waiting == false ) //check wait time is expired
      {
        //wait battery monitor saved data internally
        if( batmon_isReady() )
        {
          batmon_disable(); //switch off battery monitor
          mainTask_state = WAIT_FOR_SLEEP;
        }
        setWait(100);  //set wait time 100ms
      }

      break;

    case WAIT_FOR_SLEEP:

      if( waiting == false ) //check wait time is expired
      {

#ifdef RTC_USED_FOR_SHUTDOWN_PROCESSOR

        goIntoSleep(getNextWake( MainPeriodSleep, systemActiveTime_sec), 1);
        //will stop here
#else
        pause_mainTask();

        APP_LOG(TS_OFF, VLEVEL_H, "WAIT: %u\r\n",  getNextWake( MainPeriodSleep, systemActiveTime_sec) );

        mainTask_state = INIT_SLEEP; //go back to init after sleep, for next measure
#endif
      }

      break;

    case STOP_MAINTASK:

      stop_mainTask(true);

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
  //or if USB is connected
  if( mainTaskActive ||  getInput_board_io(EXT_IOUSB_CONNECTED) )
  {
    setNextPeriod(MainPeriodNormal);
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
            setDelayReJoin(10000); //set a delay ReJoin after 10 seconds. At Join the NVM is read. NVM needs to be saved before new join starts.
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
