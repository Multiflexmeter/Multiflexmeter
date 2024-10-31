/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    subghz_phy_app.c
  * @author  MCD Application Team
  * @brief   Application of the SubGHz_Phy Middleware
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
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
#include "subghz_phy_app.h"
#include "radio.h"

/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "stm32_timer.h"
#include "stm32_seq.h"
#include "utilities_def.h"
#include "app_version.h"
#include "subghz_phy_version.h"
/* USER CODE END Includes */

/* External variables ---------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* Configurations */
/*Timeout*/
#define RX_TIMEOUT_VALUE              3000
#define TX_TIMEOUT_VALUE              3000
/* PING string*/
#define PING "PING"
/* PONG string*/
#define PONG "PONG"
/*Size of the payload to be sent*/
/* Size must be greater of equal the PING and PONG*/
#define MAX_APP_BUFFER_SIZE          255
#if (PAYLOAD_LEN > MAX_APP_BUFFER_SIZE)
#error PAYLOAD_LEN must be less or equal than MAX_APP_BUFFER_SIZE
#endif /* (PAYLOAD_LEN > MAX_APP_BUFFER_SIZE) */
/* wait for remote to be in Rx, before sending a Tx frame*/
#define RX_TIME_MARGIN                1000
/* Afc bandwidth in Hz */
#define FSK_AFC_BANDWIDTH             83333
/* LED blink Period*/
#define LED_PERIOD_MS                 200

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* Radio events function pointer */
static RadioEvents_t RadioEvents;

/* USER CODE BEGIN PV */
/*Ping Pong FSM states */
static States_t State = TX;
static bool rxReady = false;
static bool txReady = false;
static bool txTimeout = false;
static bool rxTimeout = false;
static bool rxError = false;
/* App Rx Buffer*/
static uint8_t BufferRx[MAX_APP_BUFFER_SIZE];
/* App Tx Buffer*/
static uint8_t BufferTx[MAX_APP_BUFFER_SIZE];
/* Last  Received Buffer Size*/
uint16_t RxBufferSize = 0;
/* Last  Received packer Rssi*/
int8_t RssiValue = 0;
/* Last  Received packer SNR (in Lora modulation)*/
int8_t SnrValue = 0;
/* Led Timers objects*/
static UTIL_TIMER_Object_t timerLed;
/* random delay to make sure 2 devices will sync*/
/* the closest the random delays are, the longer it will
   take for the devices to sync when started simultaneously*/
static int32_t random_delay;

static StructPingPongStatus stPingPongStatus;

static UTIL_TIMER_Object_t receiveTimout;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/*!
 * @brief Function to be executed on Radio Tx Done event
 */
static void OnTxDone(void);

/**
  * @brief Function to be executed on Radio Rx Done event
  * @param  payload ptr of buffer received
  * @param  size buffer size
  * @param  rssi
  * @param  LoraSnr_FskCfo
  */
static void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t LoraSnr_FskCfo);

/**
  * @brief Function executed on Radio Tx Timeout event
  */
static void OnTxTimeout(void);

/**
  * @brief Function executed on Radio Rx Timeout event
  */
static void OnRxTimeout(void);

/**
  * @brief Function executed on Radio Rx Error event
  */
static void OnRxError(void);

/* USER CODE BEGIN PFP */
/**
  * @brief  Function executed on when led timer elapses
  * @param  context ptr of LED context
  */
static void OnledEvent(void *context);

/**
  * @brief PingPong state machine implementation
  */
static void PingPong_Process(void);

static void makePingBufferTx(void);

static void sendPing(void);

void receiveTimeoutCallback(void *context )
{
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_SubGHz_Phy_App_Process), CFG_SEQ_Prio_0);
}

/* USER CODE END PFP */

