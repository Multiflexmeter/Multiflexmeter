/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    lora_app.c
  * @author  MCD Application Team
  * @brief   Application of the LRWAN Middleware
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "platform.h"
#include "sys_app.h"
#include "lora_app.h"
#include "stm32_seq.h"
#include "stm32_timer.h"
#include "utilities_def.h"
#include "app_version.h"
#include "lorawan_version.h"
#include "subghz_phy_version.h"
#include "lora_info.h"
#include "LmHandler.h"
#include "adc_if.h"
#include "CayenneLpp.h"
#include "sys_sensors.h"
#include "flash_if.h"
#include "LoRaMac.h"

/* USER CODE BEGIN Includes */
#include "../../../App/measurement.h"
#include "../../../App/common/common.h"
#include "../../../App/FRAM/FRAM_functions.h"
#include "../../../App/IO/board_io.h"
#include "../../../App/IO/led.h"
/* USER CODE END Includes */

/* External variables ---------------------------------------------------------*/
/* USER CODE BEGIN EV */
/* USER CODE END EV */

/* Private typedef -----------------------------------------------------------*/
/**
  * @brief LoRa State Machine states
  */
typedef enum TxEventType_e
{
  /**
    * @brief Appdata Transmission issue based on timer every TxDutyCycleTime
    */
  TX_ON_TIMER,
  /**
    * @brief Appdata Transmission external event plugged on OnSendEvent( )
    */
  TX_ON_EVENT
  /* USER CODE BEGIN TxEventType_t */

  /* USER CODE END TxEventType_t */
} TxEventType_t;

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/**
  * LEDs period value of the timer in ms
  */
#define LED_PERIOD_TIME 500

/**
  * Join switch period value of the timer in ms
  */
#define JOIN_TIME 2000

/*---------------------------------------------------------------------------*/
/*                             LoRaWAN NVM configuration                     */
/*---------------------------------------------------------------------------*/
/**
  * @brief LoRaWAN NVM Flash address
  * @note last 2 sector of a 128kBytes device
  */
#define LORAWAN_NVM_BASE_ADDRESS                    ((void *)0x0803F000UL)

/* USER CODE BEGIN PD */
static uint8_t measurement[MAX_SIZE_MEASUREMENTDATA];
static const char *slotStrings[] = { "1", "2", "C", "C_MC", "P", "P_MC" };
static bool requestTime = 0;
static uint32_t nextRequestTime = 0;
static uint32_t countTimeRequestActive;
static uint32_t countTimeReceived;
static UTIL_TIMER_Time_t forcedLoraInterval;
static bool reJoinStarted = false;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  LoRa End Node send request
  */
static void SendTxData(void);

/**
  * @brief  TX timer callback function
  * @param  context ptr of timer context
  */
static void OnTxTimerEvent(void *context);

/**
  * @brief  join event callback function
  * @param  joinParams status of join
  */
static void OnJoinRequest(LmHandlerJoinParams_t *joinParams);

/**
  * @brief callback when LoRaWAN application has sent a frame
  * @brief  tx event callback function
  * @param  params status of last Tx
  */
static void OnTxData(LmHandlerTxParams_t *params);

/**
  * @brief callback when LoRaWAN application has received a frame
  * @param appData data received in the last Rx
  * @param params status of last Rx
  */
static void OnRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params);

/**
  * @brief callback when LoRaWAN Beacon status is updated
  * @param params status of Last Beacon
  */
static void OnBeaconStatusChange(LmHandlerBeaconParams_t *params);

/**
  * @brief callback when system time has been updated
  */
static void OnSysTimeUpdate(void);

/**
  * @brief callback when LoRaWAN application Class is changed
  * @param deviceClass new class
  */
static void OnClassChange(DeviceClass_t deviceClass);

/**
  * @brief  LoRa store context in Non Volatile Memory
  */
static void StoreContext(void);

/**
  * @brief  stop current LoRa execution to switch into non default Activation mode
  */
static void StopJoin(void);

/**
  * @brief  Join switch timer callback function
  * @param  context ptr of Join switch context
  */
static void OnStopJoinTimerEvent(void *context);

/**
  * @brief  Notifies the upper layer that the NVM context has changed
  * @param  state Indicates if we are storing (true) or restoring (false) the NVM context
  */
static void OnNvmDataChange(LmHandlerNvmContextStates_t state);

/**
  * @brief  Store the NVM Data context to the Flash
  * @param  nvm ptr on nvm structure
  * @param  nvm_size number of data bytes which were stored
  */
static void OnStoreContextRequest(void *nvm, uint32_t nvm_size);

/**
  * @brief  Restore the NVM Data context from the Flash
  * @param  nvm ptr on nvm structure
  * @param  nvm_size number of data bytes which were restored
  */
static void OnRestoreContextRequest(void *nvm, uint32_t nvm_size);

/**
  * Will be called each time a Radio IRQ is handled by the MAC layer
  *
  */
static void OnMacProcessNotify(void);

/**
  * @brief Change the periodicity of the uplink frames
  * @param periodicity uplink frames period in ms
  * @note Compliance test protocol callbacks
  */
static void OnTxPeriodicityChanged(uint32_t periodicity);

/**
  * @brief Change the confirmation control of the uplink frames
  * @param isTxConfirmed Indicates if the uplink requires an acknowledgement
  * @note Compliance test protocol callbacks
  */
static void OnTxFrameCtrlChanged(LmHandlerMsgTypes_t isTxConfirmed);

/**
  * @brief Change the periodicity of the ping slot frames
  * @param pingSlotPeriodicity ping slot frames period in ms
  * @note Compliance test protocol callbacks
  */
static void OnPingSlotPeriodicityChanged(uint8_t pingSlotPeriodicity);

/**
  * @brief Will be called to reset the system
  * @note Compliance test protocol callbacks
  */
static void OnSystemReset(void);

/* USER CODE BEGIN PFP */

/**
  * @brief  ReJoin switch timer callback function
  * @param  context ptr of Join switch context
  */
static void OnReJoinTimerEvent(void *context);

/**
  * @brief  stop current LoRa execution to rejoin
  */
