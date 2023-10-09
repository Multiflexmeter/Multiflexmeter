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
        true,                         //enable at powerup
        HI2C_IO_EXPANDER_BUS,         //I2C handle of I2C sensor bus
        ADDRESS_IO_EXPANDER_BUS_INT,  //slave address of chip
    }
    ,
    { //IO_EXPANDER_BUS_EXT, U1 on MFM Expansion board
        true,                         //enable at powerup
        HI2C_IO_EXPANDER_BUS,         //I2C handle of I2C sensor bus
        ADDRESS_IO_EXPANDER_BUS_EXT,  //slave address of chip
    }
};

/**
 * @brief I/O expander pin definition
 */
static struct_IO_ExpanderPinConfig stIO_ExpanderPinConfig[]=
{
    { IO_EXPANDER_SYS,  IO_EXP_FRAM_CS,         IO_OUTPUT,  IO_LOW_ACTIVE  } ,  // IO_FRAM_CS
    { IO_EXPANDER_SYS,  IO_EXP_FLASH_SD_CS,     IO_OUTPUT,  IO_LOW_ACTIVE  } ,  // IO_FLASH_SD_CS
    { IO_EXPANDER_SYS,  IO_EXP_FLASH_NOR_CS,    IO_OUTPUT,  IO_LOW_ACTIVE  } ,  // IO_FLASH_NOR_CS
    { IO_EXPANDER_SYS,  IO_EXP_GE_EN,           IO_OUTPUT,  IO_HIGH_ACTIVE } ,  // IO_GE_EN
    { IO_EXPANDER_SYS,  IO_EXP_BOX_OPEN,        IO_INPUT,   IO_HIGH_ACTIVE } ,  // IO_BOX_OPEN
    { IO_EXPANDER_SYS,  IO_EXP_RST,             IO_OUTPUT,  IO_LOW_ACTIVE  } ,  // IO_RST
    { IO_EXPANDER_SYS,  IO_EXP_USB_CONNECTED,   IO_INPUT,   IO_HIGH_ACTIVE } ,  // IO_USB_CONNECTED
    { IO_EXPANDER_SYS,  IO_EXP_SENSOR_INTX,     IO_INPUT,   IO_HIGH_ACTIVE } ,  // IO_SENSOR_INTX
    { IO_EXPANDER_SYS,  IO_EXP_BAT_ALERT,       IO_INPUT,   IO_LOW_ACTIVE  } ,  // IO_BAT_ALERT
    { IO_EXPANDER_SYS,  IO_EXP_VSENSOR_EN,      IO_OUTPUT,  IO_HIGH_ACTIVE } ,  // IO_VSENSOR_EN
    { IO_EXPANDER_SYS,  IO_EXP_VSYS_EN,         IO_OUTPUT,  IO_HIGH_ACTIVE } ,  // IO_VSYS_EN
    { IO_EXPANDER_SYS,  IO_EXP_VALWAYS_EN,      IO_OUTPUT,  IO_LOW_ACTIVE  } ,  // IO_VALWAYS_EN
    { IO_EXPANDER_SYS,  IO_EXP_DEBUG_SW1,       IO_OUTPUT,  IO_HIGH_ACTIVE } ,  // IO_DEBUG_SW1
    { IO_EXPANDER_SYS,  IO_EXP_DEBUG_SW2,       IO_OUTPUT,  IO_HIGH_ACTIVE } ,  // IO_DEBUG_SW2
    { IO_EXPANDER_SYS,  IO_EXP_SC_EN,           IO_OUTPUT,  IO_LOW_ACTIVE  } ,  // IO_SC_EN
    { IO_EXPANDER_SYS,  IO_EXP_RTC_PWR_EN,      IO_INPUT,   IO_LOW_ACTIVE  } ,  // IO_RTC_PWR_EN

    { IO_EXPANDER_BUS_INT,  IO_EXP_LOADSW1,     IO_OUTPUT,  IO_HIGH_ACTIVE } ,  // IO_LOADSW1
    { IO_EXPANDER_BUS_INT,  IO_EXP_LOADSW2,     IO_OUTPUT,  IO_HIGH_ACTIVE } ,  // IO_LOADSW2
    { IO_EXPANDER_BUS_INT,  IO_EXP_SLOT1_GPIO0, IO_INPUT,   IO_HIGH_ACTIVE } ,  // IO_SLOT1_GPIO0
    { IO_EXPANDER_BUS_INT,  IO_EXP_SLOT1_GPIO1, IO_INPUT,   IO_HIGH_ACTIVE } ,  // IO_SLOT1_GPIO1
    { IO_EXPANDER_BUS_INT,  IO_EXP_SLOT1_GPIO2, IO_INPUT,   IO_HIGH_ACTIVE } ,  // IO_SLOT1_GPIO2
    { IO_EXPANDER_BUS_INT,  IO_EXP_SLOT2_GPIO0, IO_INPUT,   IO_HIGH_ACTIVE } ,  // IO_SLOT2_GPIO0
    { IO_EXPANDER_BUS_INT,  IO_EXP_SLOT2_GPIO1, IO_INPUT,   IO_HIGH_ACTIVE } ,  // IO_SLOT2_GPIO1
    { IO_EXPANDER_BUS_INT,  IO_EXP_SLOT2_GPIO2, IO_INPUT,   IO_HIGH_ACTIVE } ,  // IO_SLOT2_GPIO2
    { IO_EXPANDER_BUS_INT,  IO_EXP_INT1,        IO_INPUT,   IO_LOW_ACTIVE  } ,  // IO_INT1
    { IO_EXPANDER_BUS_INT,  IO_EXP_INT2,        IO_INPUT,   IO_LOW_ACTIVE  } ,  // IO_INT2
    { IO_EXPANDER_BUS_INT,  IO_EXP_INT3,        IO_INPUT,   IO_LOW_ACTIVE  } ,  // IO_INT3
    { IO_EXPANDER_BUS_INT,  IO_EXP_INT4,        IO_INPUT,   IO_LOW_ACTIVE  } ,  // IO_INT4
    { IO_EXPANDER_BUS_INT,  IO_EXP_INT5,        IO_INPUT,   IO_LOW_ACTIVE  } ,  // IO_INT5
    { IO_EXPANDER_BUS_INT,  IO_EXP_INT6,        IO_INPUT,   IO_LOW_ACTIVE  } ,  // IO_INT6
    { IO_EXPANDER_BUS_INT,  IO_EXP_SPARE1,      IO_OUTPUT,  IO_HIGH_ACTIVE } ,  // IO_SPARE1
    { IO_EXPANDER_BUS_INT,  IO_EXP_SPARE2,      IO_OUTPUT,  IO_HIGH_ACTIVE } ,  // IO_SPARE2

    { IO_EXPANDER_BUS_EXT,  IO_EXP_LOADSW3,     IO_OUTPUT,  IO_HIGH_ACTIVE } ,  // IO_LOADSW3
    { IO_EXPANDER_BUS_EXT,  IO_EXP_LOADSW4,     IO_OUTPUT,  IO_HIGH_ACTIVE } ,  // IO_LOADSW4
    { IO_EXPANDER_BUS_EXT,  IO_EXP_LOADSW5,     IO_OUTPUT,  IO_HIGH_ACTIVE } ,  // IO_LOADSW5
    { IO_EXPANDER_BUS_EXT,  IO_EXP_LOADSW6,     IO_OUTPUT,  IO_HIGH_ACTIVE } ,  // IO_LOADSW6
    { IO_EXPANDER_BUS_EXT,  IO_EXP_SLOT3_GPIO0, IO_INPUT,   IO_HIGH_ACTIVE } ,  // IO_SLOT3_GPIO0
    { IO_EXPANDER_BUS_EXT,  IO_EXP_SLOT3_GPIO1, IO_INPUT,   IO_HIGH_ACTIVE } ,  // IO_SLOT3_GPIO1
    { IO_EXPANDER_BUS_EXT,  IO_EXP_SLOT3_GPIO2, IO_INPUT,   IO_HIGH_ACTIVE } ,  // IO_SLOT3_GPIO2
    { IO_EXPANDER_BUS_EXT,  IO_EXP_SLOT4_GPIO0, IO_INPUT,   IO_HIGH_ACTIVE } ,  // IO_SLOT4_GPIO0
    { IO_EXPANDER_BUS_EXT,  IO_EXP_SLOT4_GPIO1, IO_INPUT,   IO_HIGH_ACTIVE } ,  // IO_SLOT4_GPIO1
    { IO_EXPANDER_BUS_EXT,  IO_EXP_SLOT4_GPIO2, IO_INPUT,   IO_HIGH_ACTIVE } ,  // IO_SLOT4_GPIO2
    { IO_EXPANDER_BUS_EXT,  IO_EXP_SLOT5_GPIO0, IO_INPUT,   IO_HIGH_ACTIVE } ,  // IO_SLOT5_GPIO0
    { IO_EXPANDER_BUS_EXT,  IO_EXP_SLOT5_GPIO1, IO_INPUT,   IO_HIGH_ACTIVE } ,  // IO_SLOT5_GPIO1
    { IO_EXPANDER_BUS_EXT,  IO_EXP_SLOT5_GPIO2, IO_INPUT,   IO_HIGH_ACTIVE } ,  // IO_SLOT5_GPIO2
    { IO_EXPANDER_BUS_EXT,  IO_EXP_SLOT6_GPIO0, IO_INPUT,   IO_HIGH_ACTIVE } ,  // IO_SLOT6_GPIO0
    { IO_EXPANDER_BUS_EXT,  IO_EXP_SLOT6_GPIO1, IO_INPUT,   IO_HIGH_ACTIVE } ,  // IO_SLOT6_GPIO1
    { IO_EXPANDER_BUS_EXT,  IO_EXP_SLOT6_GPIO2, IO_INPUT,   IO_HIGH_ACTIVE } ,  // IO_SLOT6_GPIO2
};