/* Exported functions ---------------------------------------------------------*/
void SubghzApp_Init(void)
{
  /* USER CODE BEGIN SubghzApp_Init_1 */

  APP_LOG(TS_OFF, VLEVEL_M, "\r\nPING PONG\r\n");
  /* Get SubGHY_Phy APP version*/
  APP_LOG(TS_OFF, VLEVEL_M, "APPLICATION_VERSION: V%X.%X.%X\r\n",
          (uint8_t)(APP_VERSION_MAIN),
          (uint8_t)(APP_VERSION_SUB1),
          (uint8_t)(APP_VERSION_SUB2));

  /* Get MW SubGhz_Phy info */
  APP_LOG(TS_OFF, VLEVEL_M, "MW_RADIO_VERSION:    V%X.%X.%X\r\n",
          (uint8_t)(SUBGHZ_PHY_VERSION_MAIN),
          (uint8_t)(SUBGHZ_PHY_VERSION_SUB1),
          (uint8_t)(SUBGHZ_PHY_VERSION_SUB2));

  /* Led Timers*/
  UTIL_TIMER_Create(&timerLed, LED_PERIOD_MS, UTIL_TIMER_ONESHOT, OnledEvent, NULL);
  UTIL_TIMER_Start(&timerLed);

  /* Radio to public */
  Radio.SetPublicNetwork(false);

  /* USER CODE END SubghzApp_Init_1 */

  /* Radio initialization */
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.RxDone = OnRxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.RxTimeout = OnRxTimeout;
  RadioEvents.RxError = OnRxError;

  Radio.Init(&RadioEvents);

  /* USER CODE BEGIN SubghzApp_Init_2 */
  /*calculate random delay for synchronization*/
  random_delay = (Radio.Random()) >> 22; /*10bits random e.g. from 0 to 1023 ms*/

  /* Radio Set frequency */
  Radio.SetChannel(RF_FREQUENCY);

  /* Radio configuration */
#if ((USE_MODEM_LORA == 1) && (USE_MODEM_FSK == 0))
  APP_LOG(TS_OFF, VLEVEL_M, "---------------\r\n");
  APP_LOG(TS_OFF, VLEVEL_M, "LORA_MODULATION\r\n");
  APP_LOG(TS_OFF, VLEVEL_M, "LORA_BW=%d kHz\r\n", (1 << LORA_BANDWIDTH) * 125);
  APP_LOG(TS_OFF, VLEVEL_M, "LORA_SF=%d\r\n", LORA_SPREADING_FACTOR);

  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, TX_TIMEOUT_VALUE);

  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

  Radio.SetMaxPayloadLength(MODEM_LORA, MAX_APP_BUFFER_SIZE);

#elif ((USE_MODEM_LORA == 0) && (USE_MODEM_FSK == 1))
  APP_LOG(TS_OFF, VLEVEL_M, "---------------\r\n");
  APP_LOG(TS_OFF, VLEVEL_M, "FSK_MODULATION\r\n");
  APP_LOG(TS_OFF, VLEVEL_M, "FSK_BW=%d Hz\r\n", FSK_BANDWIDTH);
  APP_LOG(TS_OFF, VLEVEL_M, "FSK_DR=%d bits/s\r\n", FSK_DATARATE);

  Radio.SetTxConfig(MODEM_FSK, TX_OUTPUT_POWER, FSK_FDEV, 0,
                    FSK_DATARATE, 0,
                    FSK_PREAMBLE_LENGTH, FSK_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, 0, TX_TIMEOUT_VALUE);

  Radio.SetRxConfig(MODEM_FSK, FSK_BANDWIDTH, FSK_DATARATE,
                    0, FSK_AFC_BANDWIDTH, FSK_PREAMBLE_LENGTH,
                    0, FSK_FIX_LENGTH_PAYLOAD_ON, 0, true,
                    0, 0, false, true);

  Radio.SetMaxPayloadLength(MODEM_FSK, MAX_APP_BUFFER_SIZE);

