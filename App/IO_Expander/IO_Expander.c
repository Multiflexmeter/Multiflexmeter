/**
  ******************************************************************************
  * @addtogroup     : App
  * @{
  * @file           : IO_Expander.c
  * @brief          : IO Expander functions
  * @author         : P.Kwekkeboom
  * @date           : Oct 04, 2023
  * @}
  ******************************************************************************
  */

#include "main.h"
#include "sys_app.h"

#include "TCA9535.h"
#include "IO_Expander.h"

static volatile TCA9535Regs TCA9535_Reg_map[NR_IO_EXPANDER];
const char IO_Expander_name[][16] =
{
    { "System int. bus" },
    { "Sensor int. bus" },
    { "Sensor ext. bus" },
};


/**
 * @brief I/O expander chip definition
 */
static struct_IO_ExpanderChipConfig stIO_ExpanderChipConfig[]=
{
    { //IO_EXPANDER_SYS, U14 on MFM main board
        true,                         //enable at powerup
        HI2C_IO_EXPANDER_SYS,         //I2C handle of I2C system bus
        ADDRESS_IO_EXPANDER_SYS,      //slave address of chip
    }
    ,
    { //IO_EXPANDER_BUS_INT, U15 on MFM main board
        false,                         //disable at powerup
        HI2C_IO_EXPANDER_BUS,         //I2C handle of I2C sensor bus
        ADDRESS_IO_EXPANDER_BUS_INT,  //slave address of chip
    }
    ,
    { //IO_EXPANDER_BUS_EXT, U1 on MFM Expansion board
        false,                         //disable at powerup
        HI2C_IO_EXPANDER_BUS,         //I2C handle of I2C sensor bus
        ADDRESS_IO_EXPANDER_BUS_EXT,  //slave address of chip
    }
};

/**
 * @fn void set_register_IO_Expander(unsigned short*, uint16_t_t)
 * @brief helper function to set pinMask in register
 *
 * @param pRegister
 * @param pinMask
 */
void set_register_IO_Expander( unsigned short * pRegister, uint16_t pinMask )
{
  *pRegister |= pinMask;
}

/**
 * @fn void reset_register_IO_Expander(unsigned short*, uint16_t_t_t)
 * @brief helper function to reset pinMask in register
 *
 * @param pRegister
 * @param pinMask
 */
void reset_register_IO_Expander( unsigned short * pRegister, uint16_t pinMask )
{
  *pRegister &= ~(pinMask);
}


/**
 * @fn int8_t init_IO_ExpanderPin(ENUM_IO_EXPANDER, ENUM_IO_ExtDirection, uint16_t, ENUM_IO_ExtACTIVE)
 * @brief function to initialize one pin
 *
 * @param device number \ref ENUM_IO_EXPANDER (only IO_EXPANDER_SYS, IO_EXPANDER_BUS_INT, IO_EXPANDER_BUS_EXT
 * @param direction \ref ENUM_IO_ExtDirection
 * @param pinMask
 * @param active \ref ENUM_IO_ExtACTIVE
 * @return successful is 0, negative is error
 */
int8_t init_IO_ExpanderPin(ENUM_IO_EXPANDER device, ENUM_IO_ExtDirection direction, uint16_t pinMask, ENUM_IO_ExtACTIVE active )
{
  //check device definition is not valid
  if( device <= IO_EXPANDER_NONE || device >= NR_IO_EXPANDER )
  {
    APP_LOG(TS_OFF, VLEVEL_H, "Wrong definition in stIO_ExpanderPinConfig struct: device (%d) out of range.\r\n", device );
    return -1;
  }

  //check direction definition is not valid
  if( direction >= MAX_IO_EXT_DIRECTION )
  {
    APP_LOG(TS_OFF, VLEVEL_H, "Wrong definition in stIO_ExpanderPinConfig struct: direction (%D) out of range.\r\n", direction );
    return -2;
  }

  //check pin definition is not valid
  if( !IS_GPIO_PIN(pinMask) )
  {
    APP_LOG(TS_OFF, VLEVEL_H, "Wrong definition in stIO_ExpanderPinConfig struct: pinmask (%d) out of range.\r\n", pinMask );
    return -3;
  }

  //check active definition is not valid
  if( active >= MAX_IO_EXT_ACTIVE )
  {
    APP_LOG(TS_OFF, VLEVEL_H, "Wrong definition in stIO_ExpanderPinConfig struct: active (%d) out of range.\r\n", active );
    return -4;
  }


  switch( direction )
  {
    case IO_EXT_INPUT: //direction is IO_INPUT

      //set input
      set_register_IO_Expander((unsigned short *)&TCA9535_Reg_map[device-1].Config.all, pinMask); //set pinMask corresponding pin in CONFIG to 1 = INPUT

      switch( active )
      {
        case IO_EXT_LOW_ACTIVE: //active is LOW_ACTIVE

          set_register_IO_Expander((unsigned short *)&TCA9535_Reg_map[device-1].PolarityInversion.all, pinMask); //set pinMask corresponding pin in POLARITY to 1 is LOW_ACTIVE INPUT

          break;

        case IO_EXT_HIGH_ACTIVE: //active is HIGH_ACTIVE

          reset_register_IO_Expander((unsigned short *)&TCA9535_Reg_map[device-1].PolarityInversion.all, pinMask); //reset pinMask corresponding pin in POLARITY to 0 is HIGH_ACTIVE INPUT

          break;

        default:

          //nothing should all ready been catched
          assert_param(0);
          return -5;

          break;

      }

      break;

    case IO_EXT_OUTPUT: //direction is IO_OUTPUT

      reset_register_IO_Expander((unsigned short *)&TCA9535_Reg_map[device-1].Config.all, pinMask); //reset pinMask corresponding pin in CONFIG to 0 = OUTPUT

      switch( active )
      {
        case IO_EXT_LOW_ACTIVE: //active is LOW_ACTIVE

          set_register_IO_Expander((unsigned short *)&TCA9535_Reg_map[device-1].Output.all, pinMask); //set pin corresponding pinMask in OUTPUT to 1 is LOW_ACTIVE OUTPUT is off

          break;

        case IO_EXT_HIGH_ACTIVE: //active is HIGH_ACTIVE

          reset_register_IO_Expander((unsigned short *)&TCA9535_Reg_map[device-1].Output.all, pinMask); //reset pin corresponding pinMask in OUTPUT to 0  is HIGH_ACTIVE OUTPUT is off

          break;

        default:

          //nothing should all ready been catched
          assert_param(0);
          return -6;

          break;
      }

      break;

    default:

      //nothing should all ready been catched
      assert_param(0);
      return -7;

      break;
  }

  return 0;
}