/**
 * @fn ENUM_IO_EXPANDER get_IO_ExpanderDeviceFromItem(ENUM_IO_EXPANDER_ITEM)
 * @brief helper function to get the device ID from the given item
 *
 * @param io_item item number \ref ENUM_IO_EXPANDER_ITEM
 * @return device ID \ref ENUM_IO_EXPANDER
 */
ENUM_IO_EXPANDER get_IO_ExpanderDeviceFromItem( ENUM_IO_EXPANDER_ITEM io_item )
{
  assert_param( io_item < MAX_IO_EXPANDER_ITEM );

  if( io_item >= MAX_IO_EXPANDER_ITEM)
    return NR_IO_EXPANDER;

  return stIO_ExpanderPinConfig[io_item].device;
}

/**
 * @fn uint8_t get_IO_ExpanderPinFromItem(ENUM_IO_EXPANDER_ITEM)
 * @brief helper function to get the pin ID on the chip from the given item
 *
 * @param io_item item number \ref ENUM_IO_EXPANDER_ITEM
 * @return pin number on chip
 */
int8_t get_IO_ExpanderPinFromItem( ENUM_IO_EXPANDER_ITEM io_item )
{
  assert_param( io_item < MAX_IO_EXPANDER_ITEM );

  if( io_item >= MAX_IO_EXPANDER_ITEM)
    return -1;

  return stIO_ExpanderPinConfig[io_item].pin;
}

