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

