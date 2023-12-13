/**
  ******************************************************************************
  * @addtogroup     : App
  * @{
  * @file           : board_io.c
  * @brief          : Board IO functions
  * @author         : P.Kwekkeboom
  * @date           : Oct 09, 2023
  * @}
  ******************************************************************************
  */

#include "main.h"
#include "sys_app.h"

#include "../IO_Expander/IO_Expander.h"
#include "board_io.h"

#define NUCLEO 1
#define DEKIMO 2
//#define TARGET NUCLEO
#define TARGET DEKIMO

#if TARGET == NUCLEO

#include "stm32wlxx_nucleo.h"

#define INT_IO_MCU_IRQ_DETECT_PORT  0
#define INT_IO_DEBUG_LED1_PORT      GPIOB
#define INT_IO_BOX_OPEN_PORT        0
#define INT_IO_VDD_TCXO_EXT_PORT    0
#define INT_IO_ADC_SELECT_PORT      0
#define INT_IO_EXP_IRQ_PORT         0
#define INT_IO_3V3_DETECT_PORT      0
#define INT_IO_RF_SW_PORT           0

#define INT_IO_MCU_IRQ_DETECT_PIN  GPIO_PIN_0
#define INT_IO_DEBUG_LED1_PIN      GPIO_PIN_15
#define INT_IO_BOX_OPEN_PIN        GPIO_PIN_15
#define INT_IO_VDD_TCXO_EXT_PIN    GPIO_PIN_0
#define INT_IO_ADC_SELECT_PIN      GPIO_PIN_5
#define INT_IO_EXP_IRQ_PIN         GPIO_PIN_6
#define INT_IO_3V3_DETECT_PIN      GPIO_PIN_12
#define INT_IO_RF_SW_PIN           GPIO_PIN_13

#else

#define INT_IO_MCU_IRQ_DETECT_PORT  GPIOA
#define INT_IO_DEBUG_LED1_PORT      GPIOA
#define INT_IO_BOX_OPEN_PORT        GPIOA
#define INT_IO_VDD_TCXO_EXT_PORT    GPIOB
#define INT_IO_ADC_SELECT_PORT      GPIOB
#define INT_IO_EXP_IRQ_PORT         GPIOB
#define INT_IO_3V3_DETECT_PORT      GPIOB
#define INT_IO_RF_SW_PORT           GPIOB

#define INT_IO_MCU_IRQ_DETECT_PIN  GPIO_PIN_0
#define INT_IO_DEBUG_LED1_PIN      GPIO_PIN_4
#define INT_IO_BOX_OPEN_PIN        GPIO_PIN_15
#define INT_IO_VDD_TCXO_EXT_PIN    GPIO_PIN_0
#define INT_IO_ADC_SELECT_PIN      GPIO_PIN_5
#define INT_IO_EXP_IRQ_PIN         GPIO_PIN_6
#define INT_IO_3V3_DETECT_PIN      GPIO_PIN_12
#define INT_IO_RF_SW_PIN           GPIO_PIN_13

#endif

static GPIO_PinState board_IO_status[MAX_IO_ITEM];