#else
#error "Please define a modulation in the subghz_phy_app.h file."
#endif /* USE_MODEM_LORA | USE_MODEM_FSK */

  /*fills tx buffer*/
  memset(BufferTx, 0x0, MAX_APP_BUFFER_SIZE);

  APP_LOG(TS_ON, VLEVEL_L, "rand=%d\r\n", random_delay);
  /*starts reception*/
  Radio.Rx(RX_TIMEOUT_VALUE + random_delay);

  UTIL_TIMER_Create(&receiveTimout, RX_TIMEOUT_VALUE, UTIL_TIMER_ONESHOT, receiveTimeoutCallback, NULL);
  UTIL_TIMER_Start( &receiveTimout );

  makePingBufferTx();
  sendPing();


  /*register task to to be run in while(1) after Radio IT*/
  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_SubGHz_Phy_App_Process), UTIL_SEQ_RFU, PingPong_Process);
  /* USER CODE END SubghzApp_Init_2 */
}

/* USER CODE BEGIN EF */

void SubghzApp_DeInit(void)
{
  Radio.Sleep();
  /* Radio de-initialization */
  RadioEvents.TxDone = 0;
  RadioEvents.RxDone = 0;
  RadioEvents.TxTimeout = 0;
  RadioEvents.RxTimeout = 0;
  RadioEvents.RxError = 0;

  Radio.Init(&RadioEvents);

  UTIL_TIMER_Stop(&receiveTimout);
}

/**
 * @fn void printPingPongCounters(void)
 * @brief helper function to print PING PONG counters
 *
 */
void printPingPongCounters(void)
{
  APP_LOG(TS_ON, VLEVEL_L, "Counters: TX=%u, Rx=%u\r\n", stPingPongStatus.txCounter, stPingPongStatus.rxCounter);
}

/* USER CODE END EF */

/* Private functions ---------------------------------------------------------*/
static void OnTxDone(void)
{
  /* USER CODE BEGIN OnTxDone */
  /* check first tx action, then reset rxCounter + rssi statistics */
  if( stPingPongStatus.txCounter == 0 )
  {
    stPingPongStatus.rxCounter = 0;
    stPingPongStatus.averageRssi = 0;
    stPingPongStatus.latestRssi = 0;
  }

  /* increment pingPong Counter */
  stPingPongStatus.txCounter++;

  APP_LOG(TS_ON, VLEVEL_L, "OnTxDone\r\n");
  printPingPongCounters();

  /* Update the State of the FSM*/
  txReady = true;

  /* Run PingPong process in background*/
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_SubGHz_Phy_App_Process), CFG_SEQ_Prio_0);
  /* USER CODE END OnTxDone */
}

static void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t LoraSnr_FskCfo)
{
  /* USER CODE BEGIN OnRxDone */
  APP_LOG(TS_ON, VLEVEL_L, "OnRxDone\r\n");
#if ((USE_MODEM_LORA == 1) && (USE_MODEM_FSK == 0))
  APP_LOG(TS_ON, VLEVEL_L, "RssiValue=%d dBm, SnrValue=%ddB\r\n", rssi, LoraSnr_FskCfo);
  /* Record payload Signal to noise ratio in Lora*/
  SnrValue = LoraSnr_FskCfo;
#endif /* USE_MODEM_LORA | USE_MODEM_FSK */
#if ((USE_MODEM_LORA == 0) && (USE_MODEM_FSK == 1))
  APP_LOG(TS_ON, VLEVEL_L, "RssiValue=%d dBm, Cfo=%dkHz\r\n", rssi, LoraSnr_FskCfo);
  SnrValue = 0; /*not applicable in GFSK*/
#endif /* USE_MODEM_LORA | USE_MODEM_FSK */
  /* Update the State of the FSM*/

  /* Clear BufferRx*/
  memset(BufferRx, 0, MAX_APP_BUFFER_SIZE);
  /* Record payload size*/
  RxBufferSize = size;
  if (RxBufferSize <= MAX_APP_BUFFER_SIZE)
  {
    memcpy(BufferRx, payload, RxBufferSize);
  }

  /* Record Received Signal Strength*/
  RssiValue = rssi;


  /* Record payload content*/
  APP_LOG(TS_ON, VLEVEL_H, "RX: payload. size=%d \r\n", size);
  for (int32_t i = 0; i < PAYLOAD_LEN; i++)
  {
    APP_LOG(TS_OFF, VLEVEL_H, "0x%02X (%c), ", BufferRx[i], BufferRx[i]);
    if (i % 8 == 7)
    {
      APP_LOG(TS_OFF, VLEVEL_H, "\r\n");
    }
  }

  //only trigger on PONG
  if (strncmp((const char*) BufferRx, PONG, sizeof(PONG) - 1) == 0)
  {
    rxReady = true;
    APP_LOG(TS_OFF, VLEVEL_H, "\r\n");
    /* Run PingPong process in background*/
    UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_SubGHz_Phy_App_Process), CFG_SEQ_Prio_0);

  }
  Radio.Sleep();
  Radio.Rx(RX_TIMEOUT_VALUE);
  /* USER CODE END OnRxDone */
}

