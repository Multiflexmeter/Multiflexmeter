/**
  ******************************************************************************
  * @file           : uart.h
  * @brief          : Header for uart.c file.
  * @author         : P.Kwekkeboom
  * @date           : Apr 24, 2023
  * @copyright      : 2023 Dekimo Goes
  ******************************************************************************
  */
#ifndef COMMON_UART_H_
#define COMMON_UART_H_

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

void setCharacterMatch(UART_HandleTypeDef *huart, uint8_t matchCharacter);
bool checkMoreThenOneCharacterInDma(UART_HandleTypeDef *huart);
HAL_StatusTypeDef uartSend(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t Size);
bool uartTxBusy(UART_HandleTypeDef *huart);
bool uartTxReady(UART_HandleTypeDef *huart);

#endif /* COMMON_UART_H_ */
