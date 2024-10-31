#include "ADS7138.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>


/**
 * @brief Single register read function.
 * @param registerAddress The address of the desired register to read out
 * @return The data from the specified register
 */
static uint8_t singleRegisterRead(I2C_HandleTypeDef *hi2c, uint8_t i2cAddress, uint8_t registerAddress)
{
  uint8_t singeReadCommand[2] =
  {
      OP_SINGLE_REGISTER_READ,
      registerAddress
  };
  uint8_t result;

  HAL_I2C_Master_Transmit(hi2c, i2cAddress, singeReadCommand, sizeof(singeReadCommand), I2CTIMEOUT);
  HAL_I2C_Master_Receive(hi2c, i2cAddress, &result, sizeof(result), I2CTIMEOUT);

  return result;
}

/**
 * @brief Single register write function.
 * @param registerAddress The address of the desired register to write to
 * @param data The data to write to the register
 * @return Result of HAL driver
 */
static RESULT_StatusTypeDef singleRegisterWrite(I2C_HandleTypeDef *hi2c, uint8_t i2cAddress, uint8_t registerAddress, uint8_t data)
{
  uint8_t singeWriteCommand[3] =
  {
      OP_SINGLE_REGISTER_WRITE,
      registerAddress,
      data
  };
  return (RESULT_StatusTypeDef) HAL_I2C_Master_Transmit(hi2c, i2cAddress, singeWriteCommand, sizeof(singeWriteCommand), I2CTIMEOUT);
}

/**
 * @brief Set individual bits of a register
 * @param registerAddress The address of the desired register to write to
 * @param bits The bits to set. Every high bit will be set.
 * @return Result of HAL driver
 */
static RESULT_StatusTypeDef setBit(I2C_HandleTypeDef *hi2c, uint8_t i2cAddress, uint8_t registerAddress, uint8_t bits)
{
  uint8_t setBitsCommand[3] =
  {
      OP_SET_BIT,
      registerAddress,
      bits
  };
  return (RESULT_StatusTypeDef) HAL_I2C_Master_Transmit(hi2c, i2cAddress, setBitsCommand, sizeof(setBitsCommand), I2CTIMEOUT);
}

/**
 * @brief Clear individual bits of a register.
 * @param registerAddress The address of the desired register to write to
 * @param bits The bits to clear. Every high bit will be cleared.
 * @return Result of HAL driver
 */
static RESULT_StatusTypeDef clearBit(I2C_HandleTypeDef *hi2c, uint8_t i2cAddress, uint8_t registerAddress, uint8_t bits)
{
  uint8_t clearBitsCommand[3] =
  {
      OP_CLEAR_BIT,
      registerAddress,
      bits
  };
  return (RESULT_StatusTypeDef) HAL_I2C_Master_Transmit(hi2c, i2cAddress, clearBitsCommand, sizeof(clearBitsCommand), I2CTIMEOUT);
}

/**
 * @brief Read multiple registers continuously.
 * @param startAddress The first address to start reading from
 * @param size The amount of registers to read out
 * @param pData Pointer to where to store the results
 * @return Result of HAL driver
 */
static RESULT_StatusTypeDef multipleRegisterRead(I2C_HandleTypeDef *hi2c, uint8_t i2cAddress, uint8_t startAddress, uint8_t *pData, uint16_t size)
{
  RESULT_StatusTypeDef status;
  uint8_t multipleReadCommand[2] =
  {
      OP_MULTIPLE_REGISTER_READ,
      startAddress
  };
  status = (RESULT_StatusTypeDef) HAL_I2C_Master_Transmit(hi2c, i2cAddress, multipleReadCommand, sizeof(multipleReadCommand), I2CTIMEOUT);
  status = (RESULT_StatusTypeDef) HAL_I2C_Master_Receive(hi2c, i2cAddress, pData, size, I2CTIMEOUT);
  return status;
}

/**
 * @brief Write to multiple registers continuously.
 * @param startAddress The first address to start writing from
 * @param size The amount of registers to write to
 * @param pData Pointer to the write data
 * @return Result of HAL driver
 */
