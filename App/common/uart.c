/**
  ******************************************************************************
  * @addtogroup     : common
  * @{
  * @file           : uart.c
  * @brief          : uart functions
  * @author         : P.Kwekkeboom
  * @date           : Apr 24, 2023
  * @copyright      : 2023 Dekimo Goes
  * @}
  ******************************************************************************
  */
#include "main.h"
#include "app_types.h"
#include "usart.h"

/**
 * @brief function to setup uart character match
 * @param huart UART handle.
 * @param matchCharacter the character to match with
 */
void setCharacterMatch(UART_HandleTypeDef *huart, uint8_t matchCharacter)
{
  assert_param( huart != 0 );

  /*
   * argument checking huart is not zero
   */
  if( huart == 0 )
    return;

  /*
   * Read UE bit, must be disabled before changing CR2 register
   */
  uint32_t bit_UE = READ_BIT(huart->Instance->CR1, USART_CR1_UE); //read UE

  /*
   * UE must be disabled before changing CR2 register
   */
  CLEAR_BIT(huart->Instance->CR1, USART_CR1_UE); //disable UE

  /*
   * change CR2 register with matchCaracter
   */
  MODIFY_REG(huart->Instance->CR2, USART_CR2_ADD, ((uint32_t)matchCharacter << UART_CR2_ADDRESS_LSB_POS));

  /*
   * add enable character match interrupt, to get every character match an interrupt
   */
  bit_UE |= USART_CR1_CMIE;

  /*
   * change back UE
   */
  SET_BIT(huart->Instance->CR1, bit_UE);

}

/**
 * @brief default weak empty funciton.
 *  must be overrid by user when 9bits uart is used.
 *
 * @param huart
 * @return
 */
__weak uint16_t getCharachterMatch(UART_HandleTypeDef *huart)
{
	return 0;
}

/**
 * @brief weak default empty function, handles charactermatch callback from huart1
 *  function must be override by user
 *
 * @param huart UART handle.
 * @param nrCharsAvailable number of characters read by uart
 */
__weak void user_uart1_characterMatchDetect_Callback(UART_HandleTypeDef *huart, uint32_t nrCharsAvailable)
{

}

/**
 * @brief weak default empty function, handles charactermatch callback from huart2.
 *  function must be override by user
 *
 * @param huart UART handle.
 * @param nrCharsAvailable number of characters read by uart
 */
__weak void user_uart2_characterMatchDetect_Callback(UART_HandleTypeDef *huart, uint32_t nrCharsAvailable)
{

}

/**
 * @brief weak default function, handles charactermatch callback from huart1 and huart2.
*  function can be override by user
*
 * @param huart UART handle.
 * @param nrCharsAvailable number of characters read by uart
 */
__weak void user_characterMatchDetect_Callback(UART_HandleTypeDef *huart, uint32_t nrCharsAvailable)
{

	if( huart == &huart1 )
	{
		user_uart1_characterMatchDetect_Callback(huart, nrCharsAvailable);
	}

	if( huart == &huart2 )
	{
		user_uart2_characterMatchDetect_Callback(huart, nrCharsAvailable);
	}
}


/**
 * @brief character match IRQ handler function
 *  - checks the UART_FLAG_CMF is set
 *  - clears the flag
 *  - reads out number of received characters
 *  - user_characterMatchDetect_Callback() function is called.
 *
 *  @note for 9 bits getCharachterMatch() functions needs to be implemented
 *
 *
 * @param huart UART handle.
 */
void charachterMatch_IRQHandler(UART_HandleTypeDef *huart)
{
	/*
	 * Check whether the Character Match UART flag is set or not.
	 */
	if( __HAL_UART_GET_FLAG(huart, UART_FLAG_CMF))
	{
		/*
		 * Clear the specified UART pending flag.
		 */
		__HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_CMF);

		/*
		 * check pointer is not zero.
		 */
		if( huart->hdmarx != 0x00 )
		{
      /*
       * get number of characters
       */
      uint32_t nrCharsAvailable = huart->RxXferSize - __HAL_DMA_GET_COUNTER(huart->hdmarx);

      /*
       * for 9 bits or more check FLAG CHARACTER here manually
       */
      if ( huart->Init.WordLength == UART_WORDLENGTH_9B)
      {
        uint16_t * pRxBuffPtr_16bit = (uint16_t*) huart->pRxBuffPtr; //cast to 16 bit buffer
        if( pRxBuffPtr_16bit[nrCharsAvailable-1] == getCharachterMatch(huart))
        {
          user_characterMatchDetect_Callback(huart, nrCharsAvailable);
        }
      }

      /*
       * for 8 bits and less, just trust the interrupt flag
       */
      else
      {
        user_characterMatchDetect_Callback(huart, nrCharsAvailable);
      }
		}
	}
}


/**
 * @brief function checks if more then one character is received in DMA buffer.
 * @param huart UART handle.
 * @return bool TRUE = more then one characters received, FALSE = 0 or 1 character received
 */
bool checkMoreThenOneCharacterInDma(UART_HandleTypeDef *huart)
{
  if( huart )
    return (huart->RxXferSize - __HAL_DMA_GET_COUNTER(huart->hdmarx) > 1);

  return FALSE;
}

/**
 * @brief wrapper function for HAL_UART_Transmit_DMA with a AbortTransmit before
 *
 * @param huart UART handle.
 * @param pData Pointer to data buffer (u8 or u16 data elements).
 * @param Size  Amount of data elements (u8 or u16) to be sent.
 * @return
 */
HAL_StatusTypeDef uartSend(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t Size)
{
  HAL_UART_AbortTransmit(huart);
	return HAL_UART_Transmit_DMA(huart, pData, Size);
}

/**
 * @brief function checks if uart transmit is busy
 *
 * @param huart UART handle.
 * @return TRUE = transmit is busy
 */
bool uartTxBusy(UART_HandleTypeDef *huart)
{
	return (huart->gState == HAL_UART_STATE_BUSY_TX);
}

/**
 * @brief function checks if uart transmit is ready
 *
 * @param huart UART handle.
 * @return TRUE = transmit is ready
 */
bool uartTxReady(UART_HandleTypeDef *huart)
{
	return !(uartTxBusy(huart));
}

