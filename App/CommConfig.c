/**
  ******************************************************************************
  * @addtogroup     : App
  * @{
  * @file           : CommConfig.c
  * @brief          : Communication configuration functions
  * @author         : P.Kwekkeboom
  * @date           : Apr 24, 2023
  * @copyright      : 2023 Dekimo Goes
  * @}
  ******************************************************************************
  */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "sys_app.h"

#include "utilities_def.h"
#include "stm32_seq.h"
#include "stm32wlxx_hal.h"
#include "stm32wlxx_hal_uart.h"

#include "common/common.h"
#include "common/app_types.h"
#include "common/uart.h"
#include "CommConfig.h"

#include "secure-element.h"
#include "../Core/Inc/sys_app.h"
#include "../LoRaWAN/App/lora_app.h"

//#define NO_SLEEP

/*!
 * App key size in characters
 */
#define APP_KEY_CHARACTERS  32


#ifndef HEX8
#define HEX8(X)   X[0], X[1], X[2], X[3], X[4], X[5], X[6], X[7]
#endif

#ifndef HEX16
#define HEX16(X)  HEX8(X), X[8], X[9], X[10], X[11], X[12], X[13], X[14], X[15]
#endif

#define RECEIVE_TIMEOUT_COMMAND  (5 * config_uart.Init.BaudRate) //5 seconds

#define DUMP_ALL  UINT32_MAX

static DMA_BUFFER uint8_t bufferTxConfig[SIZE_TX_BUFFER_CONFIG];
static DMA_BUFFER uint8_t bufferRxConfig[SIZE_RX_BUFFER_CONFIG];

static UTIL_TIMER_Object_t uartConfigActive_Timer;
static UTIL_TIMER_Time_t uartConfigActiveTime_default = 30000; //30sec
static bool uartConfigKeepListen = false;

static bool dataDump;
static bool dataErase;
static bool dataTest;
static bool detectChangeRebootNeeded = false;
static int currentTest;
static int currentSubTest;
static char additionalArgumentsString[100];

static uint32_t numberOffDumpRecords;
static uint8_t loraBufferSize;

static const char cmdError[]="ERROR";
static const char cmdOkay[]="OK";
static const char cmdSet[]="Set+";
static const char cmdGet[]="Get+";
static const char cmdModuleInfo[]="ModuleInfo";
static const char cmdSensorInfo[]="SensorInfo";
static const char cmdJoinId[]="JoinID";
static const char cmdDeviceId[]="DeviceID";
static const char cmdAppKey[]="AppKey";
static const char cmdSensor[]="Sensor";
static const char cmdLoraInterval[]="LoraInterval";
static const char cmdMeasureTime[]="MeasureTime"; //todo remove
static const char cmdSamples[]="Samples";
static const char cmdDataDump[]="DataDump";
static const char cmdAlwaysOn[]="AlwaysOn";
static const char cmdErase[]="Erase";
static const char cmdTest[]="Test";
static const char cmdBat[]="Bat";
static const char cmdVbus[]="Vbus";
static const char cmdVcc[]="Vcc";
static const char cmdSave[]="Save";
static const char cmdReboot[]="Reboot";
static const char cmdInitSensor[]="InitSensor";


static const char defaultProtocol1[] = "0.0";
static const char defaultProtocol2[] = "0.0";
static const char defaultVersion[] = "0.0";

typedef struct{
    const char * command;
    int commandLength;
    void (* function)(int arguments, const char * format, ...);
    int numberOfArguments;
}struct_commands;

/**
 * @brief weak function getProtocol1(), needs to be override by real functions
 *
 * @return
 */
__weak const char * getProtocolVersionConfig(void)
{
  return defaultProtocol1;
}

/**
 * @brief weak function getProtocol2(), needs to be override by real functions
 *
 * @return
 */
__weak const char * getProtocolVersionSensor(void)
{
  return defaultProtocol2;
}

/**
 * @brief weak function getSoftwareVersionMFM(), needs to be override by real functions
 *
 * @return
 */
__weak const char * getSoftwareVersionMFM(void)
{
  return defaultVersion;
}

/**
 * @brief weak function getSoftwareSensorboard(), needs to be override by real functions
 *
 * @return
 */
__weak const char * getSoftwareSensorboard(int value)
{
  return defaultVersion;
}

/**
 * @brief weak function getProtocolSensorboard(), needs to be override by real functions
 *
 * @return
 */
__weak const uint8_t getProtocolSensorboard(int sensorModuleId)
{
  return 0;
}

/**
 * @brief weak function getJoinId(), can be override in application code.
 *
 * @return
 */
__weak const uint8_t * getJoinId(void)
{
  static uint8_t joinEui[SE_EUI_SIZE]={0};
  SecureElementGetJoinEui( joinEui );

  return joinEui;
}

/**
 * @brief weak function setJoinId(), can be override in application code.
 *
 * @return
 */
__weak const void setJoinId(const uint8_t * joinEui)
{
  SecureElementSetJoinEui( (uint8_t *)joinEui );
}


/**
 * @brief weak function getDeviceId(), can be override in application code.
 *
 * @return
 */
__weak const uint8_t * getDeviceId(void)
{
  static uint8_t devEui[SE_EUI_SIZE]={0};
  SecureElementGetDevEui( devEui );

  return devEui;
}

/**
 * @brief weak function setDeviceId(), can be override in application code.
 *
 * @return
 */
__weak const void setDeviceId(const uint8_t * joinEui)
{
  SecureElementSetDevEui( (uint8_t *)joinEui );
}

/**
 * @brief weak function getAppKey(), can be override in application code.
 *
 * @return
 */
__weak const uint8_t * getAppKey(void)
{
  Key_t *keyItem;
  SecureElementGetKeyByID( APP_KEY, &keyItem );

  return &keyItem->KeyValue[0];
}


/**
 * @brief weak function setAppKey(), can be override in application code.
 *
 * @return
 */
__weak const void setAppKey(const uint8_t *key)
{
  SecureElementSetKey(APP_KEY, (uint8_t *) key );
}


/**
 * @brief weak function getLoraInterval(), can be override in application code.
 *
 * @return Interval time in minutes
 */
__weak const uint16_t getLoraInterval(void)
{

  return 5;
}

/**
 * @brief weak function setLoraInterval(), can be override in application code.
 *
 * @return Interval time in minutes
 */
__weak const int32_t setLoraInterval(uint16_t interval)
{
  return 0;
}

/**
 * @brief weak function getNumberOfSamples(), can be override in application code.
 *
 * @param sensorId
 * @return Number of samples 1-100
 */
__weak const uint16_t getNumberOfSamples(int32_t sensorId)
{

  return 100;
}

/**
 * @brief weak function setNumberOfSamples(), can be override in application code.
 *
 * @param sensorId
 * @return Number of samples 1-100
 */
__weak const int32_t setNumberOfSamples(int32_t sensorId, uint8_t numberOfSamples)
{

  return 100;
}


/**
 * @brief weak function getNumberOfMeasures(), can be override in application code.
 *
 * @return total number of measures
 */
__weak const uint8_t getNumberOfMeasures(void)
{

  return 3;
}

/**
 * @brief weak function getVbusSupply(), can be override in application code.
 *
 * @return battery supply in mV
 */
__weak const uint16_t getVbusSupply(void)
{

  return 1250;
}

/**
 * @brief weak function getVccSupply(), can be override in application code.
 *
 * @return battery supply in mV
 */
__weak const uint16_t getVccSupply(void)
{

  return 3300;
}

/**
 * @brief weak function getAlwaysOn(), can be override in application code.
 *
 * @return always on status
 */
__weak const bool getAlwaysOn(void)
{

  return 0;
}