static struct_BoardIO_PinConfig stIO_PinConfig[]=
{
    { IO_INTERAL,   IO_EXPANDER_NONE, INT_IO_MCU_IRQ_DETECT_PORT, INT_IO_MCU_IRQ_DETECT_PIN,    GPIO_NOPULL,  IO_INPUT,   IO_LOW_ACTIVE},  // INT_IO_MCU_IRQ_DETECT,
    { IO_INTERAL,   IO_EXPANDER_NONE, INT_IO_DEBUG_LED1_PORT,     INT_IO_DEBUG_LED1_PIN,        GPIO_NOPULL,  IO_OUTPUT,  IO_HIGH_ACTIVE}, // INT_IO_DEBUG_LED1
    { IO_INTERAL,   IO_EXPANDER_NONE, INT_IO_BOX_OPEN_PORT,       INT_IO_BOX_OPEN_PIN,          GPIO_NOPULL,  IO_INPUT,   IO_HIGH_ACTIVE}, // INT_IO_BOX_OPEN
    { IO_INTERAL,   IO_EXPANDER_NONE, INT_IO_VDD_TCXO_EXT_PORT,   INT_IO_VDD_TCXO_EXT_PIN,      GPIO_NOPULL,  IO_INPUT,   IO_HIGH_ACTIVE}, // INT_IO_VDD_TCXO_EXT
    { IO_INTERAL,   IO_EXPANDER_NONE, INT_IO_ADC_SELECT_PORT,     INT_IO_ADC_SELECT_PIN,        GPIO_NOPULL,  IO_OUTPUT,  IO_HIGH_ACTIVE}, // INT_IO_ADC_SELECT
    { IO_INTERAL,   IO_EXPANDER_NONE, INT_IO_EXP_IRQ_PORT,        INT_IO_EXP_IRQ_PIN,           GPIO_NOPULL,  IO_INPUT,   IO_LOW_ACTIVE},  // INT_IO_EXP_IRQ
    { IO_INTERAL,   IO_EXPANDER_NONE, INT_IO_3V3_DETECT_PORT,     INT_IO_3V3_DETECT_PIN,        GPIO_NOPULL,  IO_INPUT,   IO_HIGH_ACTIVE}, // INT_IO_3V3_DETECT
    { IO_INTERAL,   IO_EXPANDER_NONE, INT_IO_RF_SW_PORT,          INT_IO_RF_SW_PIN,             GPIO_NOPULL,  IO_OUTPUT,  IO_HIGH_ACTIVE}, // INT_IO_RF_SW_CTRL

    { IO_EXTERNAL,  IO_EXPANDER_SYS,  0,  1UL<<IO_EXP_FRAM_CS,         0, IO_OUTPUT,  IO_LOW_ACTIVE  } ,  // EXT_IO_FRAM_CS
    { IO_EXTERNAL,  IO_EXPANDER_SYS,  0,  1UL<<IO_EXP_FLASH_SD_CS,     0, IO_OUTPUT,  IO_LOW_ACTIVE  } ,  // EXT_IO_FLASH_SD_CS
    { IO_EXTERNAL,  IO_EXPANDER_SYS,  0,  1UL<<IO_EXP_FLASH_NOR_CS,    0, IO_OUTPUT,  IO_LOW_ACTIVE  } ,  // EXT_IO_FLASH_NOR_CS
    { IO_EXTERNAL,  IO_EXPANDER_SYS,  0,  1UL<<IO_EXP_GE_EN,           0, IO_OUTPUT,  IO_HIGH_ACTIVE } ,  // EXT_IO_GE_EN
    { IO_EXTERNAL,  IO_EXPANDER_SYS,  0,  1UL<<IO_EXP_VALWAYS_SENS,    0, IO_INPUT,   IO_HIGH_ACTIVE } ,  // EXT_IO_VALWAYS_SENS
    { IO_EXTERNAL,  IO_EXPANDER_SYS,  0,  1UL<<IO_EXP_RST,             0, IO_OUTPUT,  IO_LOW_ACTIVE  } ,  // EXT_IO_RST
    { IO_EXTERNAL,  IO_EXPANDER_SYS,  0,  1UL<<IO_EXP_USB_CONNECTED,   0, IO_INPUT,   IO_HIGH_ACTIVE } ,  // EXT_IO_USB_CONNECTED
    { IO_EXTERNAL,  IO_EXPANDER_SYS,  0,  1UL<<IO_EXP_SENSOR_INTX,     0, IO_INPUT,   IO_HIGH_ACTIVE } ,  // EXT_IO_SENSOR_INTX
    { IO_EXTERNAL,  IO_EXPANDER_SYS,  0,  1UL<<IO_EXP_BAT_ALERT,       0, IO_INPUT,   IO_LOW_ACTIVE  } ,  // EXT_IO_BAT_ALERT
    { IO_EXTERNAL,  IO_EXPANDER_SYS,  0,  1UL<<IO_EXP_VSENSOR_EN,      0, IO_OUTPUT,  IO_HIGH_ACTIVE } ,  // EXT_IO_VSENSOR_EN
    { IO_EXTERNAL,  IO_EXPANDER_SYS,  0,  1UL<<IO_EXP_VSYS_EN,         0, IO_OUTPUT,  IO_HIGH_ACTIVE } ,  // EXT_IO_VSYS_EN
    { IO_EXTERNAL,  IO_EXPANDER_SYS,  0,  1UL<<IO_EXP_VALWAYS_EN,      0, IO_OUTPUT,  IO_LOW_ACTIVE  } ,  // EXT_IO_VALWAYS_EN
    { IO_EXTERNAL,  IO_EXPANDER_SYS,  0,  1UL<<IO_EXP_DEBUG_SW1,       0, IO_OUTPUT,  IO_HIGH_ACTIVE } ,  // EXT_IO_DEBUG_SW1
    { IO_EXTERNAL,  IO_EXPANDER_SYS,  0,  1UL<<IO_EXP_DEBUG_SW2,       0, IO_OUTPUT,  IO_HIGH_ACTIVE } ,  // EXT_IO_DEBUG_SW2
    { IO_EXTERNAL,  IO_EXPANDER_SYS,  0,  1UL<<IO_EXP_SC_EN,           0, IO_OUTPUT,  IO_LOW_ACTIVE  } ,  // EXT_IO_SC_EN
    { IO_EXTERNAL,  IO_EXPANDER_SYS,  0,  1UL<<IO_EXP_RTC_PWR_EN,      0, IO_INPUT,   IO_LOW_ACTIVE  } ,  // EXT_IO_RTC_PWR_EN

    { IO_EXTERNAL,  IO_EXPANDER_BUS_INT,  0,  1UL<<IO_EXP_LOADSW1,     0, IO_OUTPUT,  IO_HIGH_ACTIVE } ,  // EXT_IO_LOADSW1
    { IO_EXTERNAL,  IO_EXPANDER_BUS_INT,  0,  1UL<<IO_EXP_LOADSW2,     0, IO_OUTPUT,  IO_HIGH_ACTIVE } ,  // EXT_IO_LOADSW2
    { IO_EXTERNAL,  IO_EXPANDER_BUS_INT,  0,  1UL<<IO_EXP_SLOT1_GPIO0, 0, IO_INPUT,   IO_HIGH_ACTIVE } ,  // EXT_IO_SLOT1_GPIO0
    { IO_EXTERNAL,  IO_EXPANDER_BUS_INT,  0,  1UL<<IO_EXP_SLOT1_GPIO1, 0, IO_INPUT,   IO_HIGH_ACTIVE } ,  // EXT_IO_SLOT1_GPIO1
    { IO_EXTERNAL,  IO_EXPANDER_BUS_INT,  0,  1UL<<IO_EXP_SLOT1_GPIO2, 0, IO_INPUT,   IO_HIGH_ACTIVE } ,  // EXT_IO_SLOT1_GPIO2
    { IO_EXTERNAL,  IO_EXPANDER_BUS_INT,  0,  1UL<<IO_EXP_SLOT2_GPIO0, 0, IO_INPUT,   IO_HIGH_ACTIVE } ,  // EXT_IO_SLOT2_GPIO0
    { IO_EXTERNAL,  IO_EXPANDER_BUS_INT,  0,  1UL<<IO_EXP_SLOT2_GPIO1, 0, IO_INPUT,   IO_HIGH_ACTIVE } ,  // EXT_IO_SLOT2_GPIO1
    { IO_EXTERNAL,  IO_EXPANDER_BUS_INT,  0,  1UL<<IO_EXP_SLOT2_GPIO2, 0, IO_INPUT,   IO_HIGH_ACTIVE } ,  // EXT_IO_SLOT2_GPIO2
    { IO_EXTERNAL,  IO_EXPANDER_BUS_INT,  0,  1UL<<IO_EXP_INT1,        0, IO_INPUT,   IO_LOW_ACTIVE  } ,  // EXT_IO_INT1
    { IO_EXTERNAL,  IO_EXPANDER_BUS_INT,  0,  1UL<<IO_EXP_INT2,        0, IO_INPUT,   IO_LOW_ACTIVE  } ,  // EXT_IO_INT2
    { IO_EXTERNAL,  IO_EXPANDER_BUS_INT,  0,  1UL<<IO_EXP_INT3,        0, IO_INPUT,   IO_LOW_ACTIVE  } ,  // EXT_IO_INT3
    { IO_EXTERNAL,  IO_EXPANDER_BUS_INT,  0,  1UL<<IO_EXP_INT4,        0, IO_INPUT,   IO_LOW_ACTIVE  } ,  // EXT_IO_INT4
    { IO_EXTERNAL,  IO_EXPANDER_BUS_INT,  0,  1UL<<IO_EXP_INT5,        0, IO_INPUT,   IO_LOW_ACTIVE  } ,  // EXT_IO_INT5
    { IO_EXTERNAL,  IO_EXPANDER_BUS_INT,  0,  1UL<<IO_EXP_INT6,        0, IO_INPUT,   IO_LOW_ACTIVE  } ,  // EXT_IO_INT6
    { IO_EXTERNAL,  IO_EXPANDER_BUS_INT,  0,  1UL<<IO_EXP_SPARE1,      0, IO_OUTPUT,  IO_HIGH_ACTIVE } ,  // EXT_IO_SPARE1
    { IO_EXTERNAL,  IO_EXPANDER_BUS_INT,  0,  1UL<<IO_EXP_SPARE2,      0, IO_OUTPUT,  IO_HIGH_ACTIVE } ,  // EXT_IO_SPARE2

    { IO_EXTERNAL,  IO_EXPANDER_BUS_EXT,  0,  1UL<<IO_EXP_LOADSW3,     0, IO_OUTPUT,  IO_HIGH_ACTIVE } ,  // EXT_IO_LOADSW3
    { IO_EXTERNAL,  IO_EXPANDER_BUS_EXT,  0,  1UL<<IO_EXP_LOADSW4,     0, IO_OUTPUT,  IO_HIGH_ACTIVE } ,  // EXT_IO_LOADSW4
    { IO_EXTERNAL,  IO_EXPANDER_BUS_EXT,  0,  1UL<<IO_EXP_LOADSW5,     0, IO_OUTPUT,  IO_HIGH_ACTIVE } ,  // EXT_IO_LOADSW5
    { IO_EXTERNAL,  IO_EXPANDER_BUS_EXT,  0,  1UL<<IO_EXP_LOADSW6,     0, IO_OUTPUT,  IO_HIGH_ACTIVE } ,  // EXT_IO_LOADSW6
    { IO_EXTERNAL,  IO_EXPANDER_BUS_EXT,  0,  1UL<<IO_EXP_SLOT3_GPIO0, 0, IO_INPUT,   IO_HIGH_ACTIVE } ,  // EXT_IO_SLOT3_GPIO0
    { IO_EXTERNAL,  IO_EXPANDER_BUS_EXT,  0,  1UL<<IO_EXP_SLOT3_GPIO1, 0, IO_INPUT,   IO_HIGH_ACTIVE } ,  // EXT_IO_SLOT3_GPIO1
    { IO_EXTERNAL,  IO_EXPANDER_BUS_EXT,  0,  1UL<<IO_EXP_SLOT3_GPIO2, 0, IO_INPUT,   IO_HIGH_ACTIVE } ,  // EXT_IO_SLOT3_GPIO2
    { IO_EXTERNAL,  IO_EXPANDER_BUS_EXT,  0,  1UL<<IO_EXP_SLOT4_GPIO0, 0, IO_INPUT,   IO_HIGH_ACTIVE } ,  // EXT_IO_SLOT4_GPIO0
    { IO_EXTERNAL,  IO_EXPANDER_BUS_EXT,  0,  1UL<<IO_EXP_SLOT4_GPIO1, 0, IO_INPUT,   IO_HIGH_ACTIVE } ,  // EXT_IO_SLOT4_GPIO1
    { IO_EXTERNAL,  IO_EXPANDER_BUS_EXT,  0,  1UL<<IO_EXP_SLOT4_GPIO2, 0, IO_INPUT,   IO_HIGH_ACTIVE } ,  // EXT_IO_SLOT4_GPIO2
    { IO_EXTERNAL,  IO_EXPANDER_BUS_EXT,  0,  1UL<<IO_EXP_SLOT5_GPIO0, 0, IO_INPUT,   IO_HIGH_ACTIVE } ,  // EXT_IO_SLOT5_GPIO0
    { IO_EXTERNAL,  IO_EXPANDER_BUS_EXT,  0,  1UL<<IO_EXP_SLOT5_GPIO1, 0, IO_INPUT,   IO_HIGH_ACTIVE } ,  // EXT_IO_SLOT5_GPIO1
    { IO_EXTERNAL,  IO_EXPANDER_BUS_EXT,  0,  1UL<<IO_EXP_SLOT5_GPIO2, 0, IO_INPUT,   IO_HIGH_ACTIVE } ,  // EXT_IO_SLOT5_GPIO2
    { IO_EXTERNAL,  IO_EXPANDER_BUS_EXT,  0,  1UL<<IO_EXP_SLOT6_GPIO0, 0, IO_INPUT,   IO_HIGH_ACTIVE } ,  // EXT_IO_SLOT6_GPIO0
    { IO_EXTERNAL,  IO_EXPANDER_BUS_EXT,  0,  1UL<<IO_EXP_SLOT6_GPIO1, 0, IO_INPUT,   IO_HIGH_ACTIVE } ,  // EXT_IO_SLOT6_GPIO1
    { IO_EXTERNAL,  IO_EXPANDER_BUS_EXT,  0,  1UL<<IO_EXP_SLOT6_GPIO2, 0, IO_INPUT,   IO_HIGH_ACTIVE } ,  // EXT_IO_SLOT6_GPIO2
};