/**
 * @fn void init_IO_ExpanderData(ENUM_IO_EXPANDER)
 * @brief initialize data of IO_expander and set device handle/address
 *
 * @param device
 */
void init_IO_ExpanderData(ENUM_IO_EXPANDER device)
{
  if( device <= IO_EXPANDER_NONE ||  device >= NR_IO_EXPANDER ) //check boundary device
    return;

  int i = device - 1;

  //initialize data struct
  TCA9535InitDefault((TCA9535Regs*)& TCA9535_Reg_map[i]);

  //configure I2C_handle and device address for IO_EXPANDER_SYS
  TCA9535_Reg_map[i].I2C_handle = stIO_ExpanderChipConfig[i].I2C_handle;
  TCA9535_Reg_map[i].deviceAddress = stIO_ExpanderChipConfig[i].address;;

}

/**
 * @fn void init_IO_Expander(ENUM_IO_EXPANDER)
 * @brief function write initial config register to devices.
 * note: first initialize the registers of variable \ref TCA9535Regs TCA9535_Reg_map[] with \ref init_IO_ExpanderData() and \ref init_IO_ExpanderPin() function
 *
 * @param device
 */
void init_IO_Expander(ENUM_IO_EXPANDER device)
{
  if( device <= IO_EXPANDER_NONE ||  device >= NR_IO_EXPANDER ) //check boundary device
    return;

  int i = device - 1;
  uint8_t result = 0;

  //initialize device
  result = TCA9535InitI2CReg((TCA9535Regs*)&TCA9535_Reg_map[i]); //send initial state to I/O expander.

  if ( result == I2C_OPERATION_SUCCESSFUL )
  {
    stIO_ExpanderChipConfig[i].enabled = true; //device found, enable device
  }
  else
  {
    APP_LOG(TS_OFF, VLEVEL_H, "I/O expander not found: %s on address 0x%02x\r\n", IO_Expander_name[i], TCA9535_Reg_map[i].deviceAddress );
    stIO_ExpanderChipConfig[i].enabled = false; //disable not found, disable device
  }
}

/**
 * @fn void deinit_IO_Expander(ENUM_IO_EXPANDER)
 * @brief function to deinit IO expander.
 *
 * @param device
 */
void deinit_IO_Expander(ENUM_IO_EXPANDER device)
{

  if( device <= IO_EXPANDER_NONE ||  device >= NR_IO_EXPANDER ) //check boundary device
    return;

  int i = device - 1;

  init_IO_ExpanderData(device);

  //initialize device
  TCA9535InitI2CReg((TCA9535Regs*)&TCA9535_Reg_map[i]); //send initial state to I/O expander.

  stIO_ExpanderChipConfig[i].enabled = false; //disable device
}

/**
 * @fn void update_IO_Expander(bool input, bool output)
 * @brief function to update IO expanders, needs to be called periodically
 *
 */
