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

void setCharacterMatch(UART_HandleTypeDef *huart, uint8_t matchCharacter);
bool checkMoreThenOneCharacterInDma(UART_HandleTypeDef *huart);

#endif /* COMMON_UART_H_ */