/**
 * @fn const void init_io_internal(ENUM_IO_ITEM)
 * @brief function to initialize an internal I/O pin based on the definition of struct_BoardIO_PinConfig
 *
 * @param item
 */
const void init_io_internal(ENUM_IO_ITEM item)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if( item >= MAX_IO_ITEM )
  {
    APP_LOG(TS_OFF, VLEVEL_H, "Wrong input: item (%d) out of range.\r\n", item );
    assert_param( 0 );
    return;
  }

  if( stIO_PinConfig[item].io_location != IO_INTERAL )
  {
    APP_LOG(TS_OFF, VLEVEL_H, "Wrong call, I/O is not internal\r\n", item );
    assert_param( 0 );
    return;
  }

  memset(&GPIO_InitStruct, 0x00, sizeof(GPIO_InitStruct)); //set struct default value

  if( IS_GPIO_ALL_INSTANCE(stIO_PinConfig[item].GPIOx) ) //check GPIO is set
  {
    GPIO_InitStruct.Pin = stIO_PinConfig[item].pin;      //set PIN
    GPIO_InitStruct.Pull = stIO_PinConfig[item].pullup;  //set Pull
    switch( stIO_PinConfig[item].direction )             //check direction
    {
      case IO_INPUT:
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;       //set input
        break;

      case IO_OUTPUT:
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;   //set normal output
        break;

      default:
        GPIO_InitStruct.Mode = 0;
        break;
    }

    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;      //default low speed

    HAL_GPIO_Init(stIO_PinConfig[item].GPIOx, &GPIO_InitStruct); //init GPIO
  }
  else
  {
    APP_LOG(TS_OFF, VLEVEL_H, "Wrong definition in stIO_PinConfig struct: GPIOx is wrong.\r\n", item );
  }
}