/**
 * @fn uint16_t get_IO_ExpanderPinMaskFromItem(ENUM_IO_EXPANDER_ITEM)
 * @brief helper function to get the pin mask of the given item
 *
 * @param io_item io_item item number \ref ENUM_IO_EXPANDER_ITEM
 * @return pin mask for register
 */
uint16_t get_IO_ExpanderPinMaskFromItem( ENUM_IO_EXPANDER_ITEM io_item )
{
  assert_param( io_item < MAX_IO_EXPANDER_ITEM );

  if( io_item >= MAX_IO_EXPANDER_ITEM)
    return 0;

  return 1UL << get_IO_ExpanderPinFromItem(io_item);
}

/**
 * @fn ENUM_IO_Direction get_IO_ExpanderDirectionFromItem(ENUM_IO_EXPANDER_ITEM)
 * @brief helper function to get the direction of the given item
 *
 * @param io_item io_item item number \ref ENUM_IO_EXPANDER_ITEM
 * @return direction of pin \ref ENUM_IO_Direction
 */
ENUM_IO_Direction get_IO_ExpanderDirectionFromItem( ENUM_IO_EXPANDER_ITEM io_item )
{
  assert_param( io_item < MAX_IO_EXPANDER_ITEM );

  if( io_item >= MAX_IO_EXPANDER_ITEM)
    return MAX_IO_DIRECTION;

  return stIO_ExpanderPinConfig[io_item].direction;
}