/**
 * @brief weak function setAlwaysOn(), can be override in application code.
 *
 * @return always on status
 */
__weak const void setAlwaysOn(bool state)
{

  return;
}

/**
 * @brief weak function eraseCompleteMeasurementLog(), can be override in application code.
 *
 * @return erase status
 */
__weak const int8_t eraseCompleteMeasurementMemory(void)
{
  return 0;
}

/**
 * @fn const uint8_t eraseCompleteMeasurementBlockByBlock(uint32_t*)
 * @brief weak function eraseCompleteMeasurementBlockByBlock(), can must override in application
 *
 * @param startAddress
 * @return
 */
__weak const int8_t eraseCompleteMeasurementBlockByBlock( uint32_t * startAddress )
{
  return 0;
}

/**
 * @fn uint32_t getLatestMeasurementId(void)
 * @brief weak function getLatestMeasurementId(), can be override in application code
 *
 * @return
 */
__weak const uint32_t getLatestMeasurementId(void)
{
  return 0;
}

/**
 * @fn uint32_t getOldestMeasurementId(void)
 * @brief weak function getOldestMeasurementId(), can be override in application code
 *
 * @return
 */
__weak const uint32_t getOldestMeasurementId(void)
{
  return 0;
}

/**
 * @fn int32_t getSensorStatus(int32_t)
 * @brief weak function getSensorStatus(), can be override in application code
 *
 * @param sensorId
 * @return
 */
__weak const int32_t getSensorStatus(int32_t sensorId)
{
  return 0;
}

/**
 * @fn int32_t setSensorStatus(int32_t, bool)
 * @brief weak function setSensorStatus(), can be override in application code
 *
 * @param sensorId
 * @param status
 * @return
 */
__weak const int32_t setSensorStatus(int32_t sensorId, bool status)
{
  return 0;
}

/**
 * @fn int32_t getSensorType(int32_t)
 * @brief weak function getSensorType(), can be override in application code
 *
 * @param sensorId
 * @return
 */
__weak const int32_t getSensorType(int32_t sensorId)
{
  return 0;
}

/**
 * @fn const uint getProgressFromAddress(uint32_t)
 * @brief weak function getProgressFromAddress(), can be override in application
 *
 * @param address
 * @return
 */
__weak const uint getProgressFromAddress( uint32_t address )
{
  return 0;
}

__weak const int saveSettingsToVirtualEEPROM(void)
{
  return -1;
}

/**
 * @fn int32_t SYS_GetAdc(int)
 * @brief weak function SYS_GetAdc(), can be override in application code
 *
 * @param sensorId
 * @return
 */
__weak const uint32_t SYS_GetAdc(int channel)
{
  return 0;
}


/**
 * @fn int32_t_t_t SYS_GetVoltage(uint32_t)
 * @brief weak function SYS_GetVoltage(), can be override in application code
 *
 * @param sensorId
 * @return
 */
__weak const int32_t SYS_GetVoltage(int channel, uint32_t adcValue)
{
  return 0;
}

/**
 * @fn const int8_t SD_TEST(uint32_t*, uint32_t*)
 * @brief weak function  SD_TEST(), must be override application
 *
 * @param capacity
 * @param free
 * @return
 */
__weak const int8_t SD_TEST(uint32_t * capacity, uint32_t * free)
{
  *capacity = 0;
  *free = 0;
  return -1;
}

/**
 * @fn const int8_t testFram(uint8_t * status)
 * @brief weak function testFram(), must be override in application
 *
 * @return
 */
__weak const int8_t testFram(uint8_t * status)
{
  return -1;
}

/**
 * @fn const int8_t testDataflash(uint32_t_t*)
 * @brief weak function testDataflash(), must be override in application
 *
 * @param status
 * @return
 */
__weak const int8_t testDataflash(uint8_t test, uint32_t * status)
{
  return -1;
}

/**
 * @fn const void setLedTest(int8_t)
 * @brief weak function setLedTest(), must be override in application
 *
 * @param test
 */
__weak const void setLedTest(int8_t test)
{
  return;
}

/**
 * @fn const void testOutput_board_io(uint8_t, bool)
 * @brief weak testOutput_board_io() function, must be override in application
 *
 * @param item
 * @param state
 */
__weak const void testOutput_board_io(uint8_t item, bool state)
{
  UNUSED(item);
  UNUSED(state);
  return;
}

/**
 * @fn const int8_t testInput_board_io(uint8_t)
 * @brief weak testInput_board_io() function, must be override in application
 *
 * @param item
 * @return
 */
__weak const int8_t testInput_board_io(uint8_t item)
{
  UNUSED(item);
  return -98;
}

/**
 * @fn const bool getLigthSensorStatus(void)
 * @brief weak function getLigthSensorStatus(), must be override in application
 *
 * @return
 */
__weak const bool getLigthSensorStatus(void)
{
  return 0;
}

/**
 * @fn const void testRtc(uint8_t*)
 * @brief
 *
 * @param status
 */
__weak const void testRTC( int mode, struct_dateTime * time )
{
  UNUSED(mode);
  UNUSED(time);
}

/**
 * @fn const void testBatMon(int, int32_t*)
 * @brief
 *
 * @param mode
 * @param value
 */
__weak const void testBatMon( int mode, int32_t * value )
{
  UNUSED(mode);
  UNUSED(value);
}

/**
 * @fn const void testSystemChecks(int, int32_t*)
 * @brief
 *
 * @param mode
 * @param value
 */
__weak const void testSystemChecks( int mode, int32_t value )
{
  UNUSED(mode);
  UNUSED(value);
}

/**
 * @fn const uint8_t getBufferSize(void)
 * @brief function to get maximum buffer side.
 *
 * @return
 */
const uint8_t getBufferSize(void)
{
  return loraBufferSize;
}

void sendError(int arguments, const char * format, ... );
void sendOkay(int arguments, const char * format, ... );
void sendModuleInfo(int arguments, const char * format, ... );
void sendSensorInfo(int arguments, const char * format, ...);
void sendJoinID(int arguments, const char * format, ...);
void sendDeviceID(int arguments, const char * format, ...);
void sendAppKey(int arguments, const char * format, ...);
void sendSensor(int arguments, const char * format, ...);
void sendLoraInterval(int arguments, const char * format, ...);
void sendMeasureTime(int arguments, const char * format, ...); //todo remove
void sendSamples(int arguments, const char * format, ...);
void sendAlwaysOnState(int arguments, const char * format, ...);
void sendDataDump(int arguments, const char * format, ...);
void sendDataLine( uint32_t );
void sendBatterijStatus(int arguments, const char * format, ...);
void sendVbusStatus(int arguments, const char * format, ...);
void sendVccStatus(int arguments, const char * format, ...);
void sendAdc( int subTest );
void sendTestSD( int test );
void sendTestFRAM( int test );
void sendTestDataflash( int test, int subTest, char * extraArguments );
void sendTestRTC( int test, int subTest, char * extraArguments );
void sendTestBatMonitor( int test, int subTest);
void sendTestSystemCheck(int test, int subTest, char * extraArguments);
void sensLoraTxBufSize(int test, int subTest);

void rcvJoinId(int arguments, const char * format, ...);
void rcvDeviceID(int arguments, const char * format, ...);
void rcvAppKey(int arguments, const char * format, ...);
void rcvSensor(int arguments, const char * format, ...);
void rcvLoraInterval(int arguments, const char * format, ...);
void rcvMeasureTime(int arguments, const char * format, ...); //todo remove
void rcvSamples(int arguments, const char * format, ...);
void rcvAlwaysOnState(int arguments, const char * format, ...);
void rcvErase(int arguments, const char * format, ...);
void sendProgressLine( uint8_t percent, const char * command  );
void rcvTest(int arguments, const char * format, ...);
void rcvSave(int arguments, const char * format, ...);
void rcvReboot(int arguments, const char * format, ...);
void rcvInitSensor(int arguments, const char * format, ...);

