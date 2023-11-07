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
#include "common/app_types.h"
#include "utilities_def.h"
#include "stm32_seq.h"
#include "stm32_timer.h"

#include "IO/board_io.h"
#include "IO/led.h"
#include "CommConfig.h"
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

      //get configuration always on
      if( getAlwaysOn() )
      {
        enable_vAlwaysOn(); //enable vAlwaysOn supply
      }
      else
      {
        disable_vAlwaysOn(); //disable vAlwaysOn supply
      }

      mainTask_state++;
      break;

    case 1: //init Sleep


      if( enableListenUart )
      {
        uartListen(); //activate the config uart to process command, temporary consturction //todo change only listen when USB is attachted.
      }

      mainTask_state++;
      break;

    case 2:
      stop_mainTask(true);

      break;

    default:

      break;
  }

  update_board_io(); //periodically read


  //check boolean mainTaskActive, then set short period for triggering, if not set long period for triggering.
  if( mainTaskActive )
  {
    setNextPeriod(MainPeriodNormal);
  }
  else
  {
    setNextPeriod(MainPeriodSleep);
  }

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