/**
 * @fn ENUM_IO_ACTIVE get_IO_ExpanderActiveFromItem(ENUM_IO_EXPANDER_ITEM)
 * @brief helper function to get the active low / active high definition of the given item
 *
 * @param io_item io_item item number \ref ENUM_IO_EXPANDER_ITEM
 * @return active low or active high \ref ENUM_IO_ACTIVE
 */
ENUM_IO_ACTIVE get_IO_ExpanderActiveFromItem( ENUM_IO_EXPANDER_ITEM io_item )
{
  assert_param( io_item < MAX_IO_EXPANDER_ITEM );

  if( io_item >= MAX_IO_EXPANDER_ITEM)
    return MAX_IO_ACTIVE;

  return stIO_ExpanderPinConfig[io_item].active;
}

/**
 * @fn void set_register_IO_Expander(unsigned short*, uint8_t)
 * @brief helper function to set pin in register
 *
 * @param pRegister
 * @param pin
 */
void set_register_IO_Expander( unsigned short * pRegister, uint8_t pin )
{
  *pRegister |= (1UL<<pin);
}

/**
 * @fn void reset_register_IO_Expander(unsigned short*, uint8_t)
 * @brief helper function to reset pin in register
 *
 * @param pRegister
 * @param pin
 */
void reset_register_IO_Expander( unsigned short * pRegister, uint8_t pin )
{
  *pRegister &= ~(1UL<<pin);
}


/**
 * @fn void init_IO_Expander(void)
 * @brief function to initialize PIN configuration to each defined I/O expander chip
 *
 */