static void ReJoin(void);

/**
  * @brief  LED Tx timer callback function
  * @param  context ptr of LED context
  */
static void OnTxTimerLedEvent(void *context);

/**
  * @brief  LED Rx timer callback function
  * @param  context ptr of LED context
  */
static void OnRxTimerLedEvent(void *context);

/**
  * @brief  LED Join timer callback function
  * @param  context ptr of LED context
  */
static void OnJoinTimerLedEvent(void *context);


/**
 * @fn const void syncRTC_withSysTime(void)
 * @brief weak function syncRTC_withSysTime(), needs to be override in application.
 *
 */
__weak const void syncRTC_withSysTime(void)
{
  return;
}

/**
 * @fn const void rxDataUsrCallback(LmHandlerAppData_t*)
 * @brief weak function for rx data received to be override in user code.
 *
 * @param appData
 */
__weak const void rxDataUsrCallback(LmHandlerAppData_t *appData)
{
  UNUSED(appData);
  __NOP();
}

/**
 * @fn const void rxDataReady(LmHandlerAppData_t*)
 * @brief weak function for signal user app that rx data is ready.
 *
 * @param appData
 */
__weak const void rxDataReady(void)
{
  __NOP();
}

/* USER CODE END PFP */

/* Private variables ---------------------------------------------------------*/
/**
  * @brief LoRaWAN default activation type
  */
static ActivationType_t ActivationType = LORAWAN_DEFAULT_ACTIVATION_TYPE;

/**
  * @brief LoRaWAN force rejoin even if the NVM context is restored
  */
static bool ForceRejoin = LORAWAN_FORCE_REJOIN_AT_BOOT;

/**
  * @brief LoRaWAN handler Callbacks
  */
static LmHandlerCallbacks_t LmHandlerCallbacks =
{
  .GetBatteryLevel =              GetBatteryLevel,
  .GetTemperature =               GetTemperatureLevel,
  .GetUniqueId =                  GetUniqueId,
  .GetDevAddr =                   GetDevAddr,
  .OnRestoreContextRequest =      OnRestoreContextRequest,
  .OnStoreContextRequest =        OnStoreContextRequest,
  .OnMacProcess =                 OnMacProcessNotify,
  .OnNvmDataChange =              OnNvmDataChange,
  .OnJoinRequest =                OnJoinRequest,
  .OnTxData =                     OnTxData,
  .OnRxData =                     OnRxData,
  .OnBeaconStatusChange =         OnBeaconStatusChange,
  .OnSysTimeUpdate =              OnSysTimeUpdate,
  .OnClassChange =                OnClassChange,
  .OnTxPeriodicityChanged =       OnTxPeriodicityChanged,
  .OnTxFrameCtrlChanged =         OnTxFrameCtrlChanged,
  .OnPingSlotPeriodicityChanged = OnPingSlotPeriodicityChanged,
  .OnSystemReset =                OnSystemReset,
};

/**
  * @brief LoRaWAN handler parameters
  */
static LmHandlerParams_t LmHandlerParams =
{
  .ActiveRegion =             ACTIVE_REGION,
  .DefaultClass =             LORAWAN_DEFAULT_CLASS,
  .AdrEnable =                LORAWAN_ADR_STATE,
  .IsTxConfirmed =            LORAWAN_DEFAULT_CONFIRMED_MSG_STATE,
  .TxDatarate =               LORAWAN_DEFAULT_DATA_RATE,
  .TxPower =                  LORAWAN_DEFAULT_TX_POWER,
  .PingSlotPeriodicity =      LORAWAN_DEFAULT_PING_SLOT_PERIODICITY,
  .RxBCTimeout =              LORAWAN_DEFAULT_CLASS_B_C_RESP_TIMEOUT
};

/**
  * @brief Type of Event to generate application Tx
  */
static TxEventType_t EventType = TX_ON_TIMER;

/**
  * @brief Timer to handle the application Tx
  */
static UTIL_TIMER_Object_t TxTimer;

/**
  * @brief Tx Timer period
  */
static UTIL_TIMER_Time_t TxPeriodicity = APP_TX_DUTYCYCLE;

/**
  * @brief Join Timer period
  */
static UTIL_TIMER_Object_t StopJoinTimer;

/* USER CODE BEGIN PV */

/**
  * @brief ReJoin Timer period
  */
static UTIL_TIMER_Object_t ReJoinTimer;

/**
  * @brief User application buffer
  */
static uint8_t AppDataBuffer[LORAWAN_APP_DATA_BUFFER_MAX_SIZE];

/**
  * @brief User application data structure
  */
static LmHandlerAppData_t AppData = { 0, 0, AppDataBuffer };

/**
  * @brief Specifies the state of the application LED
  */
static uint8_t AppLedStateOn = RESET;

/**
  * @brief Timer to handle the application Tx Led to toggle
  */
static UTIL_TIMER_Object_t TxLedTimer;

/**
  * @brief Timer to handle the application Rx Led to toggle
  */
static UTIL_TIMER_Object_t RxLedTimer;

/**
  * @brief Timer to handle the application Join Led to toggle
  */
static UTIL_TIMER_Object_t JoinLedTimer;

/* USER CODE END PV */

/* Exported functions ---------------------------------------------------------*/
/* USER CODE BEGIN EF */

/**
 * @fn const void setNewTxInterval(UTIL_TIMER_Time_t)
 * @brief function to set a new transmit interval
 *
 * @param newInterval
 */
const void setNewTxInterval(UTIL_TIMER_Time_t newInterval)
{
  OnTxPeriodicityChanged( newInterval); //new lora interval in ms
}

/**
 * @fn const UTIL_TIMER_Time_t getForcedLoraInterval(void)
 * @brief function to return the lora interval forced by the remote side.
 *
 * @return force lora interval
 */
const UTIL_TIMER_Time_t getForcedLoraInterval(void)
{
  return forcedLoraInterval;
}

/**
 * @fn const void triggerSaveNvmData2Fram(void)
 * @brief function to save NVM data to FRAM
 *
 */