/**
 * definition of GET commands
 */
struct_commands stCommandsGet[] =
{
    {
        cmdModuleInfo,
        sizeof(cmdModuleInfo) - 1,
        sendModuleInfo,
        0,
    },
    {
        cmdSensorInfo,
        sizeof(cmdSensorInfo) - 1,
        sendSensorInfo,
        1,
    },
    {
        cmdJoinId,
        sizeof(cmdJoinId) - 1,
        sendJoinID,
        0,
    },
    {
        cmdDeviceId,
        sizeof(cmdDeviceId) - 1,
        sendDeviceID,
        0,
    },
    {
        cmdAppKey,
        sizeof(cmdAppKey) - 1,
        sendAppKey,
        0,
    },
    {
        cmdSensor,
        sizeof(cmdSensor) - 1,
        sendSensor,
        1,
    },
    {
        cmdLoraInterval,
        sizeof(cmdLoraInterval) - 1,
        sendLoraInterval,
        0,
    },
    { //todo remove cmdMeasureTime
        cmdMeasureTime,
        sizeof(cmdMeasureTime) - 1,
        sendMeasureTime,
        0,
    },
    {
        cmdSamples,
        sizeof(cmdSamples) - 1,
        sendSamples,
        0,
    },
    {
        cmdAlwaysOn,
        sizeof(cmdAlwaysOn) - 1,
        sendAlwaysOnState,
        1,
    },
    {
        cmdDataDump,
        sizeof(cmdDataDump) - 1,
        sendDataDump,
        0,
    },
    {
        cmdBat,
        sizeof(cmdBat) - 1,
        sendBatterijStatus,
        0,
    },
    {
        cmdVbus,
        sizeof(cmdVbus) - 1,
        sendVbusStatus,
        0,
    },
    {
        cmdVcc,
        sizeof(cmdVcc) - 1,
        sendVccStatus,
        0,
    },
    //todo complete all GET commands
};

/**
 * definition of SET commands
 */
struct_commands stCommandsSet[] =
{
    {
        cmdJoinId,
        sizeof(cmdJoinId) - 1,
        rcvJoinId,
        1,
    },
    {
        cmdDeviceId,
        sizeof(cmdDeviceId) - 1,
        rcvDeviceID,
        0,
    },
    {
        cmdAppKey,
        sizeof(cmdAppKey) - 1,
        rcvAppKey,
        1,
    },
    {
        cmdSensor,
        sizeof(cmdSensor) - 1,
        rcvSensor,
        2,
    },
    {
        cmdLoraInterval,
        sizeof(cmdLoraInterval) - 1,
        rcvLoraInterval,
        1,
    },
    {//todo remove cmdMeasureTime
        cmdMeasureTime,
        sizeof(cmdMeasureTime) - 1,
        rcvMeasureTime,
        1,
    },
    {
        cmdSamples,
        sizeof(cmdSamples) - 1,
        rcvSamples,
        1,
    },
    {
        cmdAlwaysOn,
        sizeof(cmdAlwaysOn) - 1,
        rcvAlwaysOnState,
        1,
    },
    {
        cmdErase,
        sizeof(cmdErase) - 1,
        rcvErase,
        0,
    },
    {
        cmdTest,
        sizeof(cmdTest) - 1,
        rcvTest,
        0,
    },
    {
        cmdSave,
        sizeof(cmdSave) - 1,
        rcvSave,
        0,
    },
    {
        cmdReboot,
        sizeof(cmdReboot) - 1,
        rcvReboot,
        0,
    },
    {
        cmdInitSensor,
        sizeof(cmdInitSensor) - 1,
        rcvInitSensor,
        0,
    }
    //todo complete all SET commands
};

/**
 * @brief uart handler
 * is called periodically by scheduler
 */
void UartConfigHandlerProcess(void)
{
  /*
   * call uart handler function
   */
  configUartHandler();

  /*
   * check uart needs to be active
   */
  if( UTIL_TIMER_IsRunning(&uartConfigActive_Timer))
  {
    UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_UartConfig), CFG_SEQ_Prio_1);
  }

}

/**
 * @brief Transmit complete callback function
 *  - trigger the task CFG_SEQ_Task_UartConfig
 *  - starts the uart receiver by DMA
 *
 * @param huart
 */
void uartSendReady_Config(UART_HandleTypeDef *huart)
{
  UTIL_TIMER_StartWithPeriod(&uartConfigActive_Timer, uartConfigActiveTime_default ); //set activation time of task
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_UartConfig), CFG_SEQ_Prio_1); //activate task
  uartStartReceive_Config(bufferRxConfig, sizeof(bufferRxConfig), RECEIVE_TIMEOUT_COMMAND ); //start receiver to listen
}

/**
 * @fn void uartListen(void)
 * @brief function to activate uart listening until timeout
 *
 */
void uartListen(void)
{
  uartSendReady_Config( &config_uart );
}

/**
 * @fn void uartKeepListen(bool)
 * @brief function to enable / disable the keep the uart in listen mode
 * @note if mode is active STOP2 mode will not be active.
 *
 * @param keepListenAcitve
 */
void uartKeepListen(bool keepListenAcitve)
{
  uartConfigKeepListen = keepListenAcitve;
}

/**
 * @fn void trigger_uartConfigActiveTimeout(void*)
 * @brief timeout function of TIMER, to stop the uartConfig
 *
 * @param context
 */
void trigger_uartConfigActiveTimeout( void *context )
{
  UTIL_TIMER_Stop(&uartConfigActive_Timer); //stop timer
  if( uartConfigKeepListen
#ifdef NO_SLEEP
      || true
#endif
      )
  {
    UTIL_TIMER_Start(&uartConfigActive_Timer); //start again.
  }
}

/**
 * @brief initialization function for the uart config command handling
 *  - configure a task for the scheduler with low priority and trigger it.
 *  - register TX complete callback
 *
 */
void uartInit_Config( void )
{
  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_UartConfig), UTIL_SEQ_RFU, UartConfigHandlerProcess);
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_UartConfig), CFG_SEQ_Prio_1);

  HAL_UART_RegisterCallback(&config_uart, HAL_UART_TX_COMPLETE_CB_ID, uartSendReady_Config);

  UTIL_TIMER_Create(&uartConfigActive_Timer, uartConfigActiveTime_default, UTIL_TIMER_ONESHOT, trigger_uartConfigActiveTimeout, NULL); //create timer
  UTIL_TIMER_Start(&uartConfigActive_Timer);
}

/**
 * @brief wrapper function send data to uart for Config commands
 *
 * @param pData Pointer to data buffer (u8 or u16 data elements).
 * @param Size  Amount of data elements (u8 or u16) to be sent.
 *
 * @return HAL status
 */
HAL_StatusTypeDef uartSend_Config( const uint8_t *pData, const uint16_t Size)
{
  UTIL_TIMER_SetPeriod(&uartConfigActive_Timer, uartConfigActiveTime_default ); //set activation time of task
	return uartSend(&config_uart, pData, Size);
}


/**
 * @brief function to start data receive of uart for config commands.
 *
 * @param pData   buffer of receive data, must be DMA type for aligning
 * @param Size    size of receive buffer
 * @param timeout TimeoutValue receiver timeout value in number of baud blocks. The timeout
  *                     value must be less or equal to 0x0FFFFFFFF.
 */
