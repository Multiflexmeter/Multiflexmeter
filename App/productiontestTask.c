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
#include "common/softwareVersion.h"

#include "CommConfig.h"
#include "CommConfig_usr.h"

#include "BatMon_BQ35100/BatMon_functions.h"
#include "dataflash/dataflash_functions.h"
#include "FRAM/FRAM_functions.h"
#include "IO/board_io.h"
#include "IO/board_io_test.h"

#include "productiontestTask.h"

static int productiontestTask_state;
static UTIL_TIMER_Object_t MainTimer;
static UTIL_TIMER_Time_t MainPeriodNormal = 10;

static UTIL_TIMER_Object_t taskTimeoutTimer;
static UTIL_TIMER_Time_t taskTimeoutTime = 120000UL; //120sec

enum{
  TEST_START = 0,
  TEST_IO_EXPANDER_INT_SYS,
  TEST_VSYS_OFF,
  TEST_VSYS_ON,
  TEST_IO_EXPANDER_INT_SENSOR,
  TEST_DATAFLASH,
  TEST_FRAM,
  TEST_GAUGE = 100,
};

/**
 * @fn const void setNextMainInterval(UTIL_TIMER_Time_t)
 * @brief
 *
 * @param next : interval in ms
 */
const void setNextMainInterval( UTIL_TIMER_Time_t next )
{
  UTIL_TIMER_Stop(&MainTimer);
  UTIL_TIMER_SetPeriod(&MainTimer, next);
  UTIL_TIMER_Start(&MainTimer);
}

/**
 * @fn void printOkay(void)
 * @brief helper function to print Okay for each test
 *
 */
void printOkay(void)
{
  APP_LOG(TS_OFF, VLEVEL_L, "Okay\r\n");
}

/**
 * @fn void printFailed(void)
 * @brief helper function to print Failed for each test
 *
 */
void printFailed(void)
{
  APP_LOG(TS_OFF, VLEVEL_L, "Failed\r\n");
}

/**
 * @fn void printResultTest(bool)
 * @brief helper function to print result
 *
 * @param result : true = "Okay", false = "Faield"
 */
void printResultTest(bool result)
{
  if( result == true)
  {
    printOkay();
  }
  else
  {
    printFailed();
  }
}

/**
 * @fn const void productiontestTask(void)
 * @brief
 *
 */
const void productiontestTask(void)
{
  static int8_t resultVsysOff;
  static int8_t resultVsysOn;
  switch(productiontestTask_state)
  {
    case TEST_START:

      APP_LOG(TS_OFF, VLEVEL_L, "Productiontest Task\r\n");
      APP_LOG(TS_OFF, VLEVEL_L, "Firmware: %s\r\n", getSoftwareVersionMFM());


      if( getStatusRegister().testmodeBatteryGauge )
      {
        batmon_enable();
        productiontestTask_state = TEST_GAUGE;
      }

      else
      {
        productiontestTask_state++;
      }


      break;

    case TEST_IO_EXPANDER_INT_SYS:

      APP_LOG(TS_OFF, VLEVEL_L, " - Testing I/O expander U14: ");
      {
        int8_t result;
        result = readInput_board_io(EXT_IOUSB_CONNECTED); //indirect by reading an I/O pin

        printResultTest( result > 0 );
      }

      productiontestTask_state++;

      break;

    case TEST_VSYS_OFF:

      APP_LOG(TS_OFF, VLEVEL_L, " - Testing Switch VSYS (U20): ");

      resultVsysOff = checkSpiPullupsVsystem();
      writeOutput_board_io(EXT_IOVSYS_EN, GPIO_PIN_SET); //enable vsys

      productiontestTask_state++;

      break;

    case TEST_VSYS_ON:

      resultVsysOn = checkSpiPullupsVsystem();
      writeOutput_board_io(EXT_IOVSYS_EN, GPIO_PIN_RESET); //disable vsys

      printResultTest( resultVsysOn == 1 && resultVsysOff == 0 );

      productiontestTask_state++;

      break;

    case TEST_IO_EXPANDER_INT_SENSOR:

      APP_LOG(TS_OFF, VLEVEL_L, " - Testing I/O expander U15: ");
      {
        int8_t result;
        writeOutput_board_io(EXT_IOVSYS_EN, GPIO_PIN_SET); //enable vsys
        init_board_io_device(IO_EXPANDER_BUS_INT); //init I/O expander
        result = readInput_board_io(EXT_IOSLOT1_GPIO0); //indirect by reading an I/O pin
        deinit_IO_Expander(IO_EXPANDER_BUS_INT); //deinit I/O expander
        writeOutput_board_io(EXT_IOVSYS_EN, GPIO_PIN_RESET); //disable vsys

        printResultTest( result >=0 );
      }

      productiontestTask_state++;

      break;

    case TEST_DATAFLASH: //depending on VSYS and IO_EXPANDER_INT_SYS

      APP_LOG(TS_OFF, VLEVEL_L, " - Testing NOR flash: ");
      {
        uint32_t value = 0;
        int8_t result;
        result = testDataflash(2, &value);

        printResultTest( result >=0 );
      }

      productiontestTask_state++;

      break;

    case TEST_FRAM: //depending on VSYS and IO_EXPANDER_INT_SYS

      APP_LOG(TS_OFF, VLEVEL_L, " - Testing FRAM: ");
      {
        uint8_t statusRegister = 0;
        int8_t result;
        result = testFram(&statusRegister);

        printResultTest( result >=0 );
      }

      productiontestTask_state++;

      break;


    case TEST_GAUGE:

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

  initBatMon(); //initialize I2C peripheral for battery monitor
}