/**
 * @fn const void init_io_external(ENUM_IO_ITEM, bool)
 * @brief function to initialize an internal I/O pin based on the definition of struct_BoardIO_PinConfig
 *
 * @param item
 * @param forceUpdate
 */
const void init_io_external(ENUM_IO_ITEM item, bool forceUpdate)
{
  int8_t result = false;
  if( item >= MAX_IO_ITEM )
  {
    APP_LOG(TS_OFF, VLEVEL_H, "Wrong input: item (%d) out of range.\r\n", item );
    assert_param( 0 );
    return;
  }

  if( stIO_PinConfig[item].io_location != IO_EXTERNAL )
  {
    APP_LOG(TS_OFF, VLEVEL_H, "Wrong call, I/O is not external\r\n", item );
    assert_param( 0 );
    return;
  }

  result = init_IO_ExpanderPin(stIO_PinConfig[item].device, stIO_PinConfig[item].direction, stIO_PinConfig[item].pin, stIO_PinConfig[item].active); //Initialize IO Expander pins in register variables
  if( result < 0 )
  { //error.
    APP_LOG(TS_OFF, VLEVEL_H, "Wrong definition in stIO_PinConfig struct: out of range of item %d.\r\n", item );
    return;
  }

  if( forceUpdate ) //check if output needs to be updated directly
  {
    init_IO_Expander(stIO_PinConfig[item].device);
  }
}

