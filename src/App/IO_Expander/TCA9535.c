

#include "main.h"
#include "i2c.h"
#include "TCA9535.h"

extern unsigned char NACK;


/**
 * @fn int8_t I2C_Write(uint16_t, uint16_t, uint8_t, uint8_t*, uint8_t)
 * @brief wrapper function to write a maximum of two bytes to a register address
 *
 * @param Size : number of databytes in data array for I2C write, must be 2 or smaller for this wrapper
 * @param DevAddress : device address of I2C to write to
 * @param registerAddress : register address to write to
 * @param pData : pointer to data array
 * @param offset : offset, not used, must be 0.
 * @return result : negative value is error in arguments, 0 = successful, larger then 0 is HAL error.
 */
int8_t I2C_Write(uint8_t Size, I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t registerAddress, uint8_t * pData, uint8_t offset )
{
  assert_param( Size <= 2 );
  assert_param( offset == 0 );

  UNUSED(offset);

  if( Size > 2 )
       return -1;

  if( offset != 0 )
    return -2;

  uint8_t data[3];

  data[0] = registerAddress;
  data[1] = pData[0];
  data[2] = pData[1];

  return HAL_I2C_Master_Transmit(hi2c, DevAddress, data, sizeof(data), TCA9535_HAL_TIMEOUT);
}

/**
 * @fn int8_t I2C_Read(uint8_t, uint16_t, uint8_t, uint8_t*)
 * @brief wrapper function to read maximum of two bytes of a register address
 *
 * @param Size : number of databytes two be read from register address, must be 2 or smaller
 * @param DevAddress : device address of I2C to write to
 * @param registerAddress : register address to write to
 * @param pData : pointer to data array for storing the readed data.
 * @return result : negative value is error in arguments, 0 = successful, larger then 0 is HAL error.
 */
int8_t I2C_Read( uint8_t Size, I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t registerAddress, uint8_t * pData )
{
  assert_param( Size <= 2 );

  if( Size > 2 )
    return -1;

  return HAL_I2C_Mem_Read(hi2c, DevAddress, registerAddress, I2C_MEMADD_SIZE_8BIT, pData, Size, TCA9535_HAL_TIMEOUT);
}

/**
 * @fn void TCA9535InitDefault(TCA9535Regs*)
 * @brief Initializes the confRegs structure which is a local mirror of the TPL7200 config registers
 *  additional TCA9535InitI2CReg(confRegs) function call is needed in order to get the registers updated
 *
 * @param Regs : I/O expander struct
 */
void TCA9535InitDefault(TCA9535Regs* Regs)
{
	Regs->Output.all = 0xFFFF;
	Regs->PolarityInversion.all = 0x0000;
	Regs->Config.all = 0x0000;
}

/**
 * @fn unsigned char TCA9535InitI2CReg(TCA9535Regs*)
 * @brief function to initialize the I/O expander config, output and polarity registers
 *
 * @param Regs : I/O expander struct
 * @return : negative value is error in arguments, 0 = successful, larger then 0 is HAL error \ref HAL_StatusTypeDef
 */
unsigned char TCA9535InitI2CReg(TCA9535Regs* Regs)
{
	int8_t result;

	result = I2C_Write(2, Regs->I2C_handle, Regs->deviceAddress<<1, TCA9535_CONFIG_REG0, (unsigned char*)&Regs->Config, 0);

	if( result != 0 )
	  return I2C_OPERATION_FAIL;

	result = I2C_Write(2, Regs->I2C_handle, Regs->deviceAddress<<1, TCA9535_OUTPUT_REG0, (unsigned char*)&Regs->Output, 0);

	if( result != 0 )
	    return I2C_OPERATION_FAIL;

	result = I2C_Write(2, Regs->I2C_handle, Regs->deviceAddress<<1, TCA9535_POLARITY_REG0, (unsigned char*)&Regs->PolarityInversion, 0);

	if( result != 0 )
	    return I2C_OPERATION_FAIL;

	return I2C_OPERATION_SUCCESSFUL;
}

/**
 * @fn int8_t TCA9535ReadInputReg(TCA9535Regs*)
 * @brief function to read the I/O expander input register
 *
 * @param Regs : I/O expander struct
 * @return : negative value is error in arguments, 0 = successful, larger then 0 is HAL error \ref HAL_StatusTypeDef
 */
int8_t TCA9535ReadInputReg(TCA9535Regs* Regs)
{
	return I2C_Read(2, Regs->I2C_handle, Regs->deviceAddress<<1, TCA9535_INPUT_REG0, (unsigned char*)&Regs->Input);
}

/**
 * @fn int8_t TCA9535WriteConfig(TCA9535Regs*)
 * @brief function to write the configuration register of the I/O expander
 *
 * @param Regs : I/O expander struct
 * @return : negative value is error in arguments, 0 = successful, larger then 0 is HAL error \ref HAL_StatusTypeDef
 */
int8_t TCA9535WriteConfig(TCA9535Regs * Regs)
{
  return I2C_Write(2, Regs->I2C_handle, Regs->deviceAddress<<1, TCA9535_CONFIG_REG0, (unsigned char*)&Regs->Config, 0);
}

/**
 * @fn int8_t TCA9535WriteOutput(TCA9535Regs*)
 * @brief function to write the output register of the I/O expander
 *
 * @param Regs : I/O expander struct
 * @return : negative value is error in arguments, 0 = successful, larger then 0 is HAL error \ref HAL_StatusTypeDef
 */
int8_t TCA9535WriteOutput(TCA9535Regs * Regs)
{
  return I2C_Write(2, Regs->I2C_handle, Regs->deviceAddress<<1, TCA9535_OUTPUT_REG0, (unsigned char*)&Regs->Output, 0);
}

/**
 * @fn int8_t TCA9535WritePolarity(TCA9535Regs*)
 * @brief function to write the Polarity register of the I/O expander
 *
 * @param Regs : I/O expander struct
 * @return : negative value is error in arguments, 0 = successful, larger then 0 is HAL error \ref HAL_StatusTypeDef
 */
int8_t TCA9535WritePolarity(TCA9535Regs * Regs)
{
  return I2C_Write(2, Regs->I2C_handle, Regs->deviceAddress<<1, TCA9535_POLARITY_REG0, (unsigned char*)&Regs->PolarityInversion, 0);
}