static void OnTxTimeout(void)
{
  /* USER CODE BEGIN OnTxTimeout */
  APP_LOG(TS_ON, VLEVEL_L, "OnTxTimeout\r\n");
  /* Update the State of the FSM*/
  txTimeout = true;
  /* Run PingPong process in background*/
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_SubGHz_Phy_App_Process), CFG_SEQ_Prio_0);
  /* USER CODE END OnTxTimeout */
}

static void OnRxTimeout(void)
{
  /* USER CODE BEGIN OnRxTimeout */
  APP_LOG(TS_ON, VLEVEL_L, "OnRxTimeout\r\n");
  stPingPongStatus.latestRssi = 0; //reset
  stPingPongStatus.rxTimeout = true;

  /* Update the State of the FSM*/
  rxTimeout = true;
  /* Run PingPong process in background*/
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_SubGHz_Phy_App_Process), CFG_SEQ_Prio_0);
  /* USER CODE END OnRxTimeout */
}

static void OnRxError(void)
{
  /* USER CODE BEGIN OnRxError */
  APP_LOG(TS_ON, VLEVEL_L, "OnRxError\r\n");
  /* Update the State of the FSM*/
  rxError = true;
  /* Run PingPong process in background*/
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_SubGHz_Phy_App_Process), CFG_SEQ_Prio_0);
  /* USER CODE END OnRxError */
}

/* USER CODE BEGIN PrFD */

static void makePingBufferTx(void)
{
  memset(BufferTx, 0x0, MAX_APP_BUFFER_SIZE);
  memcpy(BufferTx, PING, sizeof(PING) - 1);
  sprintf((char*) &BufferTx[strlen((const char*) BufferTx)], "%ld", random_delay);
}

static void sendPing(void)
{
  APP_LOG(TS_ON, VLEVEL_H, "TX: payload. size=%d \r\n", PAYLOAD_LEN);
  for (int32_t i = 0; i < PAYLOAD_LEN; i++)
  {
    APP_LOG(TS_OFF, VLEVEL_H, "0x%02X (%c), ", BufferTx[i], BufferTx[i]);
    if (i % 8 == 7)
    {
      APP_LOG(TS_OFF, VLEVEL_H, "\r\n");
    }
  }
  APP_LOG(TS_OFF, VLEVEL_H, "\r\n");

  Radio.Send(BufferTx, PAYLOAD_LEN);
}

