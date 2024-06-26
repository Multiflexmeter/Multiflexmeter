/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32wlxx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define RTC_PREDIV_A ((1<<(15-RTC_N_PREDIV_S))-1)
#define RTC_N_PREDIV_S 10
#define RTC_PREDIV_S ((1<<RTC_N_PREDIV_S)-1)
#define MCU_IRQ_Pin GPIO_PIN_3
#define MCU_IRQ_GPIO_Port GPIOB
#define MCU_IRQ_EXTI_IRQn EXTI3_IRQn
#define FE_CTRL3_Pin GPIO_PIN_4
#define FE_CTRL3_GPIO_Port GPIOB
#define DEBUG_LED2_Pin GPIO_PIN_5
#define DEBUG_LED2_GPIO_Port GPIOB
#define IO_EXP_IRQ_Pin GPIO_PIN_6
#define IO_EXP_IRQ_GPIO_Port GPIOB
#define IO_EXP_IRQ_EXTI_IRQn EXTI9_5_IRQn
#define USART2_CTS_Pin GPIO_PIN_0
#define USART2_CTS_GPIO_Port GPIOA
#define USART2_RTS_Pin GPIO_PIN_1
#define USART2_RTS_GPIO_Port GPIOA
#define DEBUG_LED1_Pin GPIO_PIN_4
#define DEBUG_LED1_GPIO_Port GPIOA
#define ADC_SENS_Pin GPIO_PIN_2
#define ADC_SENS_GPIO_Port GPIOB
#define DETECT_3V3_Pin GPIO_PIN_12
#define DETECT_3V3_GPIO_Port GPIOB
#define DETECT_3V3_EXTI_IRQn EXTI15_10_IRQn
#define FE_CTRL2_Pin GPIO_PIN_13
#define FE_CTRL2_GPIO_Port GPIOC
#define BOX_OPEN_Pin GPIO_PIN_15
#define BOX_OPEN_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
