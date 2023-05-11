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
#include "main.h"

#include "utilities_def.h"
#include "stm32_seq.h"
#include "stm32wlxx_hal.h"
#include "stm32wlxx_hal_uart.h"

#include "common/app_types.h"
#include "common/uart.h"
#include "CommConfig.h"

#define NO_SLEEP

#define RECEIVE_TIMEOUT_COMMAND  (5 * config_uart.Init.BaudRate) //5 seconds
#define UART_RX_TASK_ACTIVE_TIME  (1000000L)

static DMA_BUFFER uint8_t bufferTxConfig[SIZE_TX_BUFFER_CONFIG];
static DMA_BUFFER uint8_t bufferRxConfig[SIZE_RX_BUFFER_CONFIG];

static volatile uint32_t uartConfigActiveTime;

static const char cmdError[]="FOUT";
static const char cmdOkay[]="OK";
static const char cmdSet[]="Set+";
static const char cmdGet[]="Get+";
static const char cmdModuleInfo[]="ModuleInfo";
static const char cmdSensorInfo[]="SensorInfo";
static const char cmdJoinId[]="JoinID";
static const char cmdDeviceId[]="DeviceID";
static const char cmdAppKey[]="AppKey";
static const char cmdSensor[]="Sensor";
static const char cmdLeesInterval[]="LeesInterval";
static const char cmdMeetTijd[]="MeetTijd";
static const char cmdDataDump[]="DataDump";
static const char cmdAlwaysOn[]="AlwaysOn";
static const char cmdWissen[]="Wissen";
static const char cmdTest[]="Test";


static const char defaultProtocol1[] = "0.0";
static const char defaultProtocol2[] = "0.0";
static const char defaultVersion[] = "0.0";

typedef struct{
    const char * command;
    int commandLength;
    void (* function)(int arguments, ...);
    int numberOfArguments;
}struct_commands;

/**
 * @brief weak function getProtocol1(), needs to be override by real functions
 *
 * @return
 */
__weak const char * getProtocol1(void)
{
  return defaultProtocol1;
}

/**
 * @brief weak function getProtocol2(), needs to be override by real functions
 *
 * @return
 */
__weak const char * getProtocol2(void)
{
  return defaultProtocol2;
}

/**
 * @brief weak function getVersion(), needs to be override by real functions
 *
 * @return
 */
__weak const char * getVersion(void)
{
  return defaultVersion;
}

void sendError(int arguments, ... );
void sendModuleInfo(int arguments, ... );

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
        0,
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
        0,
        0,
    },
    //todo complete all SET commands
};

/**
 * @brief uart handler
 * is called periodically by scheduler
 */
void UartConfigHandlerProcess(void)
{
  /*
   * call test uart function
   */
  testConfigUartSend();

  /*
   * check uart needs to be active
   */
  if( uartConfigActiveTime )
  {
#ifndef NO_SLEEP
    uartConfigActiveTime--;
#endif
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
  uartConfigActiveTime = UART_RX_TASK_ACTIVE_TIME; //set activation time of task
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_UartConfig), CFG_SEQ_Prio_1); //activate task
  uartStartReceive_Config(bufferRxConfig, sizeof(bufferRxConfig), RECEIVE_TIMEOUT_COMMAND ); //start receiver to listen
}

/**
 * @brief initialization function for the uart config command handling
 *  - configure a task for the scheduler with low priority and trigger it.
 *  - register TX complete callback
 *
 */
void uartInit_Config( void )
{
  uartConfigActiveTime = UART_RX_TASK_ACTIVE_TIME;
  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_UartConfig), UTIL_SEQ_RFU, UartConfigHandlerProcess);
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_UartConfig), CFG_SEQ_Prio_1);

  HAL_UART_RegisterCallback(&config_uart, HAL_UART_TX_COMPLETE_CB_ID, uartSendReady_Config);
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

	assert_param( halRes != HAL_OK);
}

/**
 * @brief test function for uart Config functionality.
 *
 */
void testConfigUartSend(void)
{
  static int testStep = 0;

  switch (testStep)
  {

    case 0: //start receive data
      uartStartReceive_Config(bufferRxConfig, sizeof(bufferRxConfig), RECEIVE_TIMEOUT_COMMAND );
      testStep++;
      break;

    case 1: //send test data
      if (uartTxReady(&config_uart))
      {
        sendModuleInfo(0);
        testStep++;
      }
      break;

    case 2:
      // catch Receive timeout flag
      if( HAL_UART_GetError(&config_uart) & HAL_UART_ERROR_RTO )
      {
        uartStartReceive_Config(bufferRxConfig, sizeof(bufferRxConfig), 10*config_uart.Init.BaudRate);
      }
      break;

    default:
      break;

  }
}

/**
 * @brief callback function for character match to process the received data.
 *
 * @param huart current uart pointer, must be "config_uart"
 * @param nrCharsAvailable number of characters received
 */
void user_uart1_characterMatchDetect_Callback(UART_HandleTypeDef *huart, uint32_t nrCharsAvailable)
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
		for( int i = 0; i < sizeof( stCommandsGet) / sizeof(stCommandsGet[0]); i++ )
		{
		  /*
		   * check command is found, use strncasecmp() to ignore the case sensitivity
		   */
		  if( strncasecmp( (char*)bufferRxConfig + strlen(cmdSet), stCommandsGet[i].command, stCommandsGet[i].commandLength ) == 0 )
		  {
		    /*
		     * command is founded, then execute the callback fucntion.
		     */
		    char * startOffArguments = (char *)bufferRxConfig + strlen(cmdSet) + stCommandsGet[i].commandLength;

		    if( stCommandsGet[i].function != 0 ) //always check pointer is not zero before executing.
		    {
		      stCommandsGet[i].function(stCommandsGet[i].numberOfArguments, startOffArguments);
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
      if( strncasecmp( (char*)bufferRxConfig + strlen(cmdSet), stCommandsSet[i].command, stCommandsSet[i].commandLength ) == 0 )
      {
        /*
         * command is founded, then execute the callback fucntion.
         */
        char * startOffArguments = (char *)bufferRxConfig + strlen(cmdSet) + stCommandsSet[i].commandLength;
        if( stCommandsSet[i].function != 0 ) //always check pointer is not zero before executing.
        {
          stCommandsSet[i].function(stCommandsSet[i].numberOfArguments, startOffArguments);
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
	  sendError(0); //reply with error.
	}
}

/**
 * @brief send module information to config uart.
 *
 * @param arguments not used
 */
void sendModuleInfo(int arguments, ...)
{
  snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s=%s,%s,%s\r\n", cmdModuleInfo, getProtocol1(), getProtocol2(), getVersion());
  uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));
}

/**
 * @brief function to send ERROR command
 *
 * @param arguments not used
 */
void sendError(int arguments, ... )
{
  snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s\r\n", cmdError);
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