/**
 * @fn bool init_board_io_Int_Ext(int)
 * @brief helper function to init a internal/external I/O item
 *
 * @param item : item number in stIO_PinConfig[] config struct
 * @return true = external, false = internal
 */
static bool init_board_io_Ext_Int(int item)
{
  bool result = false;
  switch (stIO_PinConfig[item].io_location)
  {
    case IO_INTERAL:

      init_io_internal(item); //init internal I/O

      break;

    case IO_EXTERNAL:

      result = true;
      init_io_external(item, false); //init internal I/O, no force update. This is done for all I/O together.

      break;

    default:
      break;
  }
  return result;
}

/**
 * @fn void init_board_io(void)
 * @brief function to init board IO
 *
 */
const void init_board_io(void)
{
  int i;
  bool externalFound = false;

  assert_param( IO_INPUT == IO_EXT_INPUT );   //verify enumeration is equal.
  assert_param( IO_OUTPUT == IO_EXT_OUTPUT ); //verify enumeration is equal.

  assert_param( IO_LOW_ACTIVE == IO_EXT_LOW_ACTIVE );   //verify enumeration is equal.
  assert_param( IO_HIGH_ACTIVE == IO_EXT_HIGH_ACTIVE ); //verify enumeration is equal.

#if TARGET == NUCLEO
  BSP_LED_Init(LED1);
  BSP_LED_Init(LED2);
#endif

  init_IO_ExpanderData(IO_EXPANDER_SYS); //Initialize the data and address of IO Expander devices
  init_IO_ExpanderData(IO_EXPANDER_BUS_INT); //Initialize the data and address of IO Expander devices
  init_IO_ExpanderData(IO_EXPANDER_BUS_EXT); //Initialize the data and address of IO Expander devices

  for(i=0; i<sizeof(stIO_PinConfig)/sizeof(stIO_PinConfig[0]); i++ )
  {
    externalFound |= init_board_io_Ext_Int(i);
  }

  if( externalFound == true )
  {
    init_IO_Expander(IO_EXPANDER_SYS); //Send configuration registers to IO Expander devices.
    writeOutput_board_io(EXT_IOVSYS_EN, GPIO_PIN_SET); //enable VSYS, to operate IO expander on BUS.
    init_IO_Expander(IO_EXPANDER_BUS_INT); //Send configuration registers to IO Expander devices.
    init_IO_Expander(IO_EXPANDER_BUS_EXT); //Send configuration registers to IO Expander devices.
  }
  else
  {
    //nothing
  }
}

/**
 * @fn const void init_board_io_device(ENUM_IO_EXPANDER)
 * @brief function to init I/O of each device separate
 * @note make sure EXT_IOVSYS_EN is enabled for IO_EXPANDER_BUS_INT and IO_EXPANDER_BUS_EXT
 *
 * @param device
 */
