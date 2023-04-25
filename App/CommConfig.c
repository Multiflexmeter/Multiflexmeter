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
#include "common/app_types.h"
#include "common/uart.h"
#include "CommConfig.h"


static DMA_BUFFER uint8_t bufferTxConfig[SIZE_TX_BUFFER_CONFIG];
static DMA_BUFFER uint8_t bufferRxConfig[SIZE_RX_BUFFER_CONFIG];

static const uint8_t cmdSet[]="Set+";
static const uint8_t cmdGet[]="Get+";
static const uint8_t cmdModuleInfo[]="ModuleInfo";
static const uint8_t cmdSensorInfo[]="SensorInfo";
static const uint8_t cmdJoinId[]="JoinID";
static const uint8_t cmdDeviceId[]="DeviceID";
static const uint8_t cmdAppKey[]="AppKey";
static const uint8_t cmdSensor[]="Sensor";
static const uint8_t cmdLeesInterval[]="LeesInterval";
static const uint8_t cmdMeetTijd[]="MeetTijd";
static const uint8_t cmdDataDump[]="DataDump";
static const uint8_t cmdAlwaysOn[]="AlwaysOn";
static const uint8_t cmdWissen[]="Wissen";
static const uint8_t cmdTest[]="Test";


HAL_StatusTypeDef uartSend_Config( const uint8_t *pData, uint16_t Size)
{
	return uartSend(&config_uart, pData, Size);
}

void testConfigUartSend(void)
{
	config_uart.gState;
	if( uartTxReady(&config_uart) )
	{
		snprintf((char*)bufferTxConfig, sizeof(bufferTxConfig), "%s%s\r\n", cmdSet, cmdModuleInfo);
		uartSend_Config(bufferTxConfig, strlen((char*)bufferTxConfig));
	}
}


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
