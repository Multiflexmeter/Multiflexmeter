/**
  ******************************************************************************
  * @addtogroup     : App
  * @{
  * @file           : productiontestTask.c
  * @brief          : production test task functions
  * @author         : P.Kwekkeboom
  * @date           : Dec 29, 2023
  * @}
  ******************************************************************************
  */

#include "main.h"
#include "sys_app.h"
#include "utilities.h"
#include "utilities_def.h"
#include "stm32_seq.h"
#include "stm32_timer.h"
#include "stm32_systime.h"
#include "timer_if.h"

#include "common/common.h"
#include "common/app_types.h"

#include "CommConfig.h"
#include "CommConfig_usr.h"

#include "productiontestTask.h"

static int productiontestTask_state;
static UTIL_TIMER_Object_t MainTimer;
static UTIL_TIMER_Time_t MainPeriodNormal = 10;

static UTIL_TIMER_Object_t taskTimeoutTimer;
static UTIL_TIMER_Time_t taskTimeoutTime = 120000UL; //120sec


/**
 * @fn const void setNextMainInterval(UTIL_TIMER_Time_t)
 * @brief
 *
 * @param next : interval in ms
 */
const void setNextMainInterval( UTIL_TIMER_Time_t next )
{
  UTIL_TIMER_SetPeriod(&MainTimer, next);
}


/**
 * @fn const void productiontestTask(void)
 * @brief
 *
 */
const void productiontestTask(void)
{
  switch(productiontestTask_state)
  {
    case 0:


      break;

    default:

      break;
  }
  uartKeepListen( 1 ); //always keep uart active in productiontestTask
  setNextMainInterval(MainPeriodNormal);
}


/**
 * @fn const void trigger_Task(void *contex)
 * @brief function to trigger the task to schedule.
 *
 */
static const void trigger_Task(void *contex)
{
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_Main), CFG_SEQ_Prio_0); //trigger task for the scheduler
}

/**
 * @fn const void trigger_timeout(void *contex)
 * @brief function to trigger the task to schedule.
 *
 */
static const void trigger_timeout(void *contex)
{
  saveStatusTestmode( false ); //disable production test task
  startDelayedReset(); //restart
}

/**
 * @fn const void init_productiontestTask(void)
 * @brief
 *
 */
const void init_productiontestTask(void)
{
  UTIL_TIMER_Create(&MainTimer, MainPeriodNormal, UTIL_TIMER_ONESHOT, trigger_Task, NULL); //create timer
  UTIL_TIMER_Start(&MainTimer); //start timer

  UTIL_TIMER_Create(&taskTimeoutTimer, taskTimeoutTime, UTIL_TIMER_ONESHOT, trigger_timeout, NULL); //create timer
  UTIL_TIMER_Start(&taskTimeoutTimer); //start timer

  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_Main), UTIL_SEQ_RFU, productiontestTask); //register the task at the scheduler
}
