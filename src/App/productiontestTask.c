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
#include <stdarg.h>
#include <stdio.h>
#include "sys_app.h"
#include "utilities.h"
#include "utilities_def.h"
#include "stm32_seq.h"
#include "stm32_timer.h"
#include "stm32_systime.h"
#include "timer_if.h"
#include "i2c.h"
#include "usart.h"

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
#include "IO/board_io_functions.h"
#include "IO/led.h"
#include "app_fatfs.h"
#include "I2CMaster/SensorFunctions.h"
#include "RTC_AM1805/RTC_functions.h"
#include "productiontest/productionIO.h"

#include "pingpong/subghz_phy_app.h"
#include "lora_app.h"

#include "productiontestTask.h"

#define MAX_RADIO_TX_MESSAGES ( 10 )
#define MIN_RADIO_RX_MESSAGES ( 5 )
#define MIN_RADIO_RSSI        ( -65 )

static int productiontestTask_state;
static int batmonConfigTask_state = 0;
static UTIL_TIMER_Object_t MainTimer;
static UTIL_TIMER_Time_t MainPeriodNormal = 10;

static UTIL_TIMER_Object_t taskTimeoutTimer;
static UTIL_TIMER_Time_t taskTimeoutTime = 600000UL; //600sec

#define LOG_BUFFER_SIZE 255
static char buf[LOG_BUFFER_SIZE];

static uint32_t start = 0;
static bool lightSensorStatus = false;

enum{
  TEST_FAILED = -2,
  TEST_INIT = 0,
  TEST_START = 1,
  TEST_IO_EXPANDER_INT_SYS,
  TEST_VSYS_OFF,
  TEST_VSYS_ON,
  TEST_IO_EXPANDER_INT_SENSOR,
  TEST_DATAFLASH,
  TEST_FRAM,
  TEST_SDCARD,
  TEST_ADC,
  TEST_BATMON,
  TEST_VSENSOR_1,
  TEST_VSENSOR_2,
  TEST_INTERRUPTS,
  TEST_SLOTGPIO,
  TEST_VCC_SWITCH,
  TEST_CONFIGRADIO,
  TEST_LIGHTSENSOR,
  TEST_LIGHTSENSOR_WAIT,
  TEST_RADIO,
  TEST_RTC,
  TEST_RTC_WAKEUP,
  TEST_PASSED,
  TEST_GAUGE = 100,
  TEST_STOP,
};

