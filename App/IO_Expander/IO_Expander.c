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
    { IO_EXPANDER_SYS,  IO_EXP_FRAM_CS,         IO_OUTPUT,  IO_LOW_ACTIVE  } ,
    { IO_EXPANDER_SYS,  IO_EXP_FLASH_SD_CS,     IO_OUTPUT,  IO_LOW_ACTIVE  } ,
    { IO_EXPANDER_SYS,  IO_EXP_FLASH_NOR_CS,    IO_OUTPUT,  IO_LOW_ACTIVE  } ,
    { IO_EXPANDER_SYS,  IO_EXP_GE_EN,           IO_OUTPUT,  IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_SYS,  IO_EXP_BOX_OPEN,        IO_INPUT,   IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_SYS,  IO_EXP_RST,             IO_OUTPUT,  IO_LOW_ACTIVE  } ,
    { IO_EXPANDER_SYS,  IO_EXP_USB_CONNECTED,   IO_INPUT,   IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_SYS,  IO_EXP_SENSOR_INTX,     IO_INPUT,   IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_SYS,  IO_EXP_BAT_ALERT,       IO_INPUT,   IO_LOW_ACTIVE  } ,
    { IO_EXPANDER_SYS,  IO_EXP_VSENSOR_EN,      IO_OUTPUT,  IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_SYS,  IO_EXP_VSYS_EN,         IO_OUTPUT,  IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_SYS,  IO_EXP_VALWAYS_EN,      IO_OUTPUT,  IO_LOW_ACTIVE  } ,
    { IO_EXPANDER_SYS,  IO_EXP_DEBUG_SW1,       IO_OUTPUT,  IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_SYS,  IO_EXP_DEBUG_SW2,       IO_OUTPUT,  IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_SYS,  IO_EXP_SC_EN,           IO_OUTPUT,  IO_LOW_ACTIVE  } ,
    { IO_EXPANDER_SYS,  IO_EXP_RTC_PWR_EN,      IO_INPUT,   IO_LOW_ACTIVE  } ,

    { IO_EXPANDER_BUS_INT,  IO_EXP_LOADSW1,     IO_OUTPUT,  IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_BUS_INT,  IO_EXP_LOADSW2,     IO_OUTPUT,  IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_BUS_INT,  IO_EXP_SLOT1_GPIO0, IO_INPUT,   IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_BUS_INT,  IO_EXP_SLOT1_GPIO1, IO_INPUT,   IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_BUS_INT,  IO_EXP_SLOT1_GPIO2, IO_INPUT,   IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_BUS_INT,  IO_EXP_SLOT2_GPIO0, IO_INPUT,   IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_BUS_INT,  IO_EXP_SLOT2_GPIO1, IO_INPUT,   IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_BUS_INT,  IO_EXP_SLOT2_GPIO2, IO_INPUT,   IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_BUS_INT,  IO_EXP_INT1,        IO_INPUT,   IO_LOW_ACTIVE  } ,
    { IO_EXPANDER_BUS_INT,  IO_EXP_INT2,        IO_INPUT,   IO_LOW_ACTIVE  } ,
    { IO_EXPANDER_BUS_INT,  IO_EXP_INT3,        IO_INPUT,   IO_LOW_ACTIVE  } ,
    { IO_EXPANDER_BUS_INT,  IO_EXP_INT4,        IO_INPUT,   IO_LOW_ACTIVE  } ,
    { IO_EXPANDER_BUS_INT,  IO_EXP_INT5,        IO_INPUT,   IO_LOW_ACTIVE  } ,
    { IO_EXPANDER_BUS_INT,  IO_EXP_INT6,        IO_INPUT,   IO_LOW_ACTIVE  } ,
    { IO_EXPANDER_BUS_INT,  IO_EXP_SPARE1,      IO_OUTPUT,  IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_BUS_INT,  IO_EXP_SPARE2,      IO_OUTPUT,  IO_HIGH_ACTIVE } ,

    { IO_EXPANDER_BUS_EXT,  IO_EXP_LOADSW3,     IO_OUTPUT,  IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_BUS_EXT,  IO_EXP_LOADSW4,     IO_OUTPUT,  IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_BUS_EXT,  IO_EXP_LOADSW5,     IO_OUTPUT,  IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_BUS_EXT,  IO_EXP_LOADSW6,     IO_OUTPUT,  IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_BUS_EXT,  IO_EXP_SLOT3_GPIO0, IO_INPUT,   IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_BUS_EXT,  IO_EXP_SLOT3_GPIO1, IO_INPUT,   IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_BUS_EXT,  IO_EXP_SLOT3_GPIO2, IO_INPUT,   IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_BUS_EXT,  IO_EXP_SLOT4_GPIO0, IO_INPUT,   IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_BUS_EXT,  IO_EXP_SLOT4_GPIO1, IO_INPUT,   IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_BUS_EXT,  IO_EXP_SLOT4_GPIO2, IO_INPUT,   IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_BUS_EXT,  IO_EXP_SLOT5_GPIO0, IO_INPUT,   IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_BUS_EXT,  IO_EXP_SLOT5_GPIO1, IO_INPUT,   IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_BUS_EXT,  IO_EXP_SLOT5_GPIO2, IO_INPUT,   IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_BUS_EXT,  IO_EXP_SLOT6_GPIO0, IO_INPUT,   IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_BUS_EXT,  IO_EXP_SLOT6_GPIO1, IO_INPUT,   IO_HIGH_ACTIVE } ,
    { IO_EXPANDER_BUS_EXT,  IO_EXP_SLOT6_GPIO2, IO_INPUT,   IO_HIGH_ACTIVE } ,
};


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

    //check direction to configure CONFIG register
    if( stIO_ExpanderPinConfig[i].direction == IO_INPUT ) //check direction is IO_INPUT
    {
      //set input
      TCA9535_Reg_map[stIO_ExpanderPinConfig[i].device].Config.all |= (1UL << stIO_ExpanderPinConfig[i].pin); //set pin corresponding pin in CONFIG to 1 = INPUT

      //set active low
      if (stIO_ExpanderPinConfig[i].active == IO_LOW_ACTIVE ) //check active is IO_LOW_ACTIVE, only effects on inputs
      {
        TCA9535_Reg_map[stIO_ExpanderPinConfig[i].device].PolarityInversion.all |= (1UL << stIO_ExpanderPinConfig[i].pin); //set pin corresponding pin in POLARITY to 1 is LOW_ACTIVE INPUT
      }

      else //other is IO_HIGH_ACTIVE (normal)
      {
        TCA9535_Reg_map[stIO_ExpanderPinConfig[i].device].PolarityInversion.all &= ~(1UL << stIO_ExpanderPinConfig[i].pin); //set pin corresponding pin in POLARITY to 0 is HIGH_ACTIVE INPUT
      }
    }

    else //stIO_ExpanderPinConfig[i].direction == IO_OUTPUT //check direction is IO_OUTPUT
    {
      TCA9535_Reg_map[stIO_ExpanderPinConfig[i].device].Config.all &= ~(1UL << stIO_ExpanderPinConfig[i].pin); //set pin corresponding pin in CONFIG to 0 = OUTPUT

      //set default output level
      if (stIO_ExpanderPinConfig[i].active == IO_LOW_ACTIVE  ) //check active is IO_LOW_ACTIVE
      {
        TCA9535_Reg_map[stIO_ExpanderPinConfig[i].device].Output.all |= (1UL << stIO_ExpanderPinConfig[i].pin); //set pin corresponding pin in OUTPUT to 1 is LOW_ACTIVE OUTPUT is off
      }

      else //other is IO_HIGH_ACTIVE (normal)
      {
        TCA9535_Reg_map[stIO_ExpanderPinConfig[i].device].Output.all &= ~(1UL << stIO_ExpanderPinConfig[i].pin); //set pin corresponding pin in OUTPUT to 0  is HIGH_ACTIVE OUTPUT is off
      }
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
