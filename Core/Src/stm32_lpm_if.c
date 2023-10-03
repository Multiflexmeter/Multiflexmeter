/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32_lpm_if.c
  * @author  MCD Application Team
  * @brief   Low layer function to enter/exit low power modes (stop, sleep)
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
#include "stm32_lpm.h"
#include "stm32_lpm_if.h"
#include "usart_if.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* External variables ---------------------------------------------------------*/
/* USER CODE BEGIN EV */
static uint32_t CLK_CR_copy;
static uint32_t CLK_CFGR_copy;
static uint32_t GetPCLK_copy1;
static uint32_t GetPCLK_copy2;
static uint32_t GetAHBPrescaler_copy1;
static uint32_t GetAHBPrescaler_copy2;
static uint32_t GetSysClockFreq_copy1;
static uint32_t GetSysClockFreq_copy2;
static uint32_t GET_SYSCLK_SOURCE_copy1;
static uint32_t GET_SYSCLK_SOURCE_copy2;
static uint32_t time1;
static uint32_t time2;
/* USER CODE END EV */

/* Private typedef -----------------------------------------------------------*/
/**
  * @brief Power driver callbacks handler
  */
const struct UTIL_LPM_Driver_s UTIL_PowerDriver =
{
  PWR_EnterSleepMode,
  PWR_ExitSleepMode,

  PWR_EnterStopMode,
  PWR_ExitStopMode,

  PWR_EnterOffMode,
  PWR_ExitOffMode,
};

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define CFGR_CLEAR_MASK_FOR_SAVE (  ( 0x1UL << 31 )   | \
                                    ( 0x1FUL << 19)   | \
                                    ( 0x1UL << 14 )   | \
                                    ( RCC_CFGR_SWS )    \
                                    )
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Exported functions --------------------------------------------------------*/

void PWR_EnterOffMode(void)
{
  /* USER CODE BEGIN EnterOffMode_1 */

  /* USER CODE END EnterOffMode_1 */
}

void PWR_ExitOffMode(void)
{
  /* USER CODE BEGIN ExitOffMode_1 */

  /* USER CODE END ExitOffMode_1 */
}

void PWR_EnterStopMode(void)
{
  /* USER CODE BEGIN EnterStopMode_1 */

  CLK_CR_copy = RCC->CR;
  CLK_CFGR_copy = RCC->CFGR & ~CFGR_CLEAR_MASK_FOR_SAVE;

  /* USER CODE END EnterStopMode_1 */
  HAL_SuspendTick();
  /* Clear Status Flag before entering STOP/STANDBY Mode */
  LL_PWR_ClearFlag_C1STOP_C1STB();

  /* USER CODE BEGIN EnterStopMode_2 */

  /* USER CODE END EnterStopMode_2 */
  HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);
  /* USER CODE BEGIN EnterStopMode_3 */

  /* USER CODE END EnterStopMode_3 */
}

void PWR_ExitStopMode(void)
{
  /* USER CODE BEGIN ExitStopMode_1 */
  bool timeout = false;
  RCC->CR = CLK_CR_copy;
  RCC->CFGR = CLK_CFGR_copy;

  GET_SYSCLK_SOURCE_copy1 = __HAL_RCC_GET_SYSCLK_SOURCE();
  GetSysClockFreq_copy1 = HAL_RCC_GetSysClockFreq();
  GetAHBPrescaler_copy1 = LL_RCC_GetAHBPrescaler();
  GetPCLK_copy1 = HAL_RCC_GetPCLK2Freq();


  /* Get Start Tick */
  time1 = RTC->SSR;
  uint32_t tickstart = HAL_GetTick();

  HAL_ResumeTick(); //resume earlier, for check CLK timeout.

  /* check system clock source switch status */
  while ( __HAL_RCC_GET_SYSCLK_SOURCE() != ( (CLK_CFGR_copy & RCC_CFGR_SW) << RCC_CFGR_SWS_Pos) && timeout == false )
  {
    if ((HAL_GetTick() - tickstart) > (50U) )
    {
      timeout = true; //return HAL_TIMEOUT;
    }
  }

  time2 = RTC->SSR;

  GET_SYSCLK_SOURCE_copy2 = __HAL_RCC_GET_SYSCLK_SOURCE();
  GetSysClockFreq_copy2 = HAL_RCC_GetSysClockFreq();
  GetAHBPrescaler_copy2 = LL_RCC_GetAHBPrescaler();
  GetPCLK_copy2 = HAL_RCC_GetPCLK2Freq();

  /* USER CODE END ExitStopMode_1 */
  /* Resume sysTick : work around for debugger problem in dual core */
  HAL_ResumeTick();
  /*Not retained periph:
    ADC interface
    DAC interface USARTx, TIMx, i2Cx, SPIx
    SRAM ctrls, DMAx, DMAMux, AES, RNG, HSEM  */

  /* Resume not retained USARTx and DMA */
  vcom_Resume();
  /* USER CODE BEGIN ExitStopMode_2 */

  /* USER CODE END ExitStopMode_2 */
}

void PWR_EnterSleepMode(void)
{
  /* USER CODE BEGIN EnterSleepMode_1 */

  /* USER CODE END EnterSleepMode_1 */
  /* Suspend sysTick */
  HAL_SuspendTick();
  /* USER CODE BEGIN EnterSleepMode_2 */

  /* USER CODE END EnterSleepMode_2 */
  HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
  /* USER CODE BEGIN EnterSleepMode_3 */

  /* USER CODE END EnterSleepMode_3 */
}

void PWR_ExitSleepMode(void)
{
  /* USER CODE BEGIN ExitSleepMode_1 */

  /* USER CODE END ExitSleepMode_1 */
  /* Resume sysTick */
  HAL_ResumeTick();

  /* USER CODE BEGIN ExitSleepMode_2 */

  /* USER CODE END ExitSleepMode_2 */
}

/* USER CODE BEGIN EF */

/* USER CODE END EF */

/* Private Functions Definition -----------------------------------------------*/
/* USER CODE BEGIN PrFD */

/* USER CODE END PrFD */