void init_IO_Expander(void)
{
  int i;
  uint8_t result;

  //configure handle and device address
  for( i=0; i<NR_IO_EXPANDER; i++ )
  {
    //initialize data struct
    TCA9535InitDefault((TCA9535Regs*)& TCA9535_Reg_map[i]);

    //configure I2C_handle and device address for IO_EXPANDER_SYS
    TCA9535_Reg_map[i].I2C_handle = stIO_ExpanderChipConfig[i].I2C_handle;
    TCA9535_Reg_map[i].deviceAddress = stIO_ExpanderChipConfig[i].address;;

  }

  //configure all port registers
  for(i=0; i<sizeof(stIO_ExpanderPinConfig)/sizeof(stIO_ExpanderPinConfig[0]);i++)
  {
    bool tError = false;

    ENUM_IO_EXPANDER device = get_IO_ExpanderDeviceFromItem(i);         //get device definition from item
    ENUM_IO_Direction direction = get_IO_ExpanderDirectionFromItem(i);  //get direction definition from item
    int8_t pin = get_IO_ExpanderPinFromItem(i);                         //get pin number definition from item
    ENUM_IO_ACTIVE active = get_IO_ExpanderActiveFromItem(i);           //get active definition from item


    //check device definition is not valid
    if( device >= NR_IO_EXPANDER )
    {
      tError = true;
      APP_LOG(TS_OFF, VLEVEL_H, "Wrong definition in stIO_ExpanderPinConfig struct: device out of range of item %d\r\n", i );
    }

    //check direction definition is not valid
    if( direction >= MAX_IO_DIRECTION )
    {
      tError = true;
      APP_LOG(TS_OFF, VLEVEL_H, "Wrong definition in stIO_ExpanderPinConfig struct: direction out of range of item %d\r\n", i );
    }

    //check pin definition is not valid
    if( pin >= 16 )
    {
      tError = true;
      APP_LOG(TS_OFF, VLEVEL_H, "Wrong definition in stIO_ExpanderPinConfig struct: pin out of range of item %d\r\n", i );
    }

    //check active definition is not valid
    if( active >= MAX_IO_ACTIVE )
    {
      tError = true;
      APP_LOG(TS_OFF, VLEVEL_H, "Wrong definition in stIO_ExpanderPinConfig struct: pinmask out of range of item %d\r\n", i );
    }

    //check no error in definition
    if( tError == false )
    {
      switch( direction )
      {
        case IO_INPUT: //direction is IO_INPUT

          //set input
          set_register_IO_Expander((unsigned short *)&TCA9535_Reg_map[device].Config.all, pin); //set pin corresponding pin in CONFIG to 1 = INPUT

          switch( active )
          {
            case IO_LOW_ACTIVE: //active is LOW_ACTIVE

              set_register_IO_Expander((unsigned short *)&TCA9535_Reg_map[device].PolarityInversion.all, pin); //set pin corresponding pin in POLARITY to 1 is LOW_ACTIVE INPUT

              break;

            case IO_HIGH_ACTIVE: //active is HIGH_ACTIVE

              reset_register_IO_Expander((unsigned short *)&TCA9535_Reg_map[device].PolarityInversion.all, pin); //reset pin corresponding pin in POLARITY to 0 is HIGH_ACTIVE INPUT

              break;

            default:

              //nothing should all ready been catched
              assert_param(0);

              break;

          }

          break;

        case IO_OUTPUT: //direction is IO_OUTPUT

          reset_register_IO_Expander((unsigned short *)&TCA9535_Reg_map[device].Config.all, pin); //reset pin corresponding pin in CONFIG to 0 = OUTPUT

          switch( active )
          {
            case IO_LOW_ACTIVE: //active is LOW_ACTIVE

              set_register_IO_Expander((unsigned short *)&TCA9535_Reg_map[device].Output.all, pin); //set pin corresponding pin in OUTPUT to 1 is LOW_ACTIVE OUTPUT is off

              break;

            case IO_HIGH_ACTIVE: //active is HIGH_ACTIVE

              reset_register_IO_Expander((unsigned short *)&TCA9535_Reg_map[device].Output.all, pin); //reset pin corresponding pin in OUTPUT to 0  is HIGH_ACTIVE OUTPUT is off

              break;

            default:

              //nothing should all ready been catched
              assert_param(0);

              break;
          }

          break;

        default:

          //nothing should all ready been catched
          assert_param(0);

          break;
      }
    }

    else
    {
      //noting
    }
  }

  //initialize device
  for( i=0; i<NR_IO_EXPANDER; i++ )
  {
    result = TCA9535InitI2CReg((TCA9535Regs*)&TCA9535_Reg_map[i]); //send initial state to I/O expander.

    if ( result == I2C_OPERATION_SUCCESSFUL )
    {
      stIO_ExpanderChipConfig[i].enabled = true; //device found, enable device
    }
    else
    {
      stIO_ExpanderChipConfig[i].enabled = false; //disable not found, disable device
    }
  }
}

/**
 * @fn void update_IO_Expander(void)
 * @brief function to update IO expanders, needs to be called periodically
 *
 */