void uartStartReceive_Config( uint8_t *pData, const uint16_t Size, const uint32_t timeout )
{
	HAL_UART_AbortReceive( &config_uart ); //abort a previous uart receive
	setCharacterMatch(&config_uart, '\n'); //set uart matching character to '\n' line feed.
	HAL_StatusTypeDef halRes = HAL_UART_Receive_DMA(&config_uart, pData, Size); //start DMA receive
	UNUSED(halRes); //prevent warning

	HAL_UART_ReceiverTimeout_Config(&config_uart, timeout); //configure receive timeout, start after character is received.
	HAL_UART_EnableReceiverTimeout( &config_uart ); //enable the receive timeout.

	assert_param( halRes == HAL_OK);
}


/**
 * @fn void executeTest(int, int)
 * @brief function to select test
 *
 * @param test
 * @param subTest
 */
void executeTest(int test, int subTest, char * extraArguments)
{
  char *ptr; //dummy pointer
  int value;

  switch( test )
  {
    case 0:

      saveStatusTestmode(subTest ? true : false);
      snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%d,%d\r\n", cmdTest, test,  subTest);
      uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));

      break;

    case 1: //request of software version

      sendModuleInfo(0, 0); //send software versions

      break;

    case 2: //request of ADC measure

      sendAdc(subTest); //get ADC

      break;

    case 3: //LED test

      setLedTest(subTest);

      break;

    case 4: //input test

      value = (int)testInput_board_io((uint8_t)subTest);
      snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%d,%d,%d\r\n", cmdTest, test,  subTest, value);
      uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));

      break;

    case 5: //output test

      value = strtol(additionalArgumentsString+1, &ptr, 10); //skip <comma>, increment ptr.
      setLedTest(99);
      testOutput_board_io((uint8_t)subTest, value ? true : false);

      break;

    case 6: //SD card test

      sendTestSD(test);

      break;

    case 7: //RTC test

      sendTestRTC(test, subTest, extraArguments);


      break;

    case 8: //battery monitor test

      sendTestBatMonitor(test, subTest);

      break;

    case 9: //test light sensor

      value = (int)getLigthSensorStatus();
      snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%d,%d\r\n", cmdTest, test, value);
      uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));

      break;

    case 10: //test FRAM

      sendTestFRAM(test);

      break;

    case 11: //test dataflash

      sendTestDataflash(test, subTest, extraArguments);

      break;

    case 12: //test system checks

      sendTestSystemCheck(test, subTest, extraArguments);

      break;

    case 99:

      sensLoraTxBufSize(test, subTest);

      break;

    default:

      sendError(0, 0);  //unknwon command, send error.

      break;
  }

}

/**
 * @brief config uart handler for timeout and special long commands.
 *
 */
void configUartHandler(void)
{
  static int step = 0;
  int8_t result;
  static uint16_t numberOfMeasures = 0;
  static uint32_t latestMeasurment = 0;
  static uint32_t currentMeasurement = 0;
  static uint32_t previousTimeMs = 0;
  uint32_t currentTimeMs;
  bool updateRate = false;

  currentTimeMs = SysTimeToMs(SysTimeGet()); //get current time in milliseconds

  if( currentTimeMs > previousTimeMs + 200 )  //detect time passed, 200ms
  {
    previousTimeMs = currentTimeMs;
    updateRate = true;  //set updateRate flag
  }

  if (uartTxReady(&config_uart)) //check uart is ready
  {
    //state machine
    switch ( step )
    {

      case 0: //start receive data

        uartStartReceive_Config(bufferRxConfig, sizeof(bufferRxConfig), RECEIVE_TIMEOUT_COMMAND );
        step++;

        break;

      case 1: //send test data

        sendModuleInfo(0,0);
        step++;

        break;

      case 2: //wait, on special command

        //handling special commands
        if( dataDump ) // data command received
        {
          numberOfMeasures = getNumberOfMeasures(); //get number of log items
          latestMeasurment = getLatestMeasurementId(); //get latest measurement ID
          currentMeasurement = getOldestMeasurementId(); //get oldest measurement ID

          snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:count: %d, oldest: %lu, latest: %lu\r\n", cmdDataDump, numberOfMeasures, currentMeasurement, latestMeasurment);
          uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));

          //check command is not dump  ALL
          if( numberOffDumpRecords != DUMP_ALL )
          {
            if( numberOffDumpRecords < numberOfMeasures ) //check if number of measures needs to be limit by input parameter
            {
              numberOfMeasures = numberOffDumpRecords;
              currentMeasurement = latestMeasurment - numberOfMeasures; //calculate new start offset
            }
          }

          step = 3; //go to process
        }

        else if( dataErase ) //data erase command received
        {
          dataErase = false;
          currentMeasurement = 0; //set address to 0 (use currentMeasurement variable)
          step = 10;
        }

        else if(dataTest)
        {
          dataTest = false;
          step = 20;
        }

        break;

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// process dataDump command
    ////////////////////////////////////////////////////////////////////////////////////////////

      case 3:

        if( numberOfMeasures ) //check items need to print
        {
          sendDataLine(currentMeasurement++); //print current log item
          numberOfMeasures--;         //decrement

        }
        else
        { //ready

          dataDump = false; //reset
          sendOkay(1, cmdDataDump); //send ready
          step = 2; //back to wait

        }

        break;

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// Process dataErase command
    ////////////////////////////////////////////////////////////////////////////////////////////

      case 10:

        result = eraseCompleteMeasurementBlockByBlock(&currentMeasurement); //erase one block

        if( result < 0 )
        { //error
          sendError(0,0);
          step = 2; //back to wait
        }
        else if( result == 0 )
        { //busy
          if( updateRate) //check update rate flag is active
          {
            sendProgressLine(getProgressFromAddress(currentMeasurement), cmdErase); //send progress line
          }
        }
        else
        { //ready
          sendProgressLine(getProgressFromAddress(currentMeasurement), cmdErase);//send 100%
          step = 11; //got to send ready
        }

        break;

      case 11:

        sendOkay(1,cmdErase); //send ready
        step = 2; //back to wait

        break;

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// Process test command
    ////////////////////////////////////////////////////////////////////////////////////////////

      case 20:

        executeTest(currentTest, currentSubTest, additionalArgumentsString);
        step = 2; //back to wait

        break;

    ////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////

      default:
        break;

    }
  }

  // catch Receive timeout flag
  if( HAL_UART_GetError(&config_uart) & HAL_UART_ERROR_RTO )
  {
    uartStartReceive_Config(bufferRxConfig, sizeof(bufferRxConfig), 10*config_uart.Init.BaudRate);
  }

}

/**
 * @fn uint8_t getLengthCommand(const char*)
 * @brief helper function to get the length of the command
 *
 * @param data
 * @return
 */
uint8_t getLengthCommand(const char * data)
{
  char test[]="=\r\n";
  uint8_t result = strcspn(data, test);
  return result;
}

/**
 * @brief callback function for character match to process the received data.
 *
 * @param huart current uart pointer, must be "config_uart"
 * @param nrCharsAvailable number of characters received
 */