const void triggerSaveNvmData2Fram(void)
{
#ifdef FRAM_USED_FOR_NVM_DATA
    UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LoRaStoreContextEvent), CFG_SEQ_Prio_0); //save lora settings to FRAM.
#endif
}

/**
 * @fn const void triggerSendTxData(void)
 * @brief function to trigger txSendData external
 *
 */
const void triggerSendTxData(void )
{
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LoRaSendOnTxTimerOrButtonEvent), CFG_SEQ_Prio_0);
}

/**
 * @fn const void triggerReJoin(void)
 * @brief function to trigger an rejoin
 *
 */
const void triggerReJoin(void)
{
  reJoinStarted = false; //reset
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LoRaReJoinEvent), CFG_SEQ_Prio_0);
}

/**
 * @fn const void setTxConfirmed(LmHandlerMsgTypes_t)
 * @brief function to change tx Confirmed
 *
 * @param isTxConfirmed \ref LmHandlerMsgTypes_t: LORAMAC_HANDLER_UNCONFIRMED_MSG or LORAMAC_HANDLER_CONFIRMED_MSG
 */
const void setTxConfirmed(LmHandlerMsgTypes_t isTxConfirmed)
{
  OnTxFrameCtrlChanged( isTxConfirmed);
}

/**
 * @fn const void setRequestTime(void)
 * @brief function to request time sync from lora portal
 *
 */
const void setRequestTime(void)
{
  requestTime = true;
}

/**
 * @fn const void setRejoinAtNextInterval(void)
 * @brief function to set a rejoin for next interval
 *
 */
const void setRejoinAtNextInterval(void)
{
  writeBackupRegister(BACKUP_REGISTER_REJOIN, true);
}

/**
 * @fn const void clearRejoinAtNextInterval(void)
 * @brief function to reset a rejoin status for next interval
 *
 */
const void clearRejoinAtNextInterval(void)
{
  writeBackupRegister(BACKUP_REGISTER_REJOIN, false);
}

/**
 * @fn const bool getRejoinAtNextInterval(void)
 * @brief get current rejoin status.
 *
 * @return
 */
const bool getRejoinAtNextInterval(void)
{
  return (readBackupRegister(BACKUP_REGISTER_REJOIN) == true);
}

/**
 * @fn void LoRaWAN_deInit(void)
 * @brief de-initialize the LoRaWAN functionality
 *
 */
void LoRaWAN_deInit(void)
{
  LoRaMacStop();
  LmHandlerDeInit();
  UTIL_TIMER_Stop(&TxTimer);
  UTIL_TIMER_Stop(&ReJoinTimer);
  UTIL_TIMER_Stop(&StopJoinTimer);
  UTIL_TIMER_Stop(&JoinLedTimer);
  UTIL_TIMER_Stop(&RxLedTimer);
  UTIL_TIMER_Stop(&TxLedTimer);
}

/* USER CODE END EF */

