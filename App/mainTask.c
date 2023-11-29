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
#include "LmHandlerTypes.h"

#include "IO/board_io.h"
#include "IO/board_io_functions.h"
#include "IO/led.h"
#include "FRAM/FRAM_functions.h"
#include "I2CMaster/SensorFunctions.h"
#include "logging/logging.h"
#include "CommConfig.h"
#include "MFMconfiguration.h"
#include "mainTask.h"

static volatile bool mainTaskActive;
static uint32_t mainTask_tmr;
static int mainTask_state;

static UTIL_TIMER_Object_t MainTimer;
static UTIL_TIMER_Time_t MainPeriodSleep = 60000;
static UTIL_TIMER_Time_t MainPeriodNormal = 10;
static bool enableListenUart;

static UTIL_TIMER_Object_t AlwaysOnSwitch_Timer;
static UTIL_TIMER_Time_t AlwaysOnSwitchOnTime = 3000; //3sec
static UTIL_TIMER_Time_t AlwaysOnSwitchOffTime = 6000; //6sec

static UTIL_TIMER_Object_t wait_Timer;
static volatile bool waiting = false;

static UTIL_TIMER_Object_t measurement_Timer;
static volatile bool startMeasure = true;

static uint8_t dataBuffer[100];
static uint8_t sensorModuleId = 0;
static bool sensorModuleEnabled = false;
static bool loraTransmitReady = false;
static LmHandlerErrorStatus_t loraTransmitStatus;

static const void init_vAlwaysOn(void);

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
    case 0: //init Powerup

      init_board_io(); //init IO
      initLedTimer(); //init LED timer
      init_vAlwaysOn();

      executeAlwaysOn();

      mainTask_state++;
      break;

    case 1: //init Sleep

      if( enableListenUart )
      {
        uartListen(); //activate the config uart to process command, temporary consturction //todo change only listen when USB is attachted.
      }

      //check if at least one sensor module is enabled
      for( int i=0; i < MAX_SENSOR_MODULE; i++)
      {
        if( getSensorStatus(i + 1) == true )
        {
          sensorModuleEnabled = true; //a module found, copy
        }
      }

      if( sensorModuleEnabled )
      {
        mainTask_state++; //at least one active sensor module slot

        getFramSetting(FRAM_SETTING_MODEMID, (void*)&sensorModuleId, true); //read out FRAM setting module ID

        if( sensorModuleId < 0 || sensorModuleId >= MAX_SENSOR_MODULE )
        {
          sensorModuleId = 0; //force to first.
        }

      }
      else
      {
        APP_LOG(TS_OFF, VLEVEL_H, "No sensor module slot enabled\r\n" ); //print no sensor slot enabled
        mainTask_state = 99; //no sensor slot is active
      }

      break;

    case 2: //enable sensor supply

      if( startMeasure == true )
      {
        slotPower(sensorModuleId, true); //enable slot sensorModuleId (0-5)
        setWait(1000); //set wait time 1000ms
        mainTask_state++; //next state
      }

      else
      {
        //keep waiting.
      }

      break;

    case 3: //start measure

      if( waiting == false ) //check wait time is expired
      {
        memset(dataBuffer, 0x00, sizeof(dataBuffer));
        sensorFirmwareVersion(sensorModuleId, dataBuffer, sizeof(dataBuffer));

        APP_LOG(TS_OFF, VLEVEL_H, "Sensor module firmware: %d, %s\r\n", sensorModuleId, dataBuffer ); //print VERSION

        uint8_t result;
        uint8_t sensorProtocol = 0;
        result = sensorProtocolVersion(sensorModuleId, &sensorProtocol);
        APP_LOG(TS_OFF, VLEVEL_H, "Sensor module protocol version: %d, %d\r\n", sensorModuleId, result == SENSOR_OK ? sensorProtocol : -1); //print protocol version

        uint16_t sensorType = 0;
        result = sensorReadType(sensorModuleId, &sensorType);
        APP_LOG(TS_OFF, VLEVEL_H, "Sensor module type: %d, %d\r\n", sensorModuleId, sensorType ); //print sensor type

        uint16_t sensorSetupTime = 0;
        result = sensorReadSetupTime(sensorModuleId, &sensorSetupTime);
        APP_LOG(TS_OFF, VLEVEL_H, "Sensor module setup time: %d, %u\r\n", sensorModuleId, sensorSetupTime ); //print sensor setup time

        sensorStartMeasurement(sensorModuleId);

        setWait(100);  //set wait time 1000ms

        mainTask_state++; //next state
      }

      break;

    case 4:

      if( waiting == false ) //check wait time is expired
      {
        MeasurementStatus newStatus = sensorMeasurementStatus(sensorModuleId);
        APP_LOG(TS_OFF, VLEVEL_H, "Sensor measure status: %d, %d\r\n", sensorModuleId, newStatus ); //print sensor type

        if( newStatus != MEASUREMENT_ACTIVE )
        {
          mainTask_state++; //next state
        }
        else
        {
          setWait(100);  //set wait time 100ms
        }
      }

      break;

    case 5: //read measurement

      {
        SensorError newstatus = sensorReadMeasurement(sensorModuleId, dataBuffer, sizeof(dataBuffer));
        APP_LOG(TS_OFF, VLEVEL_H, "Sensor module data: %d, %d, 0x%02x", sensorModuleId, newstatus, dataBuffer[0] ); //print sensor type

        for(int i=0; i < dataBuffer[0]; i++)
        {
          APP_LOG(TS_OFF, VLEVEL_H, ", 0x%02x", dataBuffer[i+1] ); //print data
        }
        APP_LOG(TS_OFF, VLEVEL_H, "\r\n" ); //print end

        uint8_t UNUSED_VAR result;
        uint16_t sensorType = 0;
        result = sensorReadType(sensorModuleId, &sensorType);

        if( sensorType == MFM_PREASURE_RS485 || sensorType == MFM_PREASURE_ONEWIRE)
        {
          uint8_t unitPress[] = "bar";
          uint8_t unitTemp[] = " C";
          unitTemp[0] = 176; //overwrite degree sign to ascii 176

          structDataPressureSensor * pSensorData = (structDataPressureSensor*)&dataBuffer[0];
          APP_LOG(TS_OFF, VLEVEL_H, "Sensor pressure data: %d.%02d %s, %d.%02d %s , %d.%02d %s, %d.%02d %s\r\n,",
              (int)pSensorData->pressure1, getDecimal(pSensorData->pressure1, 2), unitPress,
              (int)pSensorData->temperature1, getDecimal(pSensorData->temperature1, 2), unitTemp,
              (int)pSensorData->pressure2, getDecimal(pSensorData->pressure2, 2), unitPress,
              (int)pSensorData->temperature2, getDecimal(pSensorData->temperature2, 2), unitTemp


              ); //print sensor data
        }


        uint8_t sensorProtocol = 0;
        result = sensorProtocolVersion(sensorModuleId, &sensorProtocol);

        writeNewLog(sensorModuleId, sensorType, sensorProtocol, &dataBuffer[1], dataBuffer[0]); //write log data to dataflash.

        triggerSendTxData(); //trigger Lora transmit

        slotPower(sensorModuleId, false); //disable slot sensorModuleId (0-5)

        mainTask_state++; //next state
      }

      break;

    case 6:

      {
        int escape = MAX_SENSOR_MODULE;
        do
        {
          sensorModuleId++; //increment sensor ID
          sensorModuleId %= MAX_SENSOR_MODULE; //limit from 0 to 5.

        }while (getSensorStatus(sensorModuleId + 1) == false && escape--);


        setFramSetting(FRAM_SETTING_MODEMID, (void*)&sensorModuleId, true); //save last sensor Moudle ID


        mainTask_state++;
      }

      break;

    case 7:

      //wait on transmit ready flag
      if( loraTransmitReady == true )
      {
        UTIL_TIMER_Time_t newLoraInterval = getLoraInterval() * TM_SECONDS_IN_1MINUTE * 1000;
        UTIL_TIMER_Time_t forcedInterval = 0;

        loraTransmitReady = false; //reset

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

        mainTask_state=2; //go back to state waiting for new measure.
        pause_mainTask();

      }

      break;

    case 99:

      stop_mainTask(true);

      break;

    default:

      break;
  }

  update_board_io(); //periodically read

  if( getAlwaysOn_changed(true)) //check vAlwaysOn setting is changed
  {
    executeAlwaysOn(); //execute vAlwaysOn enable/disable
  }

  //check boolean mainTaskActive, then set short period for triggering, if not set long period for triggering.
  if( mainTaskActive )
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
 * @fn const void init_mainTask(void)
 * @brief function to initialize the mainTask
 *
 */