void user_uart2_characterMatchDetect_Callback(UART_HandleTypeDef *huart, uint32_t nrCharsAvailable)
{
	assert_param( huart == &config_uart );
	assert_param( nrCharsAvailable != 0 );

	bool commandProcessed = FALSE;

	HAL_UART_AbortReceive(&config_uart);

	/*
	 * check command start with "Set", use strncasecmp() to ignore the case sensitivity
	 */
	if( strncasecmp ( (char*)bufferRxConfig, cmdSet, strlen(cmdSet)) == 0 )
	{
	  /*
	   * loop for search the received command
	   */
		for( int i = 0; i < sizeof( stCommandsSet) / sizeof(stCommandsSet[0]); i++ )
		{
		  /*
		   * check command is found, use strncasecmp() to ignore the case sensitivity
		   */
		  if( strncasecmp( (char*)bufferRxConfig + strlen(cmdSet), stCommandsSet[i].command, stCommandsSet[i].commandLength ) == 0 && getLengthCommand((char*)bufferRxConfig + strlen(cmdSet)) == stCommandsSet[i].commandLength )
		  {
		    /*
		     * command is founded, then execute the callback fucntion.
		     */
		    char * startOffArguments = (char *)bufferRxConfig + strlen(cmdSet) + stCommandsSet[i].commandLength;

		    if( stCommandsSet[i].function != 0 ) //always check pointer is not zero before executing.
		    {
		      stCommandsSet[i].function(stCommandsSet[i].numberOfArguments, startOffArguments);
		      commandProcessed = TRUE; //detect command is processed.
		    }
		  }
		}
	}

	/*
   * check command start with "Get", use strncasecmp() to ignore the case sensitivity
   */
	if( strncasecmp ( (char*)bufferRxConfig, cmdGet, strlen(cmdGet)) == 0 )
  {
	  /*
	   * loop for search the received command
	   */
    for( int i = 0; i < sizeof( stCommandsGet) / sizeof(stCommandsGet[0]); i++ )
    {
      /*
       * check command is found, use strncasecmp() to ignore the case sensitivity
       */
      if( strncasecmp( (char*)bufferRxConfig + strlen(cmdGet), stCommandsGet[i].command, stCommandsGet[i].commandLength ) == 0 && getLengthCommand((char*)bufferRxConfig + strlen(cmdGet)) == stCommandsGet[i].commandLength)
      {
        /*
         * command is founded, then execute the callback fucntion.
         */
        char * startOffArguments = (char *)bufferRxConfig + strlen(cmdGet) + stCommandsGet[i].commandLength;
        if( stCommandsGet[i].function != 0 ) //always check pointer is not zero before executing.
        {
          stCommandsGet[i].function(stCommandsGet[i].numberOfArguments, startOffArguments);
          commandProcessed = TRUE;
        }
      }
    }

  }

	/*
	 * check command is not processed
	 */
	if( commandProcessed == FALSE )
	{
	  sendError(0,0); //reply with error.
	}
}

/**
 * @brief send module information to config uart.
 *
 * @param arguments not used
 */
void sendModuleInfo(int arguments, const char * format, ...)
{
  snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%s,%s,%s\r\n", cmdModuleInfo, getProtocolVersionConfig(), getProtocolVersionSensor(), getSoftwareVersionMFM());
  uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));
}

/**
 * @brief send sensor information to config uart.
 *
 * @param arguments: 1: <sensor slot number> 1-6
 */
void sendSensorInfo(int arguments, const char * format, ...)
{
  for(int sensorId=1; sensorId<=6; sensorId++)
  {
    snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%d,%u,%s\r\n", cmdSensorInfo, sensorId, getProtocolSensorboard(sensorId-1), getSoftwareSensorboard(sensorId-1));
    uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));
  }
}

/**
 * @brief send current JoinId to config uart.
 *
 * @param arguments not used
 */
void sendJoinID(int arguments, const char * format, ...)
{

  snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:0x%02X%02X%02X%02X%02X%02X%02X%02X\r\n", cmdJoinId, HEX8( getJoinId() ) );
  uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));
}

/**
 * @brief receive new JoinId from config uart.
 *
 * @param argument: 1: <joinId number>
 */
void rcvJoinId(int arguments, const char * format, ...)
{
  char *ptr; //dummy pointer

  uint8_t joinEui[SE_EUI_SIZE]={0}; //set all elements to zero
  int i = SE_EUI_SIZE - 1; //start at last element.
  uint64_t JoinId = 0;

  if( format[0] == '=' && format[1] == '0' && format[2] == 'x')
  {

    JoinId = strtoull(&format[1], &ptr, 0); // convert string 0x## to uint64_t
                                           // <endptr> : not used
                                           // <base> = 0 : An optional prefix indicating octal or hexadecimal base ("0" or "0x"/"0X" respectively)

    //save each byte in uint8_t array.
    do{
      joinEui[i] = JoinId & 0xFF; //save one byte, start at end element.
      JoinId>>=8; //shift one byte (8bits) to right to get next byte.
    }while(i--); //untill all elements are done

    //detect a new ID
    if( memcmp((char*)getJoinId(), (char*)joinEui, sizeof(joinEui)) != 0)
    {
      detectChangeRebootNeeded = true;
    }

    setJoinId(joinEui); //set new JoinId.
  }

  snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:0x%02X%02X%02X%02X%02X%02X%02X%02X\r\n", cmdJoinId, HEX8( getJoinId() ) );
  uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));
}

/**
 * @brief send current DeviceId to config uart.
 *
 * @param arguments not used
 */
void sendDeviceID(int arguments, const char * format, ...)
{

  snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:0x%02X%02X%02X%02X%02X%02X%02X%02X\r\n", cmdDeviceId, HEX8( getDeviceId() ) );
  uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));

}

/**
 * @brief receive DeviceId from config uart.
 *
 * @param arguments not used
 */
void rcvDeviceID(int arguments, const char * format, ...)
{

  char *ptr; //dummy pointer
  bool notZero = false;
    uint8_t joinEui[SE_EUI_SIZE]={0}; //set all elements to zero
    int i = SE_EUI_SIZE - 1; //start at last element.
    uint64_t JoinId = 0;

    if( format[0] == '=' && format[1] == '0' && format[2] == 'x')
    {

      JoinId = strtoull(&format[1], &ptr, 0); // convert string 0x## to uint64_t
                                             // <endptr> : not used
                                             // <base> = 0 : An optional prefix indicating octal or hexadecimal base ("0" or "0x"/"0X" respectively)

      //save each byte in uint8_t array.
      do{
        joinEui[i] = JoinId & 0xFF; //save one byte, start at end element.
        JoinId>>=8; //shift one byte (8bits) to right to get next byte.
        if( joinEui[i] != 0x00 )
        {
          notZero=true;
        }
      }while(i--); //untill all elements are done

      if( notZero == false )
      {
        GetUniqueId(joinEui); //read unique serial when 0 is set.
      }

      //detect a new ID
      if( memcmp((char*)getDeviceId(), (char*)joinEui, sizeof(joinEui)) != 0)
      {
        detectChangeRebootNeeded = true;
      }

      setDeviceId(joinEui); //set new JoinId.
    }

    snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:0x%02X%02X%02X%02X%02X%02X%02X%02X\r\n", cmdDeviceId, HEX8( getDeviceId() ) );
    uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));

}

/**
 * @brief send current AppKey to config uart.
 *
 * @param arguments not used
 */
void sendAppKey(int arguments, const char * format, ...)
{
  snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n", cmdAppKey, HEX16( getAppKey() ) );
  uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));

}

/**
 * @brief receive new AppKey from config uart.
 *
 * @param argument: 1: <AppKey number>
 */