void LoRaWAN_Init(void)
{
  /* USER CODE BEGIN LoRaWAN_Init_LV */
  uint32_t feature_version = 0UL;
  /* USER CODE END LoRaWAN_Init_LV */

  /* USER CODE BEGIN LoRaWAN_Init_1 */

  /* Get LoRaWAN APP version*/
  APP_LOG(TS_OFF, VLEVEL_M, "APPLICATION_VERSION: V%X.%X.%X\r\n",
          (uint8_t)(APP_VERSION_MAIN),
          (uint8_t)(APP_VERSION_SUB1),
          (uint8_t)(APP_VERSION_SUB2));

  /* Get MW LoRaWAN info */
  APP_LOG(TS_OFF, VLEVEL_M, "MW_LORAWAN_VERSION:  V%X.%X.%X\r\n",
          (uint8_t)(LORAWAN_VERSION_MAIN),
          (uint8_t)(LORAWAN_VERSION_SUB1),
          (uint8_t)(LORAWAN_VERSION_SUB2));

  /* Get MW SubGhz_Phy info */
  APP_LOG(TS_OFF, VLEVEL_M, "MW_RADIO_VERSION:    V%X.%X.%X\r\n",
          (uint8_t)(SUBGHZ_PHY_VERSION_MAIN),
          (uint8_t)(SUBGHZ_PHY_VERSION_SUB1),
          (uint8_t)(SUBGHZ_PHY_VERSION_SUB2));

  /* Get LoRaWAN Link Layer info */
  LmHandlerGetVersion(LORAMAC_HANDLER_L2_VERSION, &feature_version);
  APP_LOG(TS_OFF, VLEVEL_M, "L2_SPEC_VERSION:     V%X.%X.%X\r\n",
          (uint8_t)(feature_version >> 24),
          (uint8_t)(feature_version >> 16),
          (uint8_t)(feature_version >> 8));

  /* Get LoRaWAN Regional Parameters info */
  LmHandlerGetVersion(LORAMAC_HANDLER_REGION_VERSION, &feature_version);
  APP_LOG(TS_OFF, VLEVEL_M, "RP_SPEC_VERSION:     V%X-%X.%X.%X\r\n",
          (uint8_t)(feature_version >> 24),
          (uint8_t)(feature_version >> 16),
          (uint8_t)(feature_version >> 8),
          (uint8_t)(feature_version));

  UTIL_TIMER_Create(&TxLedTimer, LED_PERIOD_TIME, UTIL_TIMER_ONESHOT, OnTxTimerLedEvent, NULL);
  UTIL_TIMER_Create(&RxLedTimer, LED_PERIOD_TIME, UTIL_TIMER_ONESHOT, OnRxTimerLedEvent, NULL);
  UTIL_TIMER_Create(&JoinLedTimer, LED_PERIOD_TIME, UTIL_TIMER_PERIODIC, OnJoinTimerLedEvent, NULL);

  if (FLASH_IF_Init(NULL) != FLASH_IF_OK)
  {
    Error_Handler();
  }

  /* USER CODE END LoRaWAN_Init_1 */

  UTIL_TIMER_Create(&StopJoinTimer, JOIN_TIME, UTIL_TIMER_ONESHOT, OnStopJoinTimerEvent, NULL);

  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_LmHandlerProcess), UTIL_SEQ_RFU, LmHandlerProcess);

  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_LoRaSendOnTxTimerOrButtonEvent), UTIL_SEQ_RFU, SendTxData);
  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_LoRaStoreContextEvent), UTIL_SEQ_RFU, StoreContext);
  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_LoRaStopJoinEvent), UTIL_SEQ_RFU, StopJoin);

  /* Init Info table used by LmHandler*/
  LoraInfo_Init();

  /* Init the Lora Stack*/
  LmHandlerInit(&LmHandlerCallbacks, APP_VERSION);

  LmHandlerConfigure(&LmHandlerParams);

  /* USER CODE BEGIN LoRaWAN_Init_2 */
  UTIL_TIMER_Create(&ReJoinTimer, JOIN_TIME, UTIL_TIMER_ONESHOT, OnReJoinTimerEvent, NULL);

  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_LoRaReJoinEvent), UTIL_SEQ_RFU, ReJoin);

  UTIL_TIMER_Start(&JoinLedTimer);

  /* get current join status */
  LmHandlerFlagStatus_t joinStatus = LmHandlerJoinStatus();
  APP_LOG(TS_OFF, VLEVEL_M, "JOIN = %s\r\n", joinStatus == LORAMAC_HANDLER_SET ? "TRUE" : "FALSE" );

  /* get rejoin status flag */
  ForceRejoin = getRejoinAtNextInterval(); //get setting for forced rejoin.

  if( ForceRejoin == true )
  {
    APP_LOG(TS_OFF, VLEVEL_M, "Force REJOIN\r\n");
    // if rejoin is active, the reset current join status.
    joinStatus = LORAMAC_HANDLER_RESET; //this forces the DR set back to 2.
  }

  /* get current ADR setting */
  bool setting_AdrEnabled;
  if( LmHandlerGetAdrEnable(&setting_AdrEnabled) == LORAMAC_HANDLER_SUCCESS )
  {
    APP_LOG(TS_OFF, VLEVEL_M, "ADR = %s\r\n", setting_AdrEnabled ? "TRUE" : "FALSE" );
  }
  else
  {
    APP_LOG(TS_OFF, VLEVEL_M, "ADR = failed\r\n" );
  }

  /* set enable ADR based on join status */
  bool enableAdr = joinStatus == LORAMAC_HANDLER_SET ? true : false;
  if( LmHandlerSetAdrEnable(enableAdr) == LORAMAC_HANDLER_ERROR )
  {
    APP_LOG(TS_OFF, VLEVEL_H, "###### FAIL init adrEnable\r\n");
  }
  else
  {
    APP_LOG(TS_OFF, VLEVEL_M, "set ADR is %s\r\n", enableAdr ? "TRUE" : "FALSE" );
  }

  /* set DR_2 only if join is not active or forced rejoin active */
  if( joinStatus == LORAMAC_HANDLER_RESET )
  {
    if( LmHandlerSetTxDatarate(DR_2) == LORAMAC_HANDLER_ERROR )
    {
      APP_LOG(TS_OFF, VLEVEL_H, "###### FAIL init datarate\r\n");
    }
    else
    {
      APP_LOG(TS_OFF, VLEVEL_M, "set Tx Datarate to DR_2\r\n" );
    }
  }

  nextRequestTime = readBackupRegister( BACKUP_REGISTER_LAST_TIME_SYNC ); //get value from backup register

  /* USER CODE END LoRaWAN_Init_2 */

  LmHandlerJoin(ActivationType, ForceRejoin);

  if (EventType == TX_ON_TIMER)
  {
    /* send every time timer elapses */
    UTIL_TIMER_Create(&TxTimer, TxPeriodicity, UTIL_TIMER_ONESHOT, OnTxTimerEvent, NULL);
    UTIL_TIMER_Start(&TxTimer);
  }
  else
  {
    /* USER CODE BEGIN LoRaWAN_Init_3 */

    /* USER CODE END LoRaWAN_Init_3 */
  }

  /* USER CODE BEGIN LoRaWAN_Init_Last */

  /* USER CODE END LoRaWAN_Init_Last */
}

/* USER CODE BEGIN PB_Callbacks */

#if 0 /* User should remove the #if 0 statement and adapt the below code according with his needs*/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  switch (GPIO_Pin)
  {
    case  BUT1_Pin:
      /* Note: when "EventType == TX_ON_TIMER" this GPIO is not initialized */
      if (EventType == TX_ON_EVENT)
      {
        UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LoRaSendOnTxTimerOrButtonEvent), CFG_SEQ_Prio_0);
      }
      break;
    case  BUT2_Pin:
      UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LoRaStopJoinEvent), CFG_SEQ_Prio_0);
      break;
    case  BUT3_Pin:
      UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LoRaStoreContextEvent), CFG_SEQ_Prio_0);
      break;
    default:
      break;
  }
}
#endif

/* USER CODE END PB_Callbacks */

/* Private functions ---------------------------------------------------------*/
/* USER CODE BEGIN PrFD */

/**
 * @fn const uint8_t getBufferSize(void)
 * @brief weak function to get maximum buffer side.
 *
 * @return
 */
__weak const uint8_t getBufferSize(void)
{
  return 50;
}

/**
 * @fn const void setNewTxInterval_usr(void)
 * @brief weak function to be override in user application
 *
 */
__weak const void setNewTxInterval_usr(LmHandlerErrorStatus_t status)
{
  UNUSED(status); //not used in weak
  UTIL_TIMER_Time_t newInterval = MAX(forcedLoraInterval, TxPeriodicity);
  setNewTxInterval( newInterval); //new lora interval in ms
}

static void ReJoin(void)
{
  UTIL_TIMER_Stop(&TxTimer);

  if (LORAMAC_HANDLER_SUCCESS != LmHandlerStop())
  {
    APP_LOG(TS_OFF, VLEVEL_M, "LmHandler Stop on going ...\r\n");
  }
  else
  {
    APP_LOG(TS_OFF, VLEVEL_M, "LmHandler Stopped\r\n");

    ActivationType = LORAWAN_DEFAULT_ACTIVATION_TYPE;

    LmHandlerConfigure(&LmHandlerParams);
    LmHandlerJoin(ActivationType, true);
    reJoinStarted = true;

    APP_LOG(TS_OFF, VLEVEL_M, "LmHandler Start join\r\n");

    UTIL_TIMER_Start(&TxTimer);
  }
  UTIL_TIMER_Start(&ReJoinTimer);
  /* USER CODE BEGIN StopJoin_Last */

  /* USER CODE END StopJoin_Last */
}