const void init_board_io_device(ENUM_IO_EXPANDER device)
{
  int i;
  bool externalFound = false;

  assert_param( device >= IO_EXPANDER_NONE && device < NR_IO_EXPANDER);   //verify enumeration is equal.

  if( device >= NR_IO_EXPANDER )
    return;

  switch( device )
  {
    case IO_EXPANDER_SYS:
      init_IO_ExpanderData(IO_EXPANDER_SYS); //Initialize the data and address of IO Expander devices
      break;
    case IO_EXPANDER_BUS_INT:
      init_IO_ExpanderData(IO_EXPANDER_BUS_INT); //Initialize the data and address of IO Expander devices
      break;
    case IO_EXPANDER_BUS_EXT:
      init_IO_ExpanderData(IO_EXPANDER_BUS_EXT); //Initialize the data and address of IO Expander devices
      break;
    default:
      //nothing
      break;
  }

  for(i=0; i<sizeof(stIO_PinConfig)/sizeof(stIO_PinConfig[0]); i++ )
  {
    //check if device is equal
    if( stIO_PinConfig[i].device == device )
    {
      externalFound |= init_board_io_Ext_Int(i);
    }
  }

  if( externalFound == true )
  {
    switch( device )
    {
      case IO_EXPANDER_SYS:
        init_IO_Expander(IO_EXPANDER_SYS); //Send configuration registers to IO Expander devices.
        break;
      case IO_EXPANDER_BUS_INT:
        init_IO_Expander(IO_EXPANDER_BUS_INT); //Send configuration registers to IO Expander devices.
        break;
      case IO_EXPANDER_BUS_EXT:
        init_IO_Expander(IO_EXPANDER_BUS_EXT); //Send configuration registers to IO Expander devices.
        break;
      default:
        //nothing
        break;
    }
  }
  else
  {
    //nothing
  }
}

/**
 * @fn GPIO_PinState read_IO_internal(ENUM_IO_ITEM)
 * @brief helper function to read an internal IO
 *
 * @param item \ref ENUM_IO_ITEM
 * @return \ref GPIO_PinState
 */
static GPIO_PinState read_IO_internal(ENUM_IO_ITEM item)
{
  assert_param( IS_GPIO_ALL_INSTANCE(stIO_PinConfig[item].GPIOx ) );

  if( IS_GPIO_ALL_INSTANCE(stIO_PinConfig[item].GPIOx) ) //check GPIO is set
  {
    GPIO_PinState pinState = HAL_GPIO_ReadPin(stIO_PinConfig[item].GPIOx, stIO_PinConfig[item].pin ); //read input
    if( stIO_PinConfig[item].active == IO_HIGH_ACTIVE )
    {
      return pinState;
    }
    else //IO_LOW_ACTIVE
    {
      return !pinState; //invert
    }
  }
  else
  {
    return 0; //force off result
  }
}

/**
 * @fn GPIO_PinState write_IO_internal(ENUM_IO_ITEM, GPIO_PinState pinState)
 * @brief helper function to write an internal IO
 *
 * @param item \ref ENUM_IO_ITEM
 */
static void write_IO_internal(ENUM_IO_ITEM item, GPIO_PinState pinState)
{
  assert_param( IS_GPIO_ALL_INSTANCE(stIO_PinConfig[item].GPIOx ) );

  if( stIO_PinConfig[item].active == IO_HIGH_ACTIVE )
  {
    pinState = pinState;
  }
  else //IO_LOW_ACTIVE
  {
    pinState = !pinState; //invert
  }

  if( IS_GPIO_ALL_INSTANCE(stIO_PinConfig[item].GPIOx ) ) //check GPIO is set
  {
    HAL_GPIO_WritePin(stIO_PinConfig[item].GPIOx, stIO_PinConfig[item].pin, pinState ); //write output
  }
}

/**
 * @fn GPIO_PinState toggle_IO_internal(ENUM_IO_ITEM)
 * @brief helper function to toggle an internal IO
 *
 * @param item \ref ENUM_IO_ITEM
 * @return \ref GPIO_PinState
 */
static void toggle_IO_internal(ENUM_IO_ITEM item)
{
  assert_param( IS_GPIO_ALL_INSTANCE(stIO_PinConfig[item].GPIOx ) );

  if( IS_GPIO_ALL_INSTANCE(stIO_PinConfig[item].GPIOx ) ) //check GPIO is set
  {
    HAL_GPIO_TogglePin(stIO_PinConfig[item].GPIOx, stIO_PinConfig[item].pin ); //toggle output
  }
}

/**
 * @fn void update_IO_internal(void)
 * @brief helper function for update IO internal for periodically calling
 *
 */
static void update_IO_internal(void)
{
  int i;

  for (i = 0; i < MAX_IO_ITEM; i++) //loop of all items
  {
    if (stIO_PinConfig[i].io_location == IO_INTERAL) //only for IO_INTERNAL
    {
      switch( stIO_PinConfig[i].direction )
      {
        case IO_INPUT:    //inputs

          board_IO_status[i] = read_IO_internal(i);

          break;

        case IO_OUTPUT:   //outputs

          write_IO_internal(i, board_IO_status[i]);

          break;

        default:      //other
          //nothing

          APP_LOG(TS_OFF, VLEVEL_H, "Wrong definition in stIO_PinConfig struct: direction (%d) out of range of item %d.\r\n", stIO_PinConfig[i].direction, i );

          break;
      }
    }
    else
    {
      //nothing
    }
  }
}