void update_IO_Expander(void)
{
  int i;
  for (i = 0; i < NR_IO_EXPANDER; i++)
  {
    if( stIO_ExpanderChipConfig[i].enabled == true ) //only when device is enabled
    {
      TCA9535WriteOutput((TCA9535Regs*) &TCA9535_Reg_map[i]); //update outputs
      TCA9535ReadInputReg((TCA9535Regs*) &TCA9535_Reg_map[i]); //read inputs
    }
  }
}

/**
 * @fn int8_t setOutput(ENUM_IO_EXPANDER_ITEM, bool)
 * @brief function to set output in IO expander variable.
 * note: no update to device.
 *
 * @param io_item item number \ref ENUM_IO_EXPANDER_ITEM
 * @param state
 * @return 0 = successful, negative is error
 */
int8_t setOutput(ENUM_IO_EXPANDER_ITEM io_item, bool state)
{
  if( io_item >= MAX_IO_EXPANDER_ITEM ) //check boundary io_item
    return -1;

  ENUM_IO_EXPANDER device = get_IO_ExpanderDeviceFromItem(io_item);

  if( device >= NR_IO_EXPANDER ) //check boundary device
    return -2;

  if( state == true )
  {
    set_register_IO_Expander((unsigned short *)&TCA9535_Reg_map[device].Output.all, get_IO_ExpanderPinFromItem(io_item));
  }

  else
  {
    reset_register_IO_Expander((unsigned short *)&TCA9535_Reg_map[device].Output.all, get_IO_ExpanderPinFromItem(io_item));
  }

  return 0;
}

/**
 * @fn int8_t getInput(ENUM_IO_EXPANDER_ITEM)
 * @brief function to get input of IO expander variable
 * note: not forced read from device
 *
 * @param io_item item number \ref ENUM_IO_EXPANDER_ITEM
 * @return 0 = low, 1 = high, negative is error
 */
int8_t getInput(ENUM_IO_EXPANDER_ITEM io_item)
{
  if( io_item >= MAX_IO_EXPANDER_ITEM ) //check boundary io_item
     return -1;

  ENUM_IO_EXPANDER device = get_IO_ExpanderDeviceFromItem(io_item);

  if( device >= NR_IO_EXPANDER ) //check boundary device
    return -2;

  if( TCA9535_Reg_map[device].Input.all & get_IO_ExpanderPinMaskFromItem(io_item) ) //check input pin is high
  {
    return true;
  }
  else
  {
    return false;
  }
}

/**
 * @fn int8_t writeIO(ENUM_IO_EXPANDER_ITEM, bool, bool)
 * @brief function to write I/O to expander
 *
 * @param io_item item number \ref ENUM_IO_EXPANDER_ITEM
 * @param state false = low, true = high
 * @return 0 = successful, negative is error
 */
int8_t writeIO(ENUM_IO_EXPANDER_ITEM io_item, bool state)
{
  if( io_item >= MAX_IO_EXPANDER_ITEM ) //check boundary io_item
    return -1;

  ENUM_IO_EXPANDER device = get_IO_ExpanderDeviceFromItem(io_item);

  if( device >= NR_IO_EXPANDER ) //check boundary device
    return -2;

  setOutput(io_item, state); //set output

  TCA9535WriteOutput((TCA9535Regs*) &TCA9535_Reg_map[device]); //update outputs

  return 0;
}

/**
 * @fn int8_t readIO(ENUM_IO_EXPANDER_ITEM)
 * @brief function to read an IO pin with forced read
 *
 * @param io_item item number \ref ENUM_IO_EXPANDER_ITEM
 * @return 0 = input is low, 1 = input is high, negative is error
 */
int8_t readIO(ENUM_IO_EXPANDER_ITEM io_item)
{
  if( io_item >= MAX_IO_EXPANDER_ITEM ) //check boundary io_item
    return -1;

  ENUM_IO_EXPANDER device = get_IO_ExpanderDeviceFromItem(io_item);

  if( device >= NR_IO_EXPANDER ) //check boundary device
    return -2;

  TCA9535ReadInputReg((TCA9535Regs*)&TCA9535_Reg_map[device]); //read input

  return getInput(io_item);
}
