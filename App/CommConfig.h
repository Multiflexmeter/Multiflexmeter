/**
  ******************************************************************************
  * @file           : CommConfig.h
  * @brief          : Header for CommConfig.c file.
  * @author         : P.Kwekkeboom
  * @date           : Apr 24, 2023
  * @copyright      : 2023 Dekimo Goes
  ******************************************************************************
  */
#ifndef COMMCONFIG_H_
#define COMMCONFIG_H_

#define config_uart huart1

#define SIZE_TX_BUFFER_CONFIG	250
#define SIZE_RX_BUFFER_CONFIG	250

HAL_StatusTypeDef uartSend_Config( const uint8_t *pData, uint16_t Size);
void testConfigUartSend(void);

#endif /* COMMCONFIG_H_ */