const void init_mainTask(void)
{
  mainTask_state = 0; //reset state for powerup
  mainTaskActive = true; //start the main task
  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_Main), UTIL_SEQ_RFU, mainTask); //register the task at the scheduler

  UTIL_TIMER_Create(&MainTimer, MainPeriodNormal, UTIL_TIMER_ONESHOT, trigger_mainTask, NULL); //create timer
  UTIL_TIMER_Start(&MainTimer); //start timer

  UTIL_TIMER_Create(&wait_Timer, 0, UTIL_TIMER_ONESHOT, trigger_wait, NULL); //create timer

  UTIL_TIMER_Create(&measurement_Timer, 0, UTIL_TIMER_ONESHOT, trigger_measure, NULL); //create timer


}


/**
 * @fn const void stop_mainTask(void)
 * @brief function to stop the mainTask
 *
 */
const void stop_mainTask(bool resume)
{
  mainTask_state = 1; //reset state for STOP2 mode
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
 * @fn const void delayedSwitchOff_IO_vAlwaysOn(void)
 * @brief function to delayed switch off the vAlwaysOn set pin
 *
 */
static const void delayedSwitchOff_IO_vAlwaysOn(void *context)
{
  writeOutput_board_io(EXT_IOVALWAYS_EN, GPIO_PIN_RESET);
}

/**
 * @fn const void init_vAlwaysOn(void)
 * @brief function to create timer at power up
 *
 */
static const void init_vAlwaysOn(void)
{
  UTIL_TIMER_Create(&AlwaysOnSwitch_Timer, AlwaysOnSwitchOnTime, UTIL_TIMER_ONESHOT, delayedSwitchOff_IO_vAlwaysOn, NULL); //create timer
}

/**
 * @fn const void enable_vAlwaysOn(void)
 * @brief function to start enable vAlways on supply
 *
 */
const void enable_vAlwaysOn(void)
{
  writeOutput_board_io(EXT_IOVALWAYS_EN, GPIO_PIN_SET);
  UTIL_TIMER_StartWithPeriod(&AlwaysOnSwitch_Timer, AlwaysOnSwitchOnTime); //set new time and start timer
}

/**
 * @fn const void disable_vAlwaysOn(void)
 * @brief function to start disable vAlways on supply
 *
 */
const void disable_vAlwaysOn(void)
{
  writeOutput_board_io(EXT_IOVALWAYS_EN, GPIO_PIN_SET);
  UTIL_TIMER_StartWithPeriod(&AlwaysOnSwitch_Timer, AlwaysOnSwitchOffTime); //set new time and start timer
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