/**
 * @fn void update_IO_external(void)
 * @brief helper function for update IO internal for periodically calling
 *
 */
static void update_IO_external(void)
{
  int i;
  int8_t inputResult;
  GPIO_PinState pinState;

  update_IO_Expander(true, false); //update external inputs only

  for (i = 0; i < MAX_IO_ITEM; i++) //loop of all items
  {
    if (stIO_PinConfig[i].io_location == IO_EXTERNAL) //only for IO_EXTERNAL
    {
      switch( stIO_PinConfig[i].direction )
      {
        case IO_INPUT:    //inputs

          inputResult = getInput_IO_Expander(stIO_PinConfig[i].device, stIO_PinConfig[i].pin); //read input

          if( inputResult >= 0 )
          {
            if( stIO_PinConfig[i].active == IO_HIGH_ACTIVE )  //check input is high active (normal)
            {
              board_IO_status[i] = inputResult;                  //normal
            }
            else //IO_LOW_ACTIVE                              //check input is low active (inverting)
            {
              board_IO_status[i] = !inputResult;                 //invert
            }
          }

          break;

        case IO_OUTPUT:   //outputs

          if( stIO_PinConfig[i].active == IO_HIGH_ACTIVE )    //check input is high active (normal)
          {
            pinState = board_IO_status[i];                    //normal
          }
          else //IO_LOW_ACTIVE                                //check input is low active (inverting)
          {
            pinState = !board_IO_status[i];                   //invert
          }

          setOutput_IO_Expander(stIO_PinConfig[i].device, stIO_PinConfig[i].pin, pinState); //write output

          break;

        default:      //other

          //nothing
          APP_LOG(TS_OFF, VLEVEL_H, "Wrong definition in stIO_PinConfig struct: direction (%d) out of range of item %d.\r\n", stIO_PinConfig[i].direction, i );

          break;
      }
    }
    else
    {
      //nothing
    }
  }

  update_IO_Expander(false, true); //update external outputs only
}

/**
 * @fn void update_board_io(void)
 * @brief  function for update all IO for periodically calling
 *
 */
const void update_board_io(void)
{
  update_IO_internal(); //update internal IO
  update_IO_external(); //update external IO
}

/**
 * @fn int8_t setOutput_board_io(ENUM_IO_ITEM, GPIO_PinState)
 * @brief function updates the output status in buffer which periodically updated to device
 *
 * @param item \ref ENUM_IO_ITEM
 * @param state \ref GPIO_PinState
 * @return 0 = successful, negative is error.
 */
const int8_t setOutput_board_io(ENUM_IO_ITEM item, GPIO_PinState state)
{
  if( item >= MAX_IO_ITEM )
  {
    APP_LOG(TS_OFF, VLEVEL_H, "Wrong input: item (%d) out of range.\r\n", item );
    assert_param( 0 );
    return -1;
  }

  if( stIO_PinConfig[item].direction != IO_OUTPUT )
  {
    APP_LOG(TS_OFF, VLEVEL_H, "Wrong input: direction (%d) is not an output of item %d.\r\n", stIO_PinConfig[item].direction, item);
    assert_param( 0 );
    return -2;
  }

  board_IO_status[item] = state;

  return 0;
}

/**
 * @fn int8_t getInput_board_io(ENUM_IO_ITEM, uint16_t)
 * @brief function reads the state of buffer which periodically updated from device
 *
 * @param item \ref ENUM_IO_ITEM
 * @param pinMask \ref GPIO_PinState
 * @return 0 = successful, negative is error.
 */
const int8_t getInput_board_io(ENUM_IO_ITEM item)
{
  if( item >= MAX_IO_ITEM )
  {
    APP_LOG(TS_OFF, VLEVEL_H, "Wrong input: item (%d) out of range.\r\n", item );
    assert_param( 0 );
    return -1;
  }

  return board_IO_status[item];
}

/**
 * @fn int8_t writeOutput_board_io(ENUM_IO_ITEM, GPIO_PinState)
 * @brief function updates the output state and directly update the output
 *
 * @param item \ref ENUM_IO_ITEM
 * @param state \ref GPIO_PinState
 * @return 0 = successful, negative is error
 */
const int8_t writeOutput_board_io(ENUM_IO_ITEM item, GPIO_PinState state)
{
  int8_t result;

  result = setOutput_board_io(item, state); //set output state in variable
  state = board_IO_status[item];

  if( stIO_PinConfig[item].active == IO_LOW_ACTIVE )
  {
    state=!state; //invert
  }

  if( result >= 0 ) //check result no error
  {

    switch( stIO_PinConfig[item].io_location )
    {
      case IO_INTERAL:    //only for IO_INTERAL

        write_IO_internal(item, state);

        break;

      case IO_EXTERNAL:   //only for IO_EXTERNAL

        writeOutput_IO_Expander(stIO_PinConfig[item].device, stIO_PinConfig[item].pin, state); //write output

        break;

      default:

        //nothing
        APP_LOG(TS_OFF, VLEVEL_H, "Wrong definition in stIO_PinConfig struct: location (%d) out of range of item %d.\r\n", stIO_PinConfig[item].io_location, item );

        break;
    }
  }
  else
  {
    //nothing
  }
  return result;
}