static void OnReJoinTimerEvent(void *context)
{
  if (reJoinStarted == false)
  {
    UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LoRaReJoinEvent), CFG_SEQ_Prio_0);
  }
}

/* USER CODE END PrFD */

static void OnRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params)
{
  /* USER CODE BEGIN OnRxData_1 */
  uint8_t RxPort = 0;

  if (params != NULL)
  {
#ifdef USE_LORA_APP_LED1
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET); /* LED_BLUE */
#endif

    UTIL_TIMER_Start(&RxLedTimer);

    if (params->IsMcpsIndication)
    {
      if (appData != NULL)
      {
        RxPort = appData->Port;
        if (appData->Buffer != NULL)
        {
          switch (appData->Port)
          {
            case LORAWAN_SWITCH_CLASS_PORT:
              /*this port switches the class*/
              if (appData->BufferSize == 1)
              {
                switch (appData->Buffer[0])
                {
                  case 0:
                  {
                    LmHandlerRequestClass(CLASS_A);
                    break;
                  }
                  case 1:
                  {
                    LmHandlerRequestClass(CLASS_B);
                    break;
                  }
                  case 2:
                  {
                    LmHandlerRequestClass(CLASS_C);
                    break;
                  }
                  default:
                    break;
                }
              }
              break;
            case LORAWAN_USER_APP_PORT:
              if (appData->BufferSize == 1)
              {
                AppLedStateOn = appData->Buffer[0] & 0x01;
                if (AppLedStateOn == RESET)
                {
                  APP_LOG(TS_OFF, VLEVEL_H, "LED OFF\r\n");
                  setGreenLedOnOf(false);
#ifdef USE_LORA_APP_LED3
                  HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET); /* LED_RED */
#endif
                }
                else
                {
                  APP_LOG(TS_OFF, VLEVEL_H, "LED ON\r\n");
                  setGreenLedOnOf(true);
#ifdef USE_LORA_APP_LED3
                  HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET); /* LED_RED */
#endif
                }
              }
              break;

            default:

              rxDataUsrCallback( appData );

              break;
          }
        }
      }
    }
    if (params->RxSlot < RX_SLOT_NONE)
    {
      APP_LOG(TS_OFF, VLEVEL_H, "###### D/L FRAME:%04d | PORT:%d | DR:%d | SLOT:%s | RSSI:%d | SNR:%d\r\n",
              params->DownlinkCounter, RxPort, params->Datarate, slotStrings[params->RxSlot],
              params->Rssi, params->Snr);
    }

    rxDataReady(); //signal to mainTask rx is ready
  }
  /* USER CODE END OnRxData_1 */
}

static void SendTxData(void)
{
  /* USER CODE BEGIN SendTxData_1 */
  LmHandlerErrorStatus_t status = LORAMAC_HANDLER_ERROR;
  UTIL_TIMER_Time_t nextTxIn = 0;
  STRUCT_measurementData *measurementData = (STRUCT_measurementData *)&measurement[0];

  if (LmHandlerIsBusy() == false)
  {
    uint32_t i = 0;

    AppData.Port = LORAWAN_USER_APP_PORT;

    /* read latest measurement data */
    readMeasurement(getLatestMeasurementId() > 0 ? getLatestMeasurementId() - 1 : 0, measurement, sizeof(measurement));

    /* get sensor module data size */
    uint8_t sensorDataSize = measurementData->sensorModuleData.sensorModuleDataSize;

    /* check if size is within limit of 36 bytes */
    if( sensorDataSize >= sizeof(measurementData->sensorModuleData) )
    {
      sensorDataSize = sizeof(measurementData->sensorModuleData); //Maximize on 36 bytes
    }

    /* fill in measurement data */
    AppData.Buffer[i++] = measurementData->protocolMFM; //protocol MFM
    AppData.Buffer[i++] = measurementData->sensorModuleData.sensorModuleSlotId;
    AppData.Buffer[i++] = measurementData->sensorModuleData.sensorModuleTypeId;
    AppData.Buffer[i++] = measurementData->sensorModuleData.sensorModuleProtocolId;
    AppData.Buffer[i++] = sensorDataSize;

    /* copy sensordata max 36 bytes */
    memcpy(&AppData.Buffer[i],measurementData->sensorModuleData.sensorModuleData, sensorDataSize );
    i+=sensorDataSize;

    /* Base data, depends on messageType */
    AppData.Buffer[i++] = measurementData->MFM_baseData.stBaseData.messageType;
    switch( measurementData->MFM_baseData.stBaseData.messageType )
    {
      case 0x00:
        //nothing
        break;

      case 0x01:
        AppData.Buffer[i++] = measurementData->MFM_baseData.stBaseData.batteryStateEos;
        AppData.Buffer[i++] = measurementData->MFM_baseData.stBaseData.temperatureGauge;
        AppData.Buffer[i++] = measurementData->MFM_baseData.stBaseData.temperatureController;
        AppData.Buffer[i++] = measurementData->MFM_baseData.stBaseData.diagnosticBits;
        break;

      case 0x02:
        AppData.Buffer[i++] = measurementData->MFM_baseData.stBaseData.temperatureController;
        AppData.Buffer[i++] = measurementData->MFM_baseData.stBaseData.diagnosticBits;
        break;

      default:
        //nothing
        break;

    }

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
    //testcode to fill databuffer until 50 bytes.
    while(i<getBufferSize())
    {
      AppData.Buffer[i++] = 0xAA;
    }
//////////////////////////////////////////////////////////////////
/// //////////////////////////////////////////////////////////////////

    AppData.BufferSize = i;

    /* verify data fitted in buffer */
    assert_param(AppData.BufferSize <= sizeof(AppDataBuffer));

    if( AppData.BufferSize > sizeof(AppDataBuffer) )
    {
      APP_LOG(TS_ON, VLEVEL_L, "SENDTXDATA: message larger then buffer\r\n");
    }

    if ((JoinLedTimer.IsRunning) && (LmHandlerJoinStatus() == LORAMAC_HANDLER_SET))
    {
      UTIL_TIMER_Stop(&JoinLedTimer);
#ifdef USE_LORA_APP_LED3
      HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET); /* LED_RED */
#endif
    }

    //Sync every day the time with the server time
    if( nextRequestTime < SysTimeGet().Seconds )
    {
      nextRequestTime = SysTimeGet().Seconds + TM_SECONDS_IN_1DAY;
      writeBackupRegister(BACKUP_REGISTER_LAST_TIME_SYNC, nextRequestTime);//save in backup register

      requestTime = true;
    }

    //check the requestTime is true
    if( requestTime == true )
    {
      countTimeRequestActive++;
      LmHandlerDeviceTimeReq(); //request the time
    }

    status = LmHandlerSend(&AppData, LmHandlerParams.IsTxConfirmed, false);
    if (LORAMAC_HANDLER_SUCCESS == status)
    {
      APP_LOG(TS_ON, VLEVEL_L, "SEND REQUEST\r\n");
    }
    else if (LORAMAC_HANDLER_DUTYCYCLE_RESTRICTED == status)
    {
      nextTxIn = LmHandlerGetDutyCycleWaitTime();
      if (nextTxIn > 0)
      {
        APP_LOG(TS_ON, VLEVEL_L, "Next Tx in  : ~%d second(s)\r\n", (nextTxIn / 1000));
      }
    }

    OnTxPeriodicityChanged(nextTxIn > TxPeriodicity ? nextTxIn : TxPeriodicity); //timer only actively used without mainTask.c
    forcedLoraInterval = nextTxIn; //save for request by mainTask.c
  }

  if (EventType == TX_ON_TIMER)
  {
    setNewTxInterval_usr(status); //set new timer or callback to maintask
  }

  /* USER CODE END SendTxData_1 */
}