void rcvAppKey(int arguments, const char * format, ...)
{
  char *ptr; //dummy pointer
  char halfKey[(APP_KEY_CHARACTERS>>1) + 1] = {0};
  Key_t newKey = {0};
  int i = SE_KEY_SIZE - 1; //start at last element.
  uint64_t AppKeyMsb = 0;
  uint64_t AppKeyLsb = 0;
  int length = strcspn (&format[3], "\r\n");

  if( format[0] == '=' && format[1] == '0' && format[2] == 'x' && length > 0 && length <=32 )
  {


    if( length > APP_KEY_CHARACTERS>>1 )
    {
      memcpy(halfKey, &format[19], APP_KEY_CHARACTERS>>1 );
      AppKeyLsb = strtoull(&halfKey[0], &ptr, APP_KEY_CHARACTERS>>1);  // convert string ## to uint64_t
                                                  // <endptr> : not used
                                                  // <base> = 16 : hexadecimal

      memcpy(halfKey, &format[3], APP_KEY_CHARACTERS>>1 );
      AppKeyMsb = strtoull(&halfKey[0], &ptr, APP_KEY_CHARACTERS>>1);  // convert string 0x## to uint64_t
                                                        // <endptr> : not used
                                                        // <base> = 16 : hexadecimal

    }
    else
    {
      memcpy(halfKey, &format[3], APP_KEY_CHARACTERS>>1 );
      AppKeyLsb = strtoull(&halfKey[0], &ptr, APP_KEY_CHARACTERS>>1);  // convert string 0x## to uint64_t
                                                        // <endptr> : not used
                                                        // <base> = 16 : hexadecimal

    }

    //save each byte in uint8_t array.
    do{

      if( i >= (SE_KEY_SIZE>>1) )
      {
        newKey.KeyValue[i] = AppKeyLsb & 0xFF; //save one byte, start at end element.
        AppKeyLsb>>=8; //shift one byte (8bits) to right to get next byte.
      }
      else
      {
        newKey.KeyValue[i] = AppKeyMsb & 0xFF; //save one byte, start at end element.
        AppKeyMsb>>=8; //shift one byte (8bits) to right to get next byte.

      }

    }while(i--); //untill all elements are done

    //detect a new key
    if( memcmp((char*)getAppKey(), (char*)newKey.KeyValue, sizeof(newKey.KeyValue)) != 0)
    {
      detectChangeRebootNeeded = true;
    }

    setAppKey((uint8_t*)&newKey.KeyValue[0]); //set new AppKey.

    snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n", cmdAppKey, HEX16( getAppKey() ) );
    uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));
  }
  else
  {
    sendError(0,0);
  }
}

/**
 * @brief send current Sensor data to config uart.
 *
 * @param arguments: 1: <sensor number> 1-6
 */
void sendSensor(int arguments, const char * format, ...)
{
  int sensorId = 0;
  int32_t sensorStatus = 0;
  int32_t sensorType = 0;
  if( format[0] == '=' && format[2] == '\r' && format[3] == '\n')
  {
    sensorId = atoi(&format[1]);
  }

  sensorStatus = getSensorStatus(sensorId); //get sensor status, value < 0 is error
  sensorType = getSensorType(sensorId); //get sensor type, value < 0 is error

  //check sensor range
  if( sensorStatus >= 0 && sensorType >=0 ) //only check value >= zero, check on sensorId not needed indirect with getSensStatus() and getSensorTyp()
  {
    snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%d,%d,%d\r\n", cmdSensor, sensorId, (bool)sensorStatus, (int16_t)sensorType);
    uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));
  }
  else
  {
    sendError(0,0);
  }
}

/**
 * @brief receive new sensor information from config uart.
 *
 * @param argument: 1: <sensor number>, 2: <sensur status>
 *
 */
void rcvSensor(int arguments, const char * format, ...)
{
  char *ptr; //dummy pointer
  int sensorId = -1;
  int32_t sensorStatus = -1;
  int32_t sensorType = 0;
  int32_t resultProcessed = -1;

  if( format[0] == '=' && format[2] == ',' && format[4] == '\r' && format[5] == '\n')
  {
    sensorId = strtol(&format[1], &ptr, 10);
    sensorStatus = strtol(ptr+1, &ptr, 10); //skip <comma>, increment ptr.
    resultProcessed = setSensorStatus(sensorId, sensorStatus);//set new value and verify if accepted.
  }

  sensorStatus = getSensorStatus(sensorId); //get sensor status, value < 0 is error
  sensorType = getSensorType(sensorId); //get sensor type, value < 0 is error

  if( sensorStatus >= 0 && sensorType >= 0 && resultProcessed >= 0 ) //only check value >= zero and resultProcessed >= zero, check on sensorId not needed indirect with getSensStatus() and getSensorTyp()
  {
    snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%d,%d,%u\r\n", cmdSensor, sensorId, (bool)sensorStatus, (uint16_t)sensorType);
    uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));
  }
  else
  {
    sendError(0,0);
  }

}


/**
 * @brief send current read interval to config uart.
 *
 * @param arguments not used
 */
void sendLoraInterval(int arguments, const char * format, ...)
{

  snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%d\r\n", cmdLoraInterval, getLoraInterval() );
  uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));

}

/**
 * @brief receive new read interval from config uart.
 *
 * @param argument: 1: <Interval>
 *
 */
void rcvLoraInterval(int arguments, const char * format, ...)
{
  char *ptr; //dummy pointer
  int interval = 0;
  int32_t resultProcessed = -1;


  if( format[0] == '=' ) //check index 0 is "="
  {
    interval = strtol(&format[1], &ptr, 10); //convert string to number
  }

  resultProcessed = setLoraInterval((uint16_t)interval);//set lora interval

  if( resultProcessed >= 0 )
  {
    snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%d\r\n", cmdLoraInterval, getLoraInterval() );
    uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));
  }
  else
  {
    sendError(0,0);
  }

}

/**
 * @brief send current measure time to config uart.
 *
 * @param arguments not used
 */
void sendMeasureTime(int arguments, const char * format, ...)
{
  char *ptr; //dummy pointer
  int sensorId = -1;
  uint16_t measureTime = -1;

  if( format[0] == '=' && format[2] == '\r' && format[3] == '\n') //check index 0 is "=" and index 2 is "\r" and index 3 is "\n"
  {
    sensorId = strtol(&format[1], &ptr, 10); //convert string to number
  }

  measureTime = getNumberOfSamples(sensorId);

  if( measureTime >= 0 && measureTime <= 60000 )
  {
    snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%d\r\n", cmdMeasureTime, getNumberOfSamples(sensorId) );
    uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));
  }
  else
  {
    sendError(0,0);
  }

}

/**
 * @brief send current number of samples to config uart.
 *
 * @param arguments not used
 */
void sendSamples(int arguments, const char * format, ...)
{
  char *ptr; //dummy pointer
  int sensorId = -1;
  uint16_t numberOfSamples = -1;

  if( format[0] == '=' && format[2] == '\r' && format[3] == '\n') //check index 0 is "=" and index 2 is "\r" and index 3 is "\n"
  {
    sensorId = strtol(&format[1], &ptr, 10); //convert string to number
  }

  numberOfSamples = getNumberOfSamples(sensorId);

  if( numberOfSamples >= 1 && numberOfSamples <= 100 )
  {
    snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%d\r\n", cmdSamples, getNumberOfSamples(sensorId) );
    uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));
  }
  else
  {
    sendError(0,0);
  }

}


/**
 * @brief receive new measure time from config uart.
 *
 * @param argument: 1: <measure time in ms>
 *
 */
void rcvMeasureTime(int arguments, const char * format, ...)
{
  char *ptr; //dummy pointer

  int sensorId = -1;
  int32_t measureTime = -1;
  int32_t resultProcessed = -1;

  if( format[0] == '=' && format[2] == ',' ) //check index 0 is "=" and index 2 is ","
  {
    sensorId = strtol(&format[1], &ptr, 10); //convert string to number
    measureTime = strtol(ptr+1, &ptr, 10); //convert string to number, skip <comma>, increment ptr.

    resultProcessed = setNumberOfSamples(sensorId, measureTime); //save value

  }

  if( resultProcessed >= 0 ) //verify saving result is zero or larger
  {
    snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%d\r\n", cmdMeasureTime, getNumberOfSamples(sensorId) );
    uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));
  }
  else
  {
    sendError(0,0);
  }

}


/**
 * @brief receive new number of samples from config uart.
 *
 * @param argument: 1: <number of samples>
 *
 */
