#include "sys_app.h"

#include "I2C_Master.h"

#include <string.h>
#include "../common/crc16.h"
#include "SensorRegister.h"

#define CRC_SIZE 2

extern I2C_HandleTypeDef hi2c2;

/**
 * @fn const bool sensorMasterTimeout(uint32_t, uint32_t)
 * @brief function to check timeout
 *
 * @param TickStart : start time in ticks
 * @param Timeout : timeout time in ticks
 * @return
 */
static const bool sensorMasterTimeout(uint32_t Tickstart, uint32_t Timeout)
{
  if (((HAL_GetTick() - Tickstart) > Timeout) || (Timeout == 0U))
  {
    return true;
  }
  return false;
}

/**
 * @fn void check_and_print_I2C_error(void)
 * @brief function to print I2C error
 *
 */
void check_and_print_I2C_error(void)
{
  HAL_I2C_StateTypeDef I2C_State = HAL_I2C_GetState(&hi2c2);
  HAL_I2C_ModeTypeDef I2C_Mode = HAL_I2C_GetMode(&hi2c2);
  uint32_t I2C_Error = HAL_I2C_GetError(&hi2c2);

  if( I2C_Error )
  {
    APP_LOG(TS_OFF, VLEVEL_H, "I2C: state: %d, Mode: 0x%02X, Error: 0x%04x\r\n", I2C_State, I2C_Mode, I2C_Error);
  }
}

/**
 * @brief Read a register on a sensor module
 *
 * @param slaveAddress The i2c slave address of the sensor module
 * @param regAddress The address of the register that needs to be read
 * @param data Pointer to a buffer to store the result in.
 * @return Return I2C_OK when the function is successful.
 *         Else returns a specific error based on the type of fault.
 */
ENUM_I2C_Error sensorMasterRead(uint8_t slaveAddress, uint8_t regAddress, uint8_t *data, uint16_t dataLength)
{
  // Determine the index of the register based on the register address
  int8_t regIndex = findRegIndex(regAddress);
  if(regIndex < 0)
    return I2C_REGISTER_ERROR;

  // Determine the size of the register
  uint8_t regSize = registers[regIndex].datatype * registers[regIndex].size;

  // Request the data from the register
  uint8_t rxBuffer[regSize + CRC_SIZE];
  HAL_StatusTypeDef error = HAL_I2C_Mem_Read(&hi2c2, slaveAddress, regAddress, 1, rxBuffer, regSize + CRC_SIZE, 1000);
  if(error != HAL_OK)
  {
    //HAL_I2C_Master_Abort_IT(&hi2c2, slaveAddress);
    check_and_print_I2C_error();
    HAL_I2C_DeInit(&hi2c2);
    HAL_I2C_Init(&hi2c2);
    return I2C_TIMEOUT;
  }

  // Check the CRC of the incoming message
  if(calculateCRC_CCITT(rxBuffer, regSize + CRC_SIZE) != 0)
    return I2C_CRC_ERROR;

  // blank the destination buffer
  memset(data, 0x00, dataLength );

  // Copy the data to the provided memory location and remove the CRC from the data
  memcpy(data, rxBuffer, regSize > dataLength ? dataLength : regSize);

  return I2C_TRANSFER_OK;
}

/**
 * @brief Write data to a register on a sensor module
 *
 * @param slaveAddress The i2c slave address of the sensor module
 * @param regAddress The address of the register to write
 * @param data The data to write
 * @return Return I2C_OK when the function is successful.
 *         Else returns a specific error based on the type of fault.
 */
ENUM_I2C_Error sensorMasterWrite(uint8_t slaveAddress, uint8_t regAddress, uint8_t *data)
{
  // Determine the index of the register based on the register address
  int8_t regIndex = findRegIndex(regAddress);
  if(regIndex < 0)
    return I2C_REGISTER_ERROR;

  // Determine the size of the register and the tx buffer
  uint8_t regSize = registers[regIndex].datatype * registers[regIndex].size;
  uint8_t txBuffer[sizeof(regAddress) + regSize + CRC_SIZE];

  // Copy the data into the tx buffer
  txBuffer[0] = regAddress;
  memcpy(&txBuffer[1], data, regSize);

  // Calculate the CRC and add this to the tx buffer
  uint16_t crc = calculateCRC_CCITT(txBuffer, regSize+1);
  txBuffer[regSize+2] = crc & 0xFF;
  txBuffer[regSize+1] = (crc >> 8) & 0xFF;

  // Write data the the register
  HAL_StatusTypeDef error = HAL_I2C_Mem_Write(&hi2c2, slaveAddress, regAddress, 1, &txBuffer[1], regSize + CRC_SIZE, 1000);
  if(error != HAL_OK)
  {
    check_and_print_I2C_error();
    HAL_I2C_DeInit(&hi2c2);
    HAL_I2C_Init(&hi2c2);
    return I2C_TIMEOUT;
  }

  return I2C_TRANSFER_OK;
}

/**
 * @brief Read data with variable length
 *
 * @param slaveAddress The i2c slave address of the sensor module
 * @param measurementData The pointer to store the measurement data, minimum of 3.
 * @return Return I2C_OK when the function is successful.
 *         Else returns a specific error based on the type of fault.
 */
ENUM_I2C_Error sensorMasterReadVariableLength(uint8_t slaveAddress, uint8_t regAddress, uint8_t* data, uint16_t dataLength)
{
  uint8_t variableLength;
  uint32_t tickstart;
  uint32_t timeout = 1000;

  /* check minimum length */
  if( dataLength <=  1 + CRC_SIZE )
  {
    return I2C_BUFFER_ERROR;
  }

  /* Init tickstart for timeout management*/
  tickstart = HAL_GetTick();

  /* Select the measurement data register */
  HAL_I2C_Master_Seq_Transmit_IT(&hi2c2, slaveAddress, &regAddress, 1, I2C_FIRST_FRAME);
  while (HAL_I2C_GetState(&hi2c2) != HAL_I2C_STATE_READY)
  {
    if(sensorMasterTimeout(tickstart,timeout) )
    {
      return I2C_TIMEOUT;
    }
  }

  /* Receive the lenght of the measurement data */
  HAL_I2C_Master_Seq_Receive_IT(&hi2c2, slaveAddress, &variableLength, 1, I2C_NEXT_FRAME);
  while (HAL_I2C_GetState(&hi2c2) != HAL_I2C_STATE_READY)
  {
    if(sensorMasterTimeout(tickstart,timeout) )
    {
      return I2C_TIMEOUT;
    }
  }

  /* Limit the message length */
  if( (variableLength + 1 + CRC_SIZE ) > dataLength )
    variableLength = dataLength - (1 + CRC_SIZE);

  data[0] = variableLength; //save effective remaining datalength

  /* Receive the measurement data */
  HAL_I2C_Master_Seq_Receive_IT(&hi2c2, slaveAddress, &data[1], variableLength + CRC_SIZE, I2C_LAST_FRAME);
  while (HAL_I2C_GetState(&hi2c2) != HAL_I2C_STATE_READY)
  {
    if(sensorMasterTimeout(tickstart,timeout) )
    {
      return I2C_TIMEOUT;
    }
  }

  /* Check the CRC of the incoming message */
  if(calculateCRC_CCITT(data, variableLength + CRC_SIZE + 1) != 0)
    return I2C_CRC_ERROR;

  return I2C_TRANSFER_OK;
}