static void OnTxTimerEvent(void *context)
{
  /* USER CODE BEGIN OnTxTimerEvent_1 */
  triggerSaveNvmData2Fram();
  /* USER CODE END OnTxTimerEvent_1 */
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LoRaSendOnTxTimerOrButtonEvent), CFG_SEQ_Prio_0);

  /*Wait for next tx slot*/
  UTIL_TIMER_Start(&TxTimer);
  /* USER CODE BEGIN OnTxTimerEvent_2 */

  /* USER CODE END OnTxTimerEvent_2 */
}

/* USER CODE BEGIN PrFD_LedEvents */
static void OnTxTimerLedEvent(void *context)
{
#ifdef USE_LORA_APP_LED2
  HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET); /* LED_GREEN */
#endif
}

static void OnRxTimerLedEvent(void *context)
{
#ifdef USE_LORA_APP_LED1
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET); /* LED_BLUE */
#endif
}

static void OnJoinTimerLedEvent(void *context)
{
#ifdef USE_LORA_APP_LED3
  HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin); /* LED_RED */
#endif
}

/* USER CODE END PrFD_LedEvents */

static void OnTxData(LmHandlerTxParams_t *params)
{
  /* USER CODE BEGIN OnTxData_1 */
  if ((params != NULL))
  {
    /* Process Tx event only if its a mcps response to prevent some internal events (mlme) */
    if (params->IsMcpsConfirm != 0)
    {
#ifdef USE_LORA_APP_LED2
      HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET); /* LED_GREEN */
#endif
      UTIL_TIMER_Start(&TxLedTimer);

      APP_LOG(TS_OFF, VLEVEL_M, "\r\n###### ========== MCPS-Confirm =============\r\n");
      APP_LOG(TS_OFF, VLEVEL_H, "###### U/L FRAME:%04d | PORT:%d | DR:%d | PWR:%d", params->UplinkCounter,
              params->AppData.Port, params->Datarate, params->TxPower);

      APP_LOG(TS_OFF, VLEVEL_H, " | MSG TYPE:");
      if (params->MsgType == LORAMAC_HANDLER_CONFIRMED_MSG)
      {
        APP_LOG(TS_OFF, VLEVEL_H, "CONFIRMED [%s]\r\n", (params->AckReceived != 0) ? "ACK" : "NACK");
      }
      else
      {
        APP_LOG(TS_OFF, VLEVEL_H, "UNCONFIRMED\r\n");
      }
      triggerSaveNvmData2Fram();
    }
  }

  /* USER CODE END OnTxData_1 */
}

static void OnJoinRequest(LmHandlerJoinParams_t *joinParams)
{
  /* USER CODE BEGIN OnJoinRequest_1 */
  if (joinParams != NULL)
  {
    triggerSaveNvmData2Fram();

    if (joinParams->Status == LORAMAC_HANDLER_SUCCESS)
    {
      UTIL_TIMER_Stop(&JoinLedTimer);
#ifdef USE_LORA_APP_LED3
      HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET); /* LED_RED */
#endif

      APP_LOG(TS_OFF, VLEVEL_M, "\r\n###### = JOINED = ");
      if (joinParams->Mode == ACTIVATION_TYPE_ABP)
      {
        APP_LOG(TS_OFF, VLEVEL_M, "ABP ======================\r\n");
      }
      else
      {
        APP_LOG(TS_OFF, VLEVEL_M, "OTAA =====================\r\n");
      }

      //always enable ADR after join
      LmHandlerSetAdrEnable(true);

    }
    else
    {
      bool currentAdrEnable;
      APP_LOG(TS_OFF, VLEVEL_M, "\r\n###### = JOIN FAILED\r\n");

      //read current adrEnable state, if true then disable it.
      if( LmHandlerGetAdrEnable(&currentAdrEnable) == LORAMAC_HANDLER_SUCCESS )
      {
        if( currentAdrEnable == true ) //check if ADR is enabled
        {
          LmHandlerSetAdrEnable(false); //disable ADR
        }
      }

      // check datarate, then lower the datarate for next attempt.
      if (joinParams->Datarate)
      {
        if( LmHandlerSetTxDatarate(joinParams->Datarate - 1) == LORAMAC_HANDLER_ERROR)
        {
          APP_LOG(TS_OFF, VLEVEL_H, "###### FAIL increase datarate\r\n");
        }
        else
        {
          APP_LOG(TS_OFF, VLEVEL_H, "###### OKAY datarate increased\r\n");
        }
      }

      // check power, increase next attempt. Lower "TxPower" is higher power.
      if (joinParams->TxPower)
      {
        if( LmHandlerSetTxPower(joinParams->TxPower - 1) == LORAMAC_HANDLER_ERROR)
        {
          APP_LOG(TS_OFF, VLEVEL_H, "###### FAIL increase power\r\n");
        }
        else
        {
          APP_LOG(TS_OFF, VLEVEL_H, "###### OKAY power increased\r\n");
        }
      }

    }

    APP_LOG(TS_OFF, VLEVEL_H, "###### U/L FRAME:JOIN | DR:%d | PWR:%d\r\n", joinParams->Datarate, joinParams->TxPower);
  }
  /* USER CODE END OnJoinRequest_1 */
}