void update_IO_Expander(bool input, bool output)
{
  int i;
  for (i = 0; i < NR_IO_EXPANDER - 1; i++)
  {
    if( stIO_ExpanderChipConfig[i].enabled == true ) //only when device is enabled
    {

      if( output )
      {
        int8_t result = TCA9535WriteOutput((TCA9535Regs*) &TCA9535_Reg_map[i]); //update outputs

        if( result != 0 )
        {
          APP_LOG(TS_OFF, VLEVEL_H, "I/O expander update write failed: %s on address 0x%02x\r\n", IO_Expander_name[i], TCA9535_Reg_map[i].deviceAddress );
        }

      }

      if( input )
      {
        int8_t result = TCA9535ReadInputReg((TCA9535Regs*) &TCA9535_Reg_map[i]); //read inputs

        if( result != 0 )
        {
          APP_LOG(TS_OFF, VLEVEL_H, "I/O expander update read failed: %s on address 0x%02x\r\n", IO_Expander_name[i], TCA9535_Reg_map[i].deviceAddress );
        }
      }
    }
  }
}

/**
 * @fn int8_t setOutput_IO_Expander(ENUM_IO_EXPANDER, uint16_t, bool)
 * @brief function to set output in IO expander variable.
 *
 * @param device
 * @param pinMask
 * @param state
 * @return 0 = successful, negative is error
 */
int8_t setOutput_IO_Expander(ENUM_IO_EXPANDER device, uint16_t pinMask, GPIO_PinState state)
{
  if( device <= IO_EXPANDER_NONE ||  device >= NR_IO_EXPANDER ) //check boundary device
    return -1;

  if( !IS_GPIO_PIN(pinMask) ) //check pinMask
    return -2;

  if( state == GPIO_PIN_SET )
  {
    set_register_IO_Expander((unsigned short *)&TCA9535_Reg_map[device-1].Output.all, pinMask);
  }

  else
  {
    reset_register_IO_Expander((unsigned short *)&TCA9535_Reg_map[device-1].Output.all, pinMask);
  }

  return 0;
}

/**
 * @fn int8_t getInput_IO_Expander(ENUM_IO_EXPANDER, uint16_t)
 * @brief function to get input of IO expander variable
 * note: not forced read from device
 *
 * @param device
 * @param pinMask
 * @return 0 = low, 1 = high, negative is error
 */
int8_t getInput_IO_Expander(ENUM_IO_EXPANDER device, uint16_t pinMask)
{
  if( device <= IO_EXPANDER_NONE ||  device >= NR_IO_EXPANDER ) //check boundary device
    return -1;

  if( !IS_GPIO_PIN(pinMask) ) //check pinMask
    return -2;

  if( TCA9535_Reg_map[device-1].Input.all & pinMask ) //check input pin is high
  {
    return GPIO_PIN_SET;
  }
  else
  {
    return GPIO_PIN_RESET;
  }
}

/**
 * @fn int8_t writeOutput_IO_Expander(ENUM_IO_EXPANDER, uint16_t, bool)
 * @brief function to write I/O to expander
 *
 * @param device
 * @param pinMask
 * @param state false = low, true = high
 * @return 0 = successful, negative is error
 */
int8_t writeOutput_IO_Expander(ENUM_IO_EXPANDER device, uint16_t pinMask, GPIO_PinState state)
{
  if( device <= IO_EXPANDER_NONE || device >= NR_IO_EXPANDER ) //check boundary device
    return -1;

  if( !IS_GPIO_PIN(pinMask) ) //check pinMask
    return -2;

  int8_t result = setOutput_IO_Expander(device, pinMask, state); //set output

  if( result != 0 )
    return -3;

  result = TCA9535WriteOutput((TCA9535Regs*) &TCA9535_Reg_map[device-1]); //update outputs

  if( result != 0 )
  {
    if( device > IO_EXPANDER_NONE )
    {
      APP_LOG(TS_OFF, VLEVEL_H, "I/O expander output write failed: %s on address 0x%02x\r\n", IO_Expander_name[device-1], TCA9535_Reg_map[device-1].deviceAddress );
    }
    return -4; //TCA9535WriteOutput error
  }

  return 0;
}

/**
 * @fn int8_t readInput_IO_Expander(ENUM_IO_EXPANDER, uint16_t)
 * @brief function to read an IO pin with forced read
 *
 * @param device
 * @param pinMask
 * @return 0 = input is low, 1 = input is high, negative is error
 */
int8_t readInput_IO_Expander(ENUM_IO_EXPANDER device, uint16_t pinMask)
{
  if( device <= IO_EXPANDER_NONE || device >= NR_IO_EXPANDER ) //check boundary device
    return -1;

  if( !IS_GPIO_PIN(pinMask) ) //check pinMask
    return -2;

  int8_t result = TCA9535ReadInputReg((TCA9535Regs*)&TCA9535_Reg_map[device-1]); //read input

  if( result != 0 )
  {
    if( device > IO_EXPANDER_NONE )
    {
      APP_LOG(TS_OFF, VLEVEL_H, "I/O expander input read failed: %s on address 0x%02x\r\n", IO_Expander_name[device-1], TCA9535_Reg_map[device-1].deviceAddress );
    }
    return -3; //TCA9535ReadInputReg error
  }

  return getInput_IO_Expander(device,pinMask);
}