static void PingPong_Process(void)
{
  Radio.Sleep();

  if (rxReady)
  {
    rxReady = false;

    if (RxBufferSize > 0)
    {
      makePingBufferTx();

      if (strncmp((const char*) BufferRx, PONG, sizeof(PONG) - 1) == 0)
      {
        if (strncmp((const char*) &BufferRx[strlen(PONG)], (const char*) &BufferTx[strlen(PING)], strlen((const char*) &BufferTx[strlen(PING)])) == 0)
        {

          if(  stPingPongStatus.latestRssi != 0 ) //check previous value is already set, then average previous and latest.
          {
            stPingPongStatus.averageRssi = (stPingPongStatus.averageRssi + RssiValue) / 2;
          }
          else
          {
            stPingPongStatus.averageRssi = RssiValue; //no previous value, take value directly
          }
          stPingPongStatus.latestRssi = RssiValue;

          /* increment pingPong Counter */
          stPingPongStatus.rxCounter++;

          printPingPongCounters();

          stPingPongStatus.rxTimeout = false;

          Radio.Rx(RX_TIMEOUT_VALUE);
          UTIL_TIMER_Stop(&timerLed);
#ifdef USE_LORA_APP_LED2
           /* switch off green led */
           HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET); /* LED_GREEN */
 #endif
#ifdef USE_LORA_APP_LED3
           /* master toggles red led */
           HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin); /* LED_RED */
 #endif
          /* Add delay between RX and TX */
          HAL_Delay(Radio.GetWakeupTime() + RX_TIME_MARGIN + random_delay);
          /* master sends PING*/
          APP_LOG(TS_ON, VLEVEL_L, "..."
              "PING"
              "\r\n");
          APP_LOG(TS_ON, VLEVEL_L, "Master Tx start\r\n");
          UTIL_TIMER_StartWithPeriod(&receiveTimout, RX_TIMEOUT_VALUE);
          sendPing();
        }
        else
        {
          Radio.Rx(RX_TIMEOUT_VALUE);
          APP_LOG(TS_ON, VLEVEL_L, "PONG from other\r\n");
        }
      }
      else /* valid reception but neither a PING or a PONG message */
      {
        Radio.Rx(RX_TIMEOUT_VALUE);
        APP_LOG(TS_ON, VLEVEL_L, "Invalid data received\r\n");
      }
    }

  }

  else if (txReady)
  {
    txReady = false;
    Radio.Rx(RX_TIMEOUT_VALUE);
  }

  else if( txTimeout )
  {
    Radio.Rx(RX_TIMEOUT_VALUE);
    txTimeout = false;
  }

  else if( rxTimeout || rxError)
  {
    rxTimeout = false;
    rxError = false;

    Radio.Rx(RX_TIMEOUT_VALUE);
    /* Send the next PING frame */
    /* Add delay between RX and TX*/
    /* add random_delay to force sync between boards after some trials*/
    HAL_Delay(Radio.GetWakeupTime() + RX_TIME_MARGIN + random_delay);
    APP_LOG(TS_ON, VLEVEL_L, "Master Tx start1\r\n");
    /* master sends PING*/
    makePingBufferTx();
    sendPing();

  }


  if (receiveTimout.Timestamp == 0)
  {
    Radio.Rx(RX_TIMEOUT_VALUE);
    UTIL_TIMER_StartWithPeriod(&receiveTimout, RX_TIMEOUT_VALUE);
    HAL_Delay( RX_TIME_MARGIN + (random_delay & 0xFFF) );

    UTIL_TIMER_StartWithPeriod(&receiveTimout, RX_TIMEOUT_VALUE);

    APP_LOG(TS_ON, VLEVEL_L, "Master Tx start2\r\n");
    sendPing();
  }
}

static void OnledEvent(void *context)
{
#ifdef USE_LORA_APP_LED2
  HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin); /* LED_GREEN */
#endif
#ifdef USE_LORA_APP_LED3
  HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin); /* LED_RED */
#endif
  UTIL_TIMER_Start(&timerLed);
}

/**
 * @fn StructPingPongStatus getPingPongStatus(void)
 * @brief function to get the PING-PONG status struct
 *
 * @return
 */
StructPingPongStatus getPingPongStatus(void)
{
  return stPingPongStatus;
}

/**
 * @fn States_t getPingPongState(void)
 * @brief get the PING-PONG state \ref States_t
 *
 * @return
 */
States_t getPingPongState(void)
{
  return State;
}

/**
 * @fn int8_t getPingPongRssi(void)
 * @brief get the last received PING-PONG rssi value
 *
 * @return
 */
int8_t getPingPongRssi(void)
{
  return stPingPongStatus.latestRssi;
}

/**
 * @fn void resetPingPongCounters(void)
 * @brief reset the PING-PONG counters
 *
 */
void resetPingPongCounters(void)
{
  stPingPongStatus.rxCounter = 0;
  stPingPongStatus.txCounter = 0;
}

/* USER CODE END PrFD */