static void OnBeaconStatusChange(LmHandlerBeaconParams_t *params)
{
  /* USER CODE BEGIN OnBeaconStatusChange_1 */
  if (params != NULL)
  {
    switch (params->State)
    {
      default:
      case LORAMAC_HANDLER_BEACON_LOST:
      {
        APP_LOG(TS_OFF, VLEVEL_M, "\r\n###### BEACON LOST\r\n");
        break;
      }
      case LORAMAC_HANDLER_BEACON_RX:
      {
        APP_LOG(TS_OFF, VLEVEL_M,
                "\r\n###### BEACON RECEIVED | DR:%d | RSSI:%d | SNR:%d | FQ:%d | TIME:%d | DESC:%d | "
                "INFO:02X%02X%02X %02X%02X%02X\r\n",
                params->Info.Datarate, params->Info.Rssi, params->Info.Snr, params->Info.Frequency,
                params->Info.Time.Seconds, params->Info.GwSpecific.InfoDesc,
                params->Info.GwSpecific.Info[0], params->Info.GwSpecific.Info[1],
                params->Info.GwSpecific.Info[2], params->Info.GwSpecific.Info[3],
                params->Info.GwSpecific.Info[4], params->Info.GwSpecific.Info[5]);
        break;
      }
      case LORAMAC_HANDLER_BEACON_NRX:
      {
        APP_LOG(TS_OFF, VLEVEL_M, "\r\n###### BEACON NOT RECEIVED\r\n");
        break;
      }
    }
  }
  /* USER CODE END OnBeaconStatusChange_1 */
}

static void OnSysTimeUpdate(void)
{
  /* USER CODE BEGIN OnSysTimeUpdate_1 */
  requestTime = false; //set false when time message is received
  countTimeReceived++;
  nextRequestTime = SysTimeGet().Seconds + TM_SECONDS_IN_1DAY;

  syncRTC_withSysTime();


  /* USER CODE END OnSysTimeUpdate_1 */
}

static void OnClassChange(DeviceClass_t deviceClass)
{
  /* USER CODE BEGIN OnClassChange_1 */
  APP_LOG(TS_OFF, VLEVEL_M, "Switch to Class %c done\r\n", "ABC"[deviceClass]);
  /* USER CODE END OnClassChange_1 */
}

static void OnMacProcessNotify(void)
{
  /* USER CODE BEGIN OnMacProcessNotify_1 */

  /* USER CODE END OnMacProcessNotify_1 */
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LmHandlerProcess), CFG_SEQ_Prio_0);

  /* USER CODE BEGIN OnMacProcessNotify_2 */

  /* USER CODE END OnMacProcessNotify_2 */
}

static void OnTxPeriodicityChanged(uint32_t periodicity)
{
  /* USER CODE BEGIN OnTxPeriodicityChanged_1 */

  /* USER CODE END OnTxPeriodicityChanged_1 */
  TxPeriodicity = periodicity;

  if (TxPeriodicity == 0)
  {
    /* Revert to application default periodicity */
    TxPeriodicity = APP_TX_DUTYCYCLE;
  }

  /* Update timer periodicity */
  UTIL_TIMER_Stop(&TxTimer);
  UTIL_TIMER_SetPeriod(&TxTimer, TxPeriodicity);
  UTIL_TIMER_Start(&TxTimer);
  /* USER CODE BEGIN OnTxPeriodicityChanged_2 */

  /* USER CODE END OnTxPeriodicityChanged_2 */
}

static void OnTxFrameCtrlChanged(LmHandlerMsgTypes_t isTxConfirmed)
{
  /* USER CODE BEGIN OnTxFrameCtrlChanged_1 */

  /* USER CODE END OnTxFrameCtrlChanged_1 */
  LmHandlerParams.IsTxConfirmed = isTxConfirmed;
  /* USER CODE BEGIN OnTxFrameCtrlChanged_2 */

  /* USER CODE END OnTxFrameCtrlChanged_2 */
}

static void OnPingSlotPeriodicityChanged(uint8_t pingSlotPeriodicity)
{
  /* USER CODE BEGIN OnPingSlotPeriodicityChanged_1 */

  /* USER CODE END OnPingSlotPeriodicityChanged_1 */
  LmHandlerParams.PingSlotPeriodicity = pingSlotPeriodicity;
  /* USER CODE BEGIN OnPingSlotPeriodicityChanged_2 */

  /* USER CODE END OnPingSlotPeriodicityChanged_2 */
}

static void OnSystemReset(void)
{
  /* USER CODE BEGIN OnSystemReset_1 */

  /* USER CODE END OnSystemReset_1 */
  if ((LORAMAC_HANDLER_SUCCESS == LmHandlerHalt()) && (LmHandlerJoinStatus() == LORAMAC_HANDLER_SET))
  {
    NVIC_SystemReset();
  }
  /* USER CODE BEGIN OnSystemReset_Last */

  /* USER CODE END OnSystemReset_Last */
}