static RESULT_StatusTypeDef multipleRegisterWrite(I2C_HandleTypeDef *hi2c, uint8_t i2cAddress, uint8_t startAddress, uint8_t *pData, uint16_t size)
{
  uint8_t multipleWriteCommand[2] =
  {
      OP_MULTIPLE_REGISTER_READ,
      startAddress
  };
  uint8_t i2cData[2 + size];
  memcpy(i2cData, multipleWriteCommand, sizeof(multipleWriteCommand));
  memcpy(i2cData + 2, pData, size);

  return (RESULT_StatusTypeDef) HAL_I2C_Master_Transmit(hi2c, i2cAddress, i2cData, sizeof(i2cData), I2CTIMEOUT);
}

void ADS7138setADC(I2C_HandleTypeDef *hi2c, uint8_t i2cAddress, uint8_t channel)
{
  // Set pins as ADC
  clearBit(hi2c, i2cAddress, PIN_CFG_ADDRESS, channel);
  clearBit(hi2c, i2cAddress, GPIO_CFG_ADDRESS, channel);
}

/**
 * @brief Configures the GPIO direction and drive mode.
 * @param channel The channel to set as an GPIO output
 * @param driveMethod The drive method for the GPIO output
 */
void ADS7138setGPIO_Output(I2C_HandleTypeDef *hi2c, uint8_t i2cAddress, uint8_t channel, GPIO_driveMethod driveMethod)
{
  // Set pins as GPIO
  setBit(hi2c, i2cAddress, PIN_CFG_ADDRESS, channel);
  setBit(hi2c, i2cAddress, GPIO_CFG_ADDRESS, channel);

  // Set GPIO drive method
  if (driveMethod == PUSH_PULL)
  {
    setBit(hi2c, i2cAddress, GPO_DRIVE_CFG_ADDRESS, channel);
  }
  else
  {
    clearBit(hi2c, i2cAddress, GPO_DRIVE_CFG_ADDRESS, channel);
  }
}

/**
 * @brief Configures the GPIO direction and drive mode.
 * @param channel The channel to set as an GPIO input
 */
void ADS7138setGPIO_Input(I2C_HandleTypeDef *hi2c, uint8_t i2cAddress, uint8_t channel)
{
  setBit(hi2c, i2cAddress, PIN_CFG_ADDRESS, channel);
  clearBit(hi2c, i2cAddress, GPIO_CFG_ADDRESS, channel);
}

/**
 * @brief Read GPIO level of the ADS7138
 * @param channel The channel of the GPIO
 * @return The state of the GPIO
 */
void ADS7138writeGPIO(I2C_HandleTypeDef *hi2c, uint8_t i2cAddress, uint8_t channel, GPIO_PinState pinState)
{
  if (pinState == GPIO_PIN_SET)
  {
    setBit(hi2c, i2cAddress, GPO_VALUE_ADDRESS, channel);
  }
  else
  {
    clearBit(hi2c, i2cAddress, GPO_VALUE_ADDRESS, channel);
  }
}

/**
 * @brief Read GPIO level of the ADS7138
 * @param channel The channel of the GPIO
 * @return The state of the GPIO
 */
GPIO_PinState ADS7138readGPIO(I2C_HandleTypeDef *hi2c, uint8_t i2cAddress, uint8_t channel)
{
  uint8_t result;

  /* Check the parameters */
  assert_param(channel);

  result = singleRegisterRead(hi2c, i2cAddress, GPI_VALUE_ADDRESS);
  if ((result & channel) != GPIO_PIN_RESET)
  {
    return GPIO_PIN_SET;
  }
  else
  {
    return GPIO_PIN_RESET;
  }
}

/**
 * @brief Set the ADS7138 to Manual mode
 * @return None
 */
void ADS7138setManualMode(I2C_HandleTypeDef *hi2c, uint8_t i2cAddress)
{
  clearBit(hi2c, i2cAddress, SEQUENCE_CFG_ADDRESS, SEQ_MODE_MASK);
  clearBit(hi2c, i2cAddress, OPMODE_CFG_ADDRESS, CONV_MODE_MASK);
}

/**
 * @brief Set the ADS7138 to Auto-Sequence mode
 * @return None
 */
void ADS7138setAutoSequenceMode(I2C_HandleTypeDef *hi2c, uint8_t i2cAddress)
{
  setBit(hi2c, i2cAddress, SEQUENCE_CFG_ADDRESS, SEQ_MODE_AUTO);
  clearBit(hi2c, i2cAddress, OPMODE_CFG_ADDRESS, CONV_MODE_MASK);
}