void rcvSamples(int arguments, const char * format, ...)
{
  char *ptr; //dummy pointer

  int sensorId = -1;
  int32_t numberOfSamples = -1;
  int32_t resultProcessed = -1;

  if( format[0] == '=' && format[2] == ',' ) //check index 0 is "=" and index 2 is ","
  {
    sensorId = strtol(&format[1], &ptr, 10); //convert string to number
    numberOfSamples = strtol(ptr+1, &ptr, 10); //convert string to number, skip <comma>, increment ptr.

    resultProcessed = setNumberOfSamples(sensorId, numberOfSamples); //save value

  }

  if( resultProcessed >= 0 ) //verify saving result is zero or larger
  {
    snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%d\r\n", cmdSamples, getNumberOfSamples(sensorId) );
    uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));
  }
  else
  {
    sendError(0,0);
  }

}


/**
 * @brief send datadump to config uart.
 *
 * @param arguments not used
 */
void sendDataDump(int arguments, const char * format, ...)
{
  char *ptr; //dummy pointer

  //check command has a extra argument
  if( format[0] == '=')
  {
    numberOffDumpRecords = strtol(&format[1], &ptr, 10); //convert string to number
  }
  else //no argument, just dump all
  {
    numberOffDumpRecords = DUMP_ALL;
  }

  snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%d\r\n", cmdDataDump, getNumberOfMeasures() );
  uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));

  dataDump = true;  //trigger dataDump in handler
}

__weak int32_t printMeasurementData( uint32_t measurementId, uint8_t * buffer, uint32_t bufferLength )
{
  return 0;
}

/**
 * @brief send data line to config uart.
 *
 * @param arguments not used
 */
void sendDataLine( uint32_t measurementId  )
{
  int length = 0;

  //get data from measurement ID
  length = printMeasurementData( measurementId, bufferTxConfig, sizeof(bufferTxConfig) );

  if( length > 0 )
  {
    uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));
  }
}

/**
 * @brief send batterijstatus to config uart.
 *
 * @param arguments not used
 */
void sendBatterijStatus(int arguments, const char * format, ...)
{
  snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%u,%u\r\n", cmdBat, getBatteryEos().voltage, getBatteryEos().EOS );
  uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));

}

/**
 * @brief send vbus status to config uart.
 *
 * @param arguments not used
 */
void sendVbusStatus(int arguments, const char * format, ...)
{

  snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%d\r\n", cmdVbus, getVbusSupply() );
  uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));

}

/**
 * @brief send vcc status to config uart.
 *
 * @param arguments not used
 */
void sendVccStatus(int arguments, const char * format, ...)
{

  snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%d\r\n", cmdVcc, getVccSupply() );
  uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));

}

/**
 * @fn void sendAdc(int)
 * @brief function to send result of ADC test
 *
 * @param subTest : channel of ADC 0 until 22
 */
void sendAdc( int subTest )
{
  char unit[] = "mV";

  if( subTest == 19 )
  {
    unit[0] = 176;
    unit[1] = 'C';
  }

  uint32_t adcValue = SYS_GetAdc(subTest);
  snprintf( (char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%d,0x%lx,%ld%s\r\n", "ADC", subTest, adcValue, SYS_GetVoltage(subTest, adcValue), unit );
  uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));
}


/**
 * @fn void sendTestSD(int)
 * @brief
 *
 * @param test
 */
void sendTestSD( int test )
{
  uint32_t cardCapacity;
  uint32_t cardFree;
  int8_t result = SD_TEST(&cardCapacity, &cardFree);
  snprintf( (char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%d,%d,%lu,%lu\r\n", cmdTest, test, result == 0 ? 1 : 0, cardCapacity, cardFree);
  uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));
}

/**
 * @fn void sendTestFRAM(int)
 * @brief
 *
 * @param test
 */
void sendTestFRAM( int test )
{
  uint8_t statusRegister;
  int8_t result = testFram(&statusRegister);
  snprintf( (char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%d,%d,0x%02x\r\n", cmdTest, test, result == 0 ? 1 : 0, statusRegister);
  uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));
}

/**
 * @fn void sendTestDataflash(int)
 * @brief
 *
 * @param test
 */
void sendTestDataflash( int test, int subTest, char * extraArguments )
{
  uint32_t statusRegister;
  int32_t value = 0;
  char *ptr; //dummy pointer

  if (subTest == 1)
  {
    value = strtol(additionalArgumentsString+1, &ptr, 10); //read day, skip <comma>, increment ptr.
  }


  if( ( subTest == 1 && value >= 0 && value < 2048) || (subTest >= 2 && subTest <= 3 && additionalArgumentsString[0] == 0 ) )
  {
    statusRegister = (uint32_t)value;
    int8_t result = testDataflash(subTest, &statusRegister);
    snprintf( (char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%d,%d,%d,%lu\r\n", cmdTest, test, result >= 0 ? 1 : 0, result, statusRegister);
    uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));
  }
  else
  {
    sendError(0,0);
  }
}

/**
 * @fn void sendTestRTC(int, int, char*)
 * @brief function to test RTC
 *
 * @param test
 * @param subTest
 * @param extraArguments
 */
void sendTestRTC( int test, int subTest, char * extraArguments )
{
  struct_dateTime dateTime;
  int32_t value;
  bool error = false;
  char *ptr; //dummy pointer

  //only read extra parameters with subtest 1 ( set date/time ).
  if( subTest == 1 )
  {
    value = strtol(additionalArgumentsString+1, &ptr, 10); //read day, skip <comma>, increment ptr.
    if( value >= 1 && value <= 31 ) //check on day number
    {
      dateTime.day = value;
    }
    else
    {
      error = true;
    }

    value = strtol(ptr+1, &ptr, 10); //read month, skip <dash>, increment ptr.
    if( value >= 1 && value <= 12 ) //check on month number
    {
      dateTime.month = value;
    }
    else
    {
      error = true;
    }

    value = strtol(ptr+1, &ptr, 10); //read year, skip <dash>, increment ptr.
    if( value >= 0 && value < 100) //check on year, starts from 2000, input starts at 0.
    {
      dateTime.year = value + 2000;
    }
    else if( value >= 2000 ) //check on year, starts at 2000
    {
      dateTime.year = value;
    }
    else
    {
      error = true;
    }

    value = strtol(ptr+1, &ptr, 10); //read hours, skip <comma>, increment ptr.
    if( value >= 0 && value <=23 ) //check on hours number
    {
      dateTime.hour = value;
    }
    else
    {
      error = true;
    }

    value = strtol(ptr+1, &ptr, 10); //read minutes, skip <colon>, increment ptr.
    if( value >= 0 && value <=59 ) //check on minutes number
    {
      dateTime.minute = value;
    }
    else
    {
      error = true;
    }

    value = strtol(ptr+1, &ptr, 10); //read seconds, field skip <colon>, increment ptr.
    if( value >= 0 && value <=59 ) //check on seconds number.
    {
      dateTime.second = value;
    }
    else
    {
      error = true;
    }
  }

  //check on no errors
  if( error == false && subTest >= 1 && subTest <= 4)
  {
    testRTC(subTest, &dateTime); //execute test
    snprintf( (char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%d,%d,%02d-%02d-%02d,%02d:%02d:%02d\r\n", cmdTest, test, subTest, dateTime.day, dateTime.month, dateTime.year, dateTime.hour, dateTime.minute, dateTime.second); //make response
    uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig)); //send response
  }
  else if( subTest == 5 )
  {
    testRTC(subTest, &dateTime); //execute test
    snprintf( (char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%d,%d\r\n", cmdTest, test, subTest ); //make response
    uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig)); //send response
  }
  else if( subTest == 6 )
  {
    testRTC(subTest, &dateTime); //execute test
    snprintf( (char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%d,%d,0x%02x\r\n", cmdTest, test, subTest, dateTime.century); //make response
    uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig)); //send response
  }
  else if( subTest == 7 )
  {
    testRTC(subTest, &dateTime); //execute test
    snprintf( (char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%d,%d\r\n", cmdTest, test, subTest); //make response
    uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig)); //send response
  }
  else if( subTest == 8)
  {
    setForceMeasurement( true ); //enable force a measurement
    snprintf( (char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%d,%d\r\n", cmdTest, test, subTest); //make response
    uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig)); //send response
  }
  else
  {
    sendError(0,0);
  }
}

