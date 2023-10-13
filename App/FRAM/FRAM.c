/**
  ******************************************************************************
  * @addtogroup     : App
  * @{
  * @file           : FRAM.c
  * @brief          : FRAM SPI functions
  * @author         : P.Kwekkeboom
  * @date           : Oct 12, 2023
  * @}
  ******************************************************************************
  */

#include "main.h"
#include "sys_app.h"

#include "../IO/board_io.h"
#include "FRAM.h"

extern SPI_HandleTypeDef hspi1;
#define HSPI_FRAM   &hspi1

#define HAL_DELAY_FRAM    100
#define FRAM_CS_PORT      GPIOA
#define FRAM_CS_PIN       GPIO_PIN_4

// Define the commands for the FM25L16B FRAM
#define FRAM_WRITE_ENABLE  0x06
#define FRAM_WRITE_DISABLE 0x04
#define FRAM_READ          0x03
#define FRAM_WRITE         0x02
#define FRAM_STATUS_READ   0x05
#define FRAM_STATUS_WRITE  0x01

/**
 * @fn const void FRAM_EnableChipSelect(void)
 * @brief function to enable chip select for FRAM
 *
 */
const void FRAM_EnableChipSelect(void)
{
  writeOutput_board_io(EXT_IO_FRAM_CS, GPIO_PIN_RESET);
}

/**
 * @fn const void FRAM_DisableChipSelect(void)
 * @brief function to disable chip select for FRAM
 *
 */
const void FRAM_DisableChipSelect(void)
{
  writeOutput_board_io(EXT_IO_FRAM_CS, GPIO_PIN_SET);
}

/**
 * @fn const void FRAM_EnableWrite()
 * @brief Function to enable write operations
 *
 */
const void FRAM_EnableWrite()
{
  uint8_t command[] = {FRAM_WRITE_ENABLE};

  HAL_SPI_Transmit(HSPI_FRAM, command, 1, HAL_DELAY_FRAM); //send enable write command
}

/**
 * @fn const void FRAM_DisableWrite()
 * @brief Function to disable write operations
 *
 */
const void FRAM_DisableWrite()
{
  uint8_t command[] = {FRAM_WRITE_DISABLE};
  HAL_SPI_Transmit(HSPI_FRAM, command, 1, HAL_DELAY_FRAM); //send disable write command
 }

/**
 * @fn const void FRAM_WriteData(uint16_t, uint8_t*, size_t)
 * @brief Function to write an array of data to the FRAM
 *
 * @param address
 * @param data
 * @param length
 */
const void FRAM_WriteData(uint16_t address, uint8_t *data, size_t length)
{
  // Enable chip select
  FRAM_EnableChipSelect();

  // Enable write operation
  FRAM_EnableWrite();

  // Send the write command and address
  uint8_t writeCommand[3] = { FRAM_WRITE, (uint8_t) (address >> 8), (uint8_t) (address & 0xFF) };
  HAL_SPI_Transmit(HSPI_FRAM, writeCommand, sizeof(writeCommand), HAL_DELAY_FRAM);

  // Send the data to be written
  HAL_SPI_Transmit(HSPI_FRAM, data, length, HAL_DELAY_FRAM);

  // Disable write operation automatically done when chip select is disabled.
  // FRAM_DisableWrite();

  // Disable chip select
  FRAM_DisableChipSelect();
}

/**
 * @fn const void FRAM_ReadData(uint16_t, uint8_t*, size_t)
 * @brief Function to read an array of data from the FRAM
 *
 * @param address
 * @param data
 * @param length
 */
const void FRAM_ReadData(uint16_t address, uint8_t *data, size_t length)
{
  // Enable chip select
  FRAM_EnableChipSelect();

  // Send the read command and address
  uint8_t readCommand[3] = { FRAM_READ, (uint8_t) (address >> 8), (uint8_t) (address & 0xFF) };
  HAL_SPI_Transmit(HSPI_FRAM, readCommand, sizeof(readCommand), HAL_DELAY_FRAM);

  // Read the data
  HAL_SPI_Receive(HSPI_FRAM, data, length, HAL_DELAY_FRAM);

  // Disable chip select
  FRAM_DisableChipSelect();
}

/**
 * @fn const uint8_t FRAM_ReadStatusRegister(void)
 * @brief funtion to read status register
 *
 * @return status register value
 */
const uint8_t FRAM_ReadStatusRegister(void)
{
  // Enable chip select
  FRAM_EnableChipSelect();

  // Send the read command and address
  uint8_t readCommand[1] = { FRAM_STATUS_READ };
  HAL_SPI_Transmit(HSPI_FRAM, readCommand, sizeof(readCommand), HAL_DELAY_FRAM);

  // Read the data
  uint8_t data[1];
  HAL_SPI_Receive(HSPI_FRAM, data, sizeof(data), HAL_DELAY_FRAM);

  // Disable chip select
  FRAM_DisableChipSelect();

  return data[0];
}

/**
 * @fn const void FRAM_WriteStatusRegister(uint8_t)
 * @brief function to write status register
 *
 * @param data
 */
const void FRAM_WriteStatusRegister(uint8_t data)
{
  // Enable chip select
  FRAM_EnableChipSelect();

  // Enable write operation
  FRAM_EnableWrite();

  // Send the read command and address
  uint8_t writeCommand[2] = { FRAM_STATUS_WRITE, data };
  HAL_SPI_Transmit(HSPI_FRAM, writeCommand, sizeof(writeCommand), HAL_DELAY_FRAM);

  // Disable write operation automatically done when chip select is disabled.
  // FRAM_DisableWrite();

  // Disable chip select
  FRAM_DisableChipSelect();

}