/**
 * @brief Set the ADS7138 to Autonomous Mode
 * @return None
 */
void ADS7138setAutonomousMode(I2C_HandleTypeDef *hi2c, uint8_t i2cAddress)
{
  setBit(hi2c, i2cAddress, SEQUENCE_CFG_ADDRESS, SEQ_MODE_AUTO);
  clearBit(hi2c, i2cAddress, OPMODE_CFG_ADDRESS, CONV_MODE_MASK);
}

/**
 * @brief Set the averaging setting of the ADS7138
 * @param averaging The oversampling ratio
 * @return None
 */
void ADS7138setAveraging(I2C_HandleTypeDef *hi2c, uint8_t i2cAddress, ADS7138_Averaging averaging)
{
  singleRegisterWrite(hi2c, i2cAddress, OSR_CFG_ADDRESS, (averaging & OSR_MASK));
}

uint16_t ADS7138readADCChannel(I2C_HandleTypeDef *hi2c, uint8_t i2cAddress, ADS7138_AINTypedef AIN)
{
  uint8_t readBuffer[2];

  singleRegisterWrite(hi2c, i2cAddress, MANUAL_CH_SEL_ADDRESS, (AIN & MANUAL_CHID_MASK));
  HAL_I2C_Master_Receive(hi2c, i2cAddress, readBuffer, 2, I2CTIMEOUT);
  return (uint16_t) ((readBuffer[0] << 4) + (readBuffer[1] >> 4)) & 0xFFF;
}

void ADS7138setAlertCFG(I2C_HandleTypeDef *hi2c, uint8_t i2cAddress, uint8_t driveMethod, uint8_t logic)
{
  // Enable the Alert pin functionality
  //setBit(GENERAL_CFG_ADDRESS, DWC_EN_ENABLE);

  // Set the Alert pin configuration
  singleRegisterWrite(hi2c, i2cAddress, ALERT_PIN_CFG_ADDRESS, ((driveMethod & ALERT_DRIVE_MASK) + (logic & ALERT_LOGIC_MASK)));
}

/**
 * @brief Set the thresholds for triggering the alert pin
 * @param channel Wich channel to set the threshold for
 * @param lowThreshold The low threshold. Max 12 bits
 * @return The data from the specified register
 */
void ADS7138setThreshold(I2C_HandleTypeDef *hi2c, uint8_t i2cAddress, ADS7138_AINTypedef AIN, uint16_t lowThreshold, uint16_t highThreshold, uint8_t eventCount, uint8_t hysteresis)
{
  uint8_t threshold[4] =
  {
      (uint8_t) (((highThreshold & 0x0F) << 4) + (hysteresis & 0x0F)),
      (uint8_t) ((highThreshold & 0xFF0) >> 4),
      (uint8_t) (((lowThreshold & 0x0F) << 4) + (eventCount & 0x0F)),
      (uint8_t) ((lowThreshold & 0xFF0) >> 4)
  };

  //singleRegisterWrite((HYSTERESIS_CH0_ADDRESS + (AIN*4)), threshold[0]);
  //singleRegisterWrite((HYSTERESIS_CH0_ADDRESS + (AIN*4)) + 1, threshold[1]);
  //singleRegisterWrite((HYSTERESIS_CH0_ADDRESS + (AIN*4)) + 2, threshold[2]);
  //singleRegisterWrite((HYSTERESIS_CH0_ADDRESS + (AIN*4)) + 3, threshold[3]);

  multipleRegisterWrite(hi2c, i2cAddress, (HYSTERESIS_CH0_ADDRESS + (AIN*4)), threshold, sizeof(threshold));

  // Enable the AIN channel to assert the alert pin
  setBit(hi2c, i2cAddress, ALERT_CH_SEL_ADDRESS, 1<<AIN);

  // Clear event Flag
  singleRegisterWrite(hi2c, i2cAddress, EVENT_HIGH_FLAG_ADDRESS, 0xFF);
}

bool ADS7138alertHighFlag(I2C_HandleTypeDef *hi2c, uint8_t i2cAddress, ADS7138_AINTypedef channel)
{
  uint8_t flags = singleRegisterRead(hi2c, i2cAddress, EVENT_HIGH_FLAG_ADDRESS);
  if((flags & (1<<channel)) == 0)
    return false;
  else
    return true;
}