/**
 * @fn void sendTestBatMonitor(int, int)
 * @brief function to test battery monitor
 *
 * @param test
 * @param subTest
 */
void sendTestBatMonitor(int test, int subTest)
{
  if (subTest >= 0 && subTest <= 10)
  {
    int32_t value;
    testBatMon(subTest, &value );
    snprintf((char*) bufferTxConfig, sizeof(bufferTxConfig), "%s:%d,%d,%ld\r\n", cmdTest, test, subTest, value); //make response
    uartSend_Config(bufferTxConfig, strlen((char*) bufferTxConfig)); //send response
  }
  else
  {
    sendError(0, 0);
  }
}

/**
 * @fn void sendTestSystemCheck(,)
 * @brief
 *
 * @param test
 * @param subTest
 */
void sendTestSystemCheck(int test, int subTest, char * extraArguments)
{
  char *ptr; //dummy pointer
  if (subTest >= 0 && subTest <= 2)
  {
    int32_t value = strtol(additionalArgumentsString+1, &ptr, 10); //read day, skip <comma>, increment ptr.

    testSystemChecks(subTest, value);
    snprintf((char*) bufferTxConfig, sizeof(bufferTxConfig), "%s:%d,%d,%ld\r\n", cmdTest, test, subTest, value); //make response
    uartSend_Config(bufferTxConfig, strlen((char*) bufferTxConfig)); //send response
  }
  else
  {
    sendError(0, 0);
  }
}

/**
 * @fn void sensLoraTxBufSize(,)
 * @brief
 *
 * @param test
 * @param subTest
 */
void sensLoraTxBufSize(int test, int subTest)
{
  if (subTest >= 0 && subTest <= 255)
  {
    loraBufferSize = subTest;
    snprintf((char*) bufferTxConfig, sizeof(bufferTxConfig), "%s:%d,%u\r\n", cmdTest, test, loraBufferSize); //make response
    uartSend_Config(bufferTxConfig, strlen((char*) bufferTxConfig)); //send response
  }
  else
  {
    sendError(0, 0);
  }
}

/**
 * @brief send alwaysOn supply setting to config uart
 *
 * @param arguments not used
 */
void sendAlwaysOnState(int arguments, const char * format, ...)
{

  snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%d\r\n", cmdAlwaysOn, getAlwaysOn() );
  uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));

}

/**
 * @brief receive alwaysOn supply setting from config uart.
 *
 * @param argument: 1: <always on state>
 *
 */
void rcvAlwaysOnState(int arguments, const char * format, ...)
{
  char *ptr; //dummy pointer
  int status = -1;


  if( format[0] == '=' )
  {
    status = strtol(&format[1], &ptr, 10);
  }

  if( status >= 0 && status <= 1 )
  {
    setAlwaysOn(status); //set alwaysOnState

    snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%d\r\n", cmdAlwaysOn, getAlwaysOn() );
    uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));
  }
  else
  {
    sendError(0,0);
  }

}

/**
 * @brief receive erase command from config uart.
 *
 * @param argument: not used
 *
 */
void rcvErase(int arguments, const char * format, ...)
{
  sendProgressLine(0, cmdErase); //send progress line

  dataErase = true; //trigger dataErase in handler
}

/**
 * @brief send progress line to config uart.
 *
 * @param arguments not used
 */
void sendProgressLine( uint8_t percent, const char * command  )
{
  int length = 0;

  //get data from log ID
  length = snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s=%d%%\r\n", command, percent );

  if( length > 0 )
  {
    uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));
  }
}

/**
 * @brief receive test command from config uart.
 *
 * @param argument: not used
 *
 */
void rcvTest(int arguments, const char * format, ...)
{
  char *ptr; //dummy pointer
  int test = 0;
  int subtest = -1;
  bool clearArgumentString = true;

  if( format[0] == '=' )
  {
    test = strtol(&format[1], &ptr, 10);
    if( *ptr!='\r' && *ptr!='\n')
    {
      subtest = strtol(ptr+1, &ptr, 10); //skip <comma>, increment ptr.
      if( *ptr!='\r' && *ptr!='\n')
      {
        clearArgumentString = false;
        strncpy(additionalArgumentsString, ptr, sizeof(additionalArgumentsString)); //save argument for later use
        additionalArgumentsString[sizeof(additionalArgumentsString)-1]=0;//make sure array is terminated with null character
      }
    }
  }

  if( clearArgumentString )
  {
    memset(additionalArgumentsString, 0x00, sizeof(additionalArgumentsString));//clear array
  }

  //todo set sensorStatus

  if( (test >= 0 && test <= 12) || test == 99 )
  {
    dataTest = true;
    currentTest = test;
    currentSubTest = subtest;

    if( subtest>=0)
    {
      snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%d,%d\r\n", cmdTest, test,  subtest);
    }
    else
    {
      snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%d\r\n", cmdTest, test );
    }

    uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));
  }
  else
  {
    sendError(0,0);
  }
}

/**
 * @fn void rcvSave(int, const char*, ...)
 * @brief receive save command
 *
 * @param arguments
 * @param format
 */
void rcvSave(int arguments, const char * format, ...)
{
  int result = saveSettingsToVirtualEEPROM();

  if( result == 0 )
  {
    sendOkay(1,cmdSave);

    triggerSaveNvmData2Fram(); //force to save JoinId, DevEui and AppKey to NVM

    if( detectChangeRebootNeeded ) //check if reboot is needed, no need to reset variable, because of reboot.
    {
      setRejoinAtNextInterval(); //force a rejoin after the reboot.
      startDelayedReset(); //trigger delayed reboot, to force loading new settings with a new join.
    }
  }

  else
  {
    sendError(0,0);
  }
}

/**
 * @fn void rcvReboot(int, const char*, ...)
 * @brief receive of reboot command
 *
 * @param arguments
 * @param format
 */
void rcvReboot(int arguments, const char * format, ...)
{
  sendOkay(1,cmdReboot);

  startDelayedReset();
}

/**
 * @fn void rcvInitSensor(int, const char*, ...)
 * @brief receive of init sensor command
 *
 * @param arguments
 * @param format
 */
void rcvInitSensor(int arguments, const char * format, ...)
{
  sendOkay(1,cmdInitSensor);

  setForceInitSensor(true);
}

/**
 * @brief function to send ERROR command
 *
 * @param arguments not used
 */
void sendError(int arguments, const char * format, ... )
{
  snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s\r\n", cmdError);
  uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));
}

/**
 * @brief function to send Okay command
 *
 * @param arguments not used
 */
void sendOkay(int arguments, const char * format, ... )
{
  snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s:%s\r\n", format, cmdOkay);
  uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));
}

/**
 * @brief temporary override function to make sure transmit is ready before going in the IDLE mode (STOP2).
 *
 */
void UTIL_SEQ_PreIdle( void )
{
  /*
   * Unless specified by the application, there is nothing to be done
   */

	while( uartTxBusy(&config_uart) )
	{
		asm("NOP");
	}

  return;
}