enum{
  BATMON_INIT = 0,
  BATMON_STOP,
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
 * @brief Print text to the USB C console.
 * @param text String to print.
 */
void LOG_Print(const char* text, ...)
{
    if (text == NULL)
    {
        return;
    }

    va_list argptr;
    va_start(argptr, text);
    vsnprintf(buf, LOG_BUFFER_SIZE, text, argptr);
    va_end(argptr);

    HAL_UART_Transmit(&config_uart, (uint8_t*)buf, strlen(buf), 1000);
}

/**
 * @fn void printOkay(void)
 * @brief helper function to print Okay for each test
 *
 */
void printPassed(void)
{
  LOG_Print("Passed\r\n");
}

/**
 * @fn void printFailed(void)
 * @brief helper function to print Failed for each test
 *
 */
void printFailed(void)
{
  LOG_Print("Failed\r\n");
}

/**
 * @fn void printResultTest(bool)
 * @brief helper function to print result
 *
 * @param result : true = "Passed", false = "Faield"
 */
void printResultTest(bool result)
{
  if( result == true)
  {
    printPassed();
  }
  else
  {
    printFailed();
  }
}

/**
 * @brief Check the other interrupts
 * @param interrupts interrupt that is currently tested
 * @return the status of the other interrupts
 */
int8_t checkOtherInts(ENUM_IO_ITEM interrupts)
{
  switch (interrupts) {
    case EXT_IOINT1:
      return(readInput_board_io(EXT_IOINT2));

    case EXT_IOINT2:
      return(readInput_board_io(EXT_IOINT1) && readInput_board_io(EXT_IOINT3));

    case EXT_IOINT3:
      return(readInput_board_io(EXT_IOINT2) && readInput_board_io(EXT_IOINT4));

    case EXT_IOINT4:
      return(readInput_board_io(EXT_IOINT3) && readInput_board_io(EXT_IOINT5));

    case EXT_IOINT5:
      return(readInput_board_io(EXT_IOINT4) && readInput_board_io(EXT_IOINT6));

    case EXT_IOINT6:
      return(readInput_board_io(EXT_IOINT5));

    default:
      return -1;
  }
}

/**
 * @brief Check the other slot gpio
 * @param SlotGPIO Slot GPIO that is currently tested
 * @return the status of the other slot gpio
 */
int8_t checkOtherSlotGPIO(ENUM_IO_ITEM SlotGPIO)
{
  switch (SlotGPIO) {
    case EXT_IOSLOT1_GPIO0:
      return(readInput_board_io(EXT_IOSLOT1_GPIO1));

    case EXT_IOSLOT1_GPIO1:
      return(readInput_board_io(EXT_IOSLOT1_GPIO0) && readInput_board_io(EXT_IOSLOT1_GPIO2));

    case EXT_IOSLOT1_GPIO2:
      return(readInput_board_io(EXT_IOSLOT1_GPIO1));

    case EXT_IOSLOT2_GPIO0:
      return(readInput_board_io(EXT_IOSLOT2_GPIO1));

    case EXT_IOSLOT2_GPIO1:
      return(readInput_board_io(EXT_IOSLOT2_GPIO0) && readInput_board_io(EXT_IOSLOT2_GPIO2));

    case EXT_IOSLOT2_GPIO2:
      return(readInput_board_io(EXT_IOSLOT2_GPIO1));

    default:
      return -1;
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
    case TEST_INIT:

      /* Pause the normal UartConfig task */
      UTIL_SEQ_PauseTask(1 << CFG_SEQ_Task_UartConfig);
      disableSleep();
      productionIO_init();

      /* Sync RTC with interal RTC to check later */
      syncRTC_withSysTime();
      initLedTimer();
      setLed(LED_1, LED_TOGGLE, LED_ON, 100, 0, 0);
      setLed(LED_2, LED_TOGGLE, LED_ON, 100, 0, 0);

      /* Restore previous state if needed */
      productiontestTask_state = readBackupRegister(BACKUP_REGISTER_PRODUCTIONTEST_STATE);
      writeBackupRegister(BACKUP_REGISTER_PRODUCTIONTEST_STATE, TEST_START);

      break;

    case TEST_START:

      HAL_Delay(5000);
      LOG_Print("\r\n========================================================================================\r\n");
      LOG_Print("\t\t\t\tProductiontest Task\r\n");
      LOG_Print("\t\t\t\tP22296-10-SW-%s", getSoftwareVersionMFM());
      LOG_Print("\r\n========================================================================================\r\n");

      productiontestTask_state++;

      break;

    case TEST_IO_EXPANDER_INT_SYS:

      LOG_Print(" - Testing I/O expander U14: ");
      {
        int8_t result;
        result = readInput_board_io(EXT_IOUSB_CONNECTED); //indirect by reading an I/O pin

        printResultTest( result > 0 );
      }

      productiontestTask_state++;

      break;

    case TEST_VSYS_OFF:

      LOG_Print(" - Testing Switch VSYS (U20): ");

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

      LOG_Print(" - Testing I/O expander U15: ");
      {
        int8_t result;
        writeOutput_board_io(EXT_IOVSYS_EN, GPIO_PIN_SET); //enable vsys
        MX_I2C2_Init(); //initialize I2C for BUS, otherwise it could not work because power was down.
        init_board_io_device(IO_EXPANDER_BUS_INT); //init I/O expander
        result = readInput_board_io(EXT_IOSLOT1_GPIO0); //indirect by reading an I/O pin
        deinit_IO_Expander(IO_EXPANDER_BUS_INT); //deinit I/O expander
        writeOutput_board_io(EXT_IOVSYS_EN, GPIO_PIN_RESET); //disable vsys

        printResultTest( result >=0 );
      }

      productiontestTask_state++;

      break;

    case TEST_DATAFLASH: //depending on VSYS and IO_EXPANDER_INT_SYS

      LOG_Print(" - Testing NOR flash: ");
      {
        uint32_t value = 0;
        int8_t result;
        result = testDataflash(2, &value);

        printResultTest( result >=0 );
      }

      productiontestTask_state++;

      break;

    case TEST_FRAM: //depending on VSYS and IO_EXPANDER_INT_SYS

      LOG_Print(" - Testing FRAM: ");
      {
        uint8_t statusRegister = 0;
        int8_t result;
        result = testFram(&statusRegister);

        printResultTest( result >=0 );
      }

      productiontestTask_state++;

      break;

    case TEST_SDCARD:

      LOG_Print(" - Testing SD Card: ");
      uint32_t cardCapacity;
      uint32_t cardFree;
      if(SD_TEST(&cardCapacity, &cardFree) != 0)
      {
        printResultTest(false);
        productiontestTask_state = TEST_FAILED;
      }
      else
      {
        printResultTest(true);
        productiontestTask_state++;
      }

      break;

    case TEST_ADC:

      /* Test the ADC VBUS on 10V */
      LOG_Print(" - Testing VBUS: ");
      productionIOTurnOff(VBUS);
      HAL_Delay(10);
      printResultTest((getVbusSupply() > 11500) && (getVbusSupply() < 12500));
      if(!((getVbusSupply() > 11500) && (getVbusSupply() < 12500)))
      {
        LOG_Print("VBUS = %dmV\r\n", getVbusSupply());
        productiontestTask_state = TEST_FAILED;
        break;
      }
      productionIOTurnOn(VBUS);

      /* Test the ADC VCC on 3.3V */
      LOG_Print(" - Testing VCC: ");
      printResultTest((getVccSupply() > 3200) && (getVccSupply() < 3400));
      if(!((getVccSupply() > 3200) && (getVccSupply() < 3400)))
      {
        LOG_Print("VCC = %dmV\r\n", getVccSupply());
        productiontestTask_state = TEST_FAILED;
        break;
      }

      productiontestTask_state++;

      break;

    case TEST_BATMON:

      LOG_Print(" - Battery Monitor Test:");
      batmon_enable();
      HAL_Delay(1000);

      if (batmon_isInitComplet())
      {
        batmon_measure();
        structBatMonData measurement = batmon_getMeasure();
        if(measurement.voltage > 3200 || measurement.voltage < 3400)
        {
          printResultTest(true);
          productiontestTask_state++;
        }
        else
        {
          printResultTest(false);
          productiontestTask_state = TEST_FAILED;
        }
      }
      else
      {
        printResultTest(false);
        productiontestTask_state = TEST_FAILED;
      }

      batmon_disable();

      break;

    case TEST_VSENSOR_1:
      // Initialize the I/O Expander
      enableVsys(); //enable supply for I/O expander
      MX_I2C2_Init(); //initialize I2C for BUS, otherwise it could not work because power was down.
      init_board_io_device(IO_EXPANDER_BUS_INT);

      LOG_Print(" - Sensorslot 1 Test:");
      slotPower(sensorSlot1, true); //enable slot 1
      HAL_Delay(200);
      uint16_t sensorType = 0; //reset first
      uint8_t result = sensorReadType(sensorSlot1, &sensorType);
      slotPower(sensorSlot1, false); //disable slot 1
      if(result || !sensorType)
      {
        printResultTest(false);
        productiontestTask_state = TEST_FAILED;
      }
      else
      {
        printResultTest(true);
        productiontestTask_state++;
      }

      break;

    case TEST_VSENSOR_2:
      LOG_Print(" - Sensorslot 2 Test:");
      slotPower(sensorSlot2, true); //enable slot 1
      HAL_Delay(200);
      sensorType = 0; //reset first
      result = sensorReadType(sensorSlot2, &sensorType);
      slotPower(sensorSlot2, false); //disable slot 1
      if(result || !sensorType)
      {
        printResultTest(false);
        productiontestTask_state = TEST_FAILED;
      }
      else
      {
        printResultTest(true);
        productiontestTask_state++;
      }
      break;

    case TEST_INTERRUPTS:

      LOG_Print(" - Interrupts Test: ");

      // Test INT1 line
      slotPower(sensorSlot1, true); //enable slot 1
      HAL_Delay(100);
      sensorWriteIO(sensorSlot2, 0);
      sensorWriteIO(sensorSlot1, INTERRUPT);
      HAL_Delay(10);
      if((readInput_board_io(EXT_IOINT1) == 0) || (checkOtherInts(EXT_IOINT1) == 1))
      {
        LOG_Print("INT1 Failed\r\n");
        productiontestTask_state = TEST_FAILED;
        break;
      }
      sensorWriteIO(sensorSlot1, 0);
      HAL_Delay(10);
      if((readInput_board_io(EXT_IOINT1) == 1))
      {
        LOG_Print("INT1 Failed\r\n");
        productiontestTask_state = TEST_FAILED;
        break;
      }
      slotPower(sensorSlot1, false); //disable slot 1

      // Test INT2 line
      slotPower(sensorSlot2, true); //enable slot 1
      HAL_Delay(100);
      sensorWriteIO(sensorSlot2, INTERRUPT);
      HAL_Delay(10);
      if((readInput_board_io(EXT_IOINT2) == 0) || (checkOtherInts(EXT_IOINT2) == 1))
      {
        LOG_Print("INT2 Failed\r\n");
        productiontestTask_state = TEST_FAILED;
        break;
      }

      sensorWriteIO(sensorSlot2, 0);
      HAL_Delay(10);
      if(readInput_board_io(EXT_IOINT2))
      {
        LOG_Print("INT2 Failed\r\n");
        productiontestTask_state = TEST_FAILED;
        break;
      }
      slotPower(sensorSlot2, false); //disable slot 1

      for (uint8_t i = 0; i < 4; ++i)
      {
        // Check the on state of the interrupt
        productionIOTurnOn(INT3+i);
        HAL_Delay(1);
        if((readInput_board_io(EXT_IOINT3+i) == 0) || (checkOtherInts(EXT_IOINT3+i) == 1))
        {
          LOG_Print("INT%d Failed\r\n", 3+i);
          productiontestTask_state = TEST_FAILED;
          setNextMainInterval(MainPeriodNormal);
          return;
        }

        // Check the off state of the interrupt
        productionIOTurnOff(INT3+i);
        if(readInput_board_io(EXT_IOINT3+i))
        {
          LOG_Print("INT%d Failed\r\n", 3+i);
          productiontestTask_state = TEST_FAILED;
          setNextMainInterval(MainPeriodNormal);
          return;
        }
      }
      LOG_Print("Passed\r\n");
      productiontestTask_state++;

      break;

    case TEST_SLOTGPIO:

      // Test slot 1 GPIO's
      LOG_Print(" - SLOT GPIO Test: ");
      slotPower(sensorSlot1, true); //enable slot 1
      HAL_Delay(250);

      /* Set direction  to outputs */
      sensorDirectionIO(sensorSlot1, SLOT_GPIO_0 | SLOT_GPIO_1 | SLOT_GPIO_2);

      for(uint8_t i=0;i<3;i++)
      {
        // Check the on state of the gpio
        sensorWriteIO(sensorSlot1, SLOT_GPIO_0<<i);
        HAL_Delay(10);
        if((readInput_board_io(EXT_IOSLOT1_GPIO0+i) == 0) || (checkOtherSlotGPIO(EXT_IOSLOT1_GPIO0+i) == 1))
        {
          LOG_Print("SLOT1 GPIO%d Failed\r\n", i);
          productiontestTask_state = TEST_FAILED;
          slotPower(sensorSlot1, false);
          setNextMainInterval(MainPeriodNormal);
          return;
        }

        // Check the off state of the gpio
        sensorWriteIO(sensorSlot1, 0);
        HAL_Delay(10);
        if(readInput_board_io(EXT_IOSLOT1_GPIO0+i))
        {
          LOG_Print("SLOT1 GPIO%d Failed\r\n", i);
          productiontestTask_state = TEST_FAILED;
          slotPower(sensorSlot1, false);
          setNextMainInterval(MainPeriodNormal);
          return;
        }
      }
      slotPower(sensorSlot1, false);

      // Test slot 2 GPIO's
      slotPower(sensorSlot2, true); //enable slot 2
      HAL_Delay(250);

      /* Set direction  to outputs */
      sensorDirectionIO(sensorSlot2, SLOT_GPIO_0 | SLOT_GPIO_1 | SLOT_GPIO_2);

      for(uint8_t i=0;i<3;i++)
      {
        sensorWriteIO(sensorSlot2, SLOT_GPIO_0<<i);
        HAL_Delay(10);
        if(!readInput_board_io(EXT_IOSLOT2_GPIO0+i))
        {
          LOG_Print("SLOT2 GPIO%d Failed\r\n", i);
          productiontestTask_state = TEST_FAILED;
          slotPower(sensorSlot2, false);
          setNextMainInterval(MainPeriodNormal);
          return;
        }

        // Check the off state of the interrupt
        sensorWriteIO(sensorSlot2, 0);
        HAL_Delay(10);
        if(readInput_board_io(EXT_IOSLOT2_GPIO0+i))
        {
          LOG_Print("SLOT2 GPIO%d Failed\r\n", i);
          productiontestTask_state = TEST_FAILED;
          slotPower(sensorSlot2, false);
          setNextMainInterval(MainPeriodNormal);
          return;
        }
      }
      slotPower(sensorSlot2, false);
      LOG_Print("Passed\r\n");
      productiontestTask_state++;

      break;

    case TEST_VCC_SWITCH:

      LOG_Print(" - Starting VCC Switch Test: \r\n");
      LOG_Print("   If no messages are received after 10 seconds then the test has failed\r\n");
      LOG_Print("   To restart the test perform a power cycle\r\n");
      HAL_Delay(1000);
      productionIOTurnOff(VUSB);
      HAL_Delay(2000);
      productionIOTurnOn(VUSB);
      HAL_Delay(3500);
      LOG_Print(" - VCC Switch Test: Passed\r\n");
      productiontestTask_state++;
      break;

    case TEST_CONFIGRADIO:

      LOG_Print(" - Starting Lora Test: \r\n");
      LoRaWAN_deInit(); //de-initialize LoRaWan functionality

      LOG_Print("   Init Radio for PING-PONG\r\n");
      SubghzApp_Init(); //initialize PING/PONG

      productiontestTask_state++;

      break;

    case TEST_RADIO:

      //wait until MIN_MESSAGE are send
      if( getPingPongStatus().txCounter > MAX_RADIO_TX_MESSAGES || getPingPongStatus().rxCounter > MIN_RADIO_RX_MESSAGES)
      {
        SubghzApp_DeInit(); //disable radio transmit PING PONG

        bool resultValid = true;
        //validate result
        if( getPingPongStatus().rxTimeout == true )
        {
          resultValid = false;
          LOG_Print("   Radio failed, rx timeout\r\n");
        }

        if( getPingPongStatus().rxCounter < MIN_RADIO_RX_MESSAGES )
        {
          resultValid = false;
          LOG_Print("   Radio failed, too less messages received: %u rx, %u tx\r\n", getPingPongStatus().rxCounter, getPingPongStatus().txCounter);
        }

        if ( getPingPongStatus().averageRssi < MIN_RADIO_RSSI)
        {
          resultValid = false;
          LOG_Print("   Radio failed, RSSI not enough: average %d, latest %d, threshold %d\r\n", getPingPongStatus().averageRssi, getPingPongStatus().latestRssi, MIN_RADIO_RSSI);
        }

        if( resultValid == true )
        {
          LOG_Print("   Radio okay. RSSI: latest %d, average %d, Messages: %u rx, %u tx\r\n", getPingPongStatus().latestRssi, getPingPongStatus().averageRssi, getPingPongStatus().rxCounter, getPingPongStatus().txCounter - 1);
        }

        LOG_Print(" - Testing Lora: ");
        printResultTest(resultValid);

        if(resultValid)
          productiontestTask_state++;
        else
          productiontestTask_state = TEST_FAILED;
      }

      break;

    case TEST_LIGHTSENSOR:

      LOG_Print(" - Starting Light sensor Test:\r\n");

      // Check current state of the light sensor
      lightSensorStatus = getLigthSensorStatus();
      if(!lightSensorStatus) LOG_Print("   No light detected. Remove the cover from the light sensor.\r\n");
      else LOG_Print("   light detected. Place the cover over the light sensor.\r\n");

      // Wait until state has changed. If it takes longer than 30 seconds test failed.
      start = HAL_GetTick();
      productiontestTask_state++;
      break;

    case TEST_LIGHTSENSOR_WAIT:
      if(lightSensorStatus != getLigthSensorStatus())
      {
        LOG_Print(" - Light sensor Test: Passed\r\n");
        LOG_Print(" - Testing Lora: \r\n");
        productiontestTask_state++;
      }
      else
      {
        if(HAL_GetTick() - start >= 60000)
        {
          LOG_Print(" - Light sensor Test: Failed\r\n");
          productiontestTask_state = TEST_FAILED;
          setNextMainInterval(MainPeriodNormal);
          return;
        }

        //Disable lora task if enough messages are received
        if( getPingPongStatus().txCounter > MAX_RADIO_TX_MESSAGES || getPingPongStatus().rxCounter > MIN_RADIO_RX_MESSAGES)
        {
          SubghzApp_DeInit(); //disable radio transmit PING PONG
        }
        watchdogReload();
      }
      break;

    case TEST_RTC:

      /* Testing RTC clock time */
      LOG_Print(" - Starting RTC Time Test: \r\n");
      struct tm localTime;
      struct_dateTime RTCTime;

      SysTimeLocalTime(SysTimeGet().Seconds, &localTime);
      testRTC(2, &RTCTime);
      LOG_Print("   Local time: %d:%d:%d\r\n", localTime.tm_hour, localTime.tm_min, localTime.tm_sec);
      LOG_Print("   RTC time: %d:%d:%d\r\n", RTCTime.hour, RTCTime.minute, RTCTime.second);

      if((localTime.tm_sec - RTCTime.second > 4) || (localTime.tm_sec - RTCTime.second < -4))
      {
        productiontestTask_state = TEST_FAILED;
        LOG_Print(" - RTC Time Test: Failed\r\n");
        break;
      }
      LOG_Print(" - RTC Time Test: Passed\r\n");

      // Start sleep mode and test wake up
      LOG_Print("   System going to sleep. Press the red button to wake up the system.\r\n");
      LOG_Print(" - RTC Wakeup Test: ");
      writeBackupRegister(BACKUP_REGISTER_PRODUCTIONTEST_STATE, TEST_RTC_WAKEUP);
      HAL_Delay(250);

      productionIOTurnOff(VUSB);
      readStatusRegisterRtc();
      goIntoSleep(30, 1);
      while(1);
      break;

    case TEST_RTC_WAKEUP:
      HAL_Delay(3500);
      LOG_Print("Passed\r\n");
      LOG_Print(" - Do not forget to check the LEDs D2 and D3.\r\n");
      productiontestTask_state++;

      break;

    case TEST_GAUGE:

      break;

    case TEST_FAILED:

      LOG_Print("\r\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\r\n");
      LOG_Print("\t\t\t\tProduction test FAILED");
      LOG_Print("\r\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\r\n");
      productiontestTask_state = TEST_STOP;


      /* Re-enable command uart */
      UTIL_SEQ_ResumeTask(1 << CFG_SEQ_Task_UartConfig);
      uartListen();
      SubghzApp_DeInit(); //disable radio transmit PING PONG

      /* Disable Production test */
      saveStatusTestmode(0);

      break;

    case TEST_PASSED:

      LOG_Print("\r\n****************************************************************************************\r\n");
      LOG_Print("\t\t\t\tProduction test PASSED");
      LOG_Print("\r\n****************************************************************************************\r\n");
      productiontestTask_state = TEST_STOP;

      /* Re-enable command uart */
      UTIL_SEQ_ResumeTask(1 << CFG_SEQ_Task_UartConfig);
      uartListen();

      /* Disable Production test */
      saveStatusTestmode(0);

      break;

    case TEST_STOP:
      /* Wait 10 seconds before reseting the MFM */
      LOG_Print("MFM will rest in 10 seconds and exit the production test.\r\n\r\n");
      HAL_Delay(10000);
      startDelayedReset();
      HAL_Delay(10000);
      break;

    default:

      break;
  }
  watchdogReload();
  setNextMainInterval(MainPeriodNormal);
}


/**
 * @fn const void batmonConfigTask(void)
 * @brief Battery monitor configuration task
 */
const void batmonConfigTask(void)
{
  switch (batmonConfigTask_state)
  {
    case BATMON_INIT:

      // Disable i2c peripheral to not influence batmon configuration
      productionIO_init();
      batmon_enable();
      HAL_I2C_DeInit(&hi2c1);

      HAL_Delay(5000);
      LOG_Print("\r\n========================================================================================\r\n");
      LOG_Print("\t\t\t\tBatmon Config Task\r\n");
      LOG_Print("\t\t\t\tP22296-10-SW-%s", getSoftwareVersionMFM());
      LOG_Print("\r\n========================================================================================\r\n");
      LOG_Print("1. Open TI's BQStudio.\r\n");
      LOG_Print("2. Check that the device is recognized on the left hand side of the screen.\r\n");
      LOG_Print("3. Press the UNSEAL button in the commands panel.\r\n");
      LOG_Print("4. Navigate to the programming tab and select the right configuration specified in the prodoc!\r\n");
      LOG_Print("5. Click the program button and wait until this is done.\r\n");
      LOG_Print("6. Navigate to Data Memory->Configuration and check that register Operation Config A is 0x82.\r\n");
      LOG_Print("7. Press the NEW BATTERY button.\r\n");
      LOG_Print("8. Navigate to the Register tab and press refresh in the top right.\r\n");
      LOG_Print("9. Check that SOH is 100%% and Design capacity is 2600mAh.\r\n");
      LOG_Print("10. To end the test press the reset button or send Set+Reboot\r\n");
      LOG_Print("11. If you want to start the production test then send the command Set+Test=0,1\r\n");

      /* Disable Batmon config */
      saveStatusTestmode(0);

      batmonConfigTask_state++;
      break;

    case BATMON_STOP:
      break;

    default:
      break;
  }

  watchdogReload();
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
 * @brief initialize the production test task for use in production.
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

/**
 * @fn const void init_batmonConfigTask(void)
 * @brief initialize the batmon config task for use in production.
 */
const void init_batmonConfigTask(void)
{
  UTIL_TIMER_Create(&MainTimer, MainPeriodNormal, UTIL_TIMER_ONESHOT, trigger_Task, NULL); //create timer
  UTIL_TIMER_Start(&MainTimer); //start timer

  UTIL_TIMER_Create(&taskTimeoutTimer, taskTimeoutTime, UTIL_TIMER_ONESHOT, trigger_timeout, NULL); //create timer
  UTIL_TIMER_Start(&taskTimeoutTimer); //start timer

  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_Main), UTIL_SEQ_RFU, batmonConfigTask); //register the task at the scheduler

  initBatMon(); //initialize I2C peripheral for battery monitor
}