/**
 * @fn int8_t readInput_board_io(ENUM_IO_ITEM)
 * @brief function reads the input state and update the state in data
 *
 * @param item
 * @return 0 = input not active, 1 = input active, negative is error
 */
const int8_t readInput_board_io(ENUM_IO_ITEM item)
{
  int8_t result = -10;

  result = getInput_board_io(item); //only for argument checking, re-use.

  if (result < 0) //check result  error
  {
    return result;
  }

  switch (stIO_PinConfig[item].io_location)
  {
    case IO_INTERAL:    //only for IO_INTERAL

      board_IO_status[item] = read_IO_internal(item);

      break;

    case IO_EXTERNAL:   //only for IO_EXTERNAL

      result = readInput_IO_Expander(stIO_PinConfig[item].device,stIO_PinConfig[item].pin); //no invert in software needed.

      if( result < 0 ) //check on error
      {
        return result;

      }
      else
      {
        board_IO_status[item] = result; //copy result
      }

      break;

    default:

      //nothing
      APP_LOG(TS_OFF, VLEVEL_H, "Wrong definition in stIO_PinConfig struct: location (%d) out of range of item %d.\r\n", stIO_PinConfig[item].io_location, item);

      return result;

      break;
  }

    return board_IO_status[item];
}

/**
 * @fn void toggleOutput_board_io(ENUM_IO_ITEM)
 * @brief toggle function for board IO
 *
 * @param item \ref ENUM_IO_ITEM
 */
const void toggleOutput_board_io(ENUM_IO_ITEM item)
{
  int8_t result = -10;

  result = getInput_board_io(item); //only for argument checking, re-use.

  if (result < 0) //check result  error
  {
    return;
  }

  board_IO_status[item] = !board_IO_status[item];
  switch (stIO_PinConfig[item].io_location)
  {
    case IO_INTERAL:    //only for IO_INTERAL

      toggle_IO_internal(item);

      break;

    case IO_EXTERNAL:   //only for IO_EXTERNAL

      writeOutput_IO_Expander(stIO_PinConfig[item].device, stIO_PinConfig[item].pin, board_IO_status[item]); //write output

      break;

    default:

      //nothing
      APP_LOG(TS_OFF, VLEVEL_H, "Wrong definition in stIO_PinConfig struct: location (%d) out of range of item %d.\r\n", stIO_PinConfig[item].io_location, item);

      return;

      break;
  }
}

/**
 * @fn const void setAsInput(ENUM_IO_ITEM)
 * @brief function to set an I/O as input
 *
 * @param item
 */
const void setAsInput(ENUM_IO_ITEM item)
{

  if (item >= MAX_IO_ITEM)
  {
    APP_LOG(TS_OFF, VLEVEL_H, "Wrong input: item (%d) out of range.\r\n", item);
    assert_param(0);
    return;
  }

  stIO_PinConfig[item].direction = IO_INPUT;


  switch (stIO_PinConfig[item].io_location)
  {
    case IO_INTERAL:    //only for IO_INTERAL

      init_io_internal(item);

      break;

    case IO_EXTERNAL:   //only for IO_EXTERNAL

      init_io_external(item, true);

      break;

    default:

      //nothing
      APP_LOG(TS_OFF, VLEVEL_H, "Wrong definition in stIO_PinConfig struct: location (%d) out of range of item %d.\r\n", stIO_PinConfig[item].io_location, item);

      break;
  }
}

/**
 * @fn const void setAsOutput(ENUM_IO_ITEM)
 * @brief function to set an I/O as output.
 *
 * @param item
 */
const void setAsOutput(ENUM_IO_ITEM item)
{

  if (item >= MAX_IO_ITEM)
  {
    APP_LOG(TS_OFF, VLEVEL_H, "Wrong input: item (%d) out of range.\r\n", item);
    assert_param(0);
    return;
  }

  stIO_PinConfig[item].direction = IO_OUTPUT;

  switch (stIO_PinConfig[item].io_location)
  {
    case IO_INTERAL:    //only for IO_INTERAL

      init_io_internal(item);

      break;

    case IO_EXTERNAL:   //only for IO_EXTERNAL

      init_io_external(item, true);

      break;

    default:

      //nothing
      APP_LOG(TS_OFF, VLEVEL_H, "Wrong definition in stIO_PinConfig struct: location (%d) out of range of item %d.\r\n", stIO_PinConfig[item].io_location, item);

      break;
  }
}