static void StopJoin(void)
{
  /* USER CODE BEGIN StopJoin_1 */
#ifdef USE_LORA_APP_LED1
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET); /* LED_BLUE */
#endif
#ifdef USE_LORA_APP_LED2
  HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET); /* LED_GREEN */
#endif
#ifdef USE_LORA_APP_LED3
  HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET); /* LED_RED */
#endif
  /* USER CODE END StopJoin_1 */

  UTIL_TIMER_Stop(&TxTimer);

  if (LORAMAC_HANDLER_SUCCESS != LmHandlerStop())
  {
    APP_LOG(TS_OFF, VLEVEL_M, "LmHandler Stop on going ...\r\n");
  }
  else
  {
    APP_LOG(TS_OFF, VLEVEL_M, "LmHandler Stopped\r\n");
    if (LORAWAN_DEFAULT_ACTIVATION_TYPE == ACTIVATION_TYPE_ABP)
    {
      ActivationType = ACTIVATION_TYPE_OTAA;
      APP_LOG(TS_OFF, VLEVEL_M, "LmHandler switch to OTAA mode\r\n");
    }
    else
    {
      ActivationType = ACTIVATION_TYPE_ABP;
      APP_LOG(TS_OFF, VLEVEL_M, "LmHandler switch to ABP mode\r\n");
    }
    LmHandlerConfigure(&LmHandlerParams);
    LmHandlerJoin(ActivationType, true);
    UTIL_TIMER_Start(&TxTimer);
  }
  UTIL_TIMER_Start(&StopJoinTimer);
  /* USER CODE BEGIN StopJoin_Last */

  /* USER CODE END StopJoin_Last */
}

static void OnStopJoinTimerEvent(void *context)
{
  /* USER CODE BEGIN OnStopJoinTimerEvent_1 */

  /* USER CODE END OnStopJoinTimerEvent_1 */
  if (ActivationType == LORAWAN_DEFAULT_ACTIVATION_TYPE)
  {
    UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LoRaStopJoinEvent), CFG_SEQ_Prio_0);
  }
  /* USER CODE BEGIN OnStopJoinTimerEvent_Last */
#ifdef USE_LORA_APP_LED1
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET); /* LED_BLUE */
#endif
#ifdef USE_LORA_APP_LED2
  HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET); /* LED_GREEN */
#endif
#ifdef USE_LORA_APP_LED3
  HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET); /* LED_RED */
#endif
  /* USER CODE END OnStopJoinTimerEvent_Last */
}

static void StoreContext(void)
{
  LmHandlerErrorStatus_t status = LORAMAC_HANDLER_ERROR;

  /* USER CODE BEGIN StoreContext_1 */

  /* USER CODE END StoreContext_1 */
  status = LmHandlerNvmDataStore();

  if (status == LORAMAC_HANDLER_NVM_DATA_UP_TO_DATE)
  {
    APP_LOG(TS_OFF, VLEVEL_M, "NVM DATA UP TO DATE\r\n");
  }
  else if (status == LORAMAC_HANDLER_ERROR)
  {
    APP_LOG(TS_OFF, VLEVEL_M, "NVM DATA STORE FAILED\r\n");
  }
  /* USER CODE BEGIN StoreContext_Last */

  /* USER CODE END StoreContext_Last */
}

static void OnNvmDataChange(LmHandlerNvmContextStates_t state)
{
  /* USER CODE BEGIN OnNvmDataChange_1 */

  /* USER CODE END OnNvmDataChange_1 */
  if (state == LORAMAC_HANDLER_NVM_STORE)
  {
    APP_LOG(TS_OFF, VLEVEL_M, "NVM DATA STORED\r\n");
  }
  else
  {
    APP_LOG(TS_OFF, VLEVEL_M, "NVM DATA RESTORED\r\n");
  }
  /* USER CODE BEGIN OnNvmDataChange_Last */

  /* USER CODE END OnNvmDataChange_Last */
}

static void OnStoreContextRequest(void *nvm, uint32_t nvm_size)
{
  /* USER CODE BEGIN OnStoreContextRequest_1 */

#ifdef FRAM_USED_FOR_NVM_DATA
  //save data to FRAM
  static_assert (sizeof(LoRaMacNvmData_t) <= MAX_SIZE_LORA_SETTINGS, "Size LoRaMacNvmData_t is too large for reserved FRAM memory");
  saveLoraSettings((const void *)nvm, nvm_size);
  return; //prevent to execute write in internal flash, cycles of 10k too less
#endif

  /* USER CODE END OnStoreContextRequest_1 */
  /* store nvm in flash */
  if (FLASH_IF_Erase(LORAWAN_NVM_BASE_ADDRESS, FLASH_PAGE_SIZE) == FLASH_IF_OK)
  {
    FLASH_IF_Write(LORAWAN_NVM_BASE_ADDRESS, (const void *)nvm, nvm_size);
  }
  /* USER CODE BEGIN OnStoreContextRequest_Last */

  /* USER CODE END OnStoreContextRequest_Last */
}

static void OnRestoreContextRequest(void *nvm, uint32_t nvm_size)
{
  /* USER CODE BEGIN OnRestoreContextRequest_1 */
#ifdef FRAM_USED_FOR_NVM_DATA

#ifdef ERASE_FRAM_NVM
#warning NVM data is not read at powerup, equal to erasing the NVM data
  //do not read, just return emty array
  memset(nvm, 0x00, nvm_size);
  return; //prevent to execute read from internal flash
#else

  //read data to FRAM
  restoreLoraSettings((const void *)nvm, nvm_size);
  return; //prevent to execute read from internal flash, cycles of 10k too less
#endif
#endif
  /* USER CODE END OnRestoreContextRequest_1 */
  FLASH_IF_Read(nvm, LORAWAN_NVM_BASE_ADDRESS, nvm_size);
  /* USER CODE BEGIN OnRestoreContextRequest_Last */

  /* USER CODE END OnRestoreContextRequest_Last */
}

