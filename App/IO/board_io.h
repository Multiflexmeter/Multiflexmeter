/**
  ******************************************************************************
  * @file           : board_io.h
  * @brief          : Header for board_io.c file.
  * @author         : P.Kwekkeboom
  * @date           : Oct 09, 2023
  ******************************************************************************
  */

#ifndef IO_BOARD_IO_H_
#define IO_BOARD_IO_H_

#include "../IO_Expander/IO_Expander.h"

typedef enum
{
  INT_IO_MCU_IRQ_DETECT,
  INT_IO_DEBUG_LED2,
  INT_IO_DEBUG_LED1,
  INT_IO_BOX_OPEN,
  INT_IO_VDD_TCXO_EXT,
  INT_IO_ADC_SELECT,
  INT_IO_EXP_IRQ,
  INT_IO_3V3_DETECT,
  INT_IO_RF_SW_CTRL,

  EXT_IO_FRAM_CS,
  EXT_IO_FLASH_SD_CS,
  EXT_IO_FLASH_NOR_CS,
  EXT_IO_GE_EN,
  EXT_IO_BOX_OPEN,
  EXT_IORST,
  EXT_IOUSB_CONNECTED,
  EXT_IOSENSOR_INTX,
  EXT_IOBAT_ALERT,
  EXT_IOVSENSOR_EN,
  EXT_IOVSYS_EN,
  EXT_IOVALWAYS_EN,
  EXT_IODEBUG_SW1,
  EXT_IODEBUG_SW2,
  EXT_IOSC_EN,
  EXT_IORTC_PWR_EN,

  EXT_IOLOADSW1,
  EXT_IOLOADSW2,
  EXT_IOSLOT1_GPIO0,
  EXT_IOSLOT1_GPIO1,
  EXT_IOSLOT1_GPIO2,
  EXT_IOSLOT2_GPIO0,
  EXT_IOSLOT2_GPIO1,
  EXT_IOSLOT2_GPIO2,
  EXT_IOINT1,
  EXT_IOINT2,
  EXT_IOINT3,
  EXT_IOINT4,
  EXT_IOINT5,
  EXT_IOINT6,
  EXT_IOSPARE1,
  EXT_IOSPARE2,

  EXT_IOLOADSW3,
  EXT_IOLOADSW4,
  EXT_IOLOADSW5,
  EXT_IOLOADSW6,
  EXT_IOSLOT3_GPIO0,
  EXT_IOSLOT3_GPIO1,
  EXT_IOSLOT3_GPIO2,
  EXT_IOSLOT4_GPIO0,
  EXT_IOSLOT4_GPIO1,
  EXT_IOSLOT4_GPIO2,
  EXT_IOSLOT5_GPIO0,
  EXT_IOSLOT5_GPIO1,
  EXT_IOSLOT5_GPIO2,
  EXT_IOSLOT6_GPIO0,
  EXT_IOSLOT6_GPIO1,
  EXT_IOSLOT6_GPIO2,

  MAX_IO_ITEM,
} ENUM_IO_ITEM;

typedef enum{
  IO_INTERAL,
  IO_EXTERNAL,

  MAX_IO_TYPE,
} ENUM_IO_LOCATION;

typedef enum
{
  IO_INPUT,
  IO_OUTPUT,
  MAX_IO_DIRECTION,
}ENUM_IO_Direction;

typedef enum
{
  IO_HIGH_ACTIVE,
  IO_LOW_ACTIVE,
  MAX_IO_ACTIVE,
}ENUM_IO_ACTIVE;

typedef struct
{
    ENUM_IO_LOCATION io_location;
    ENUM_IO_EXPANDER device;        //must be IO_EXPANDER_NONE for internal IO
    GPIO_TypeDef *GPIOx;            //must be NULL for external IO
    uint16_t pin;
    uint32_t pullup;                //only for internal pins
    ENUM_IO_Direction direction;
    ENUM_IO_ACTIVE active;

}struct_BoardIO_PinConfig;

const void init_board_io(void);
const void update_board_io(void);
const int8_t setOutput_board_io(ENUM_IO_ITEM item, GPIO_PinState state);
const int8_t getInput_board_io(ENUM_IO_ITEM item);
const int8_t writeOutput_board_io(ENUM_IO_ITEM item, GPIO_PinState state);
const int8_t readInput_board_io(ENUM_IO_ITEM item);
const void toggleOutput_board_io(ENUM_IO_ITEM item);
const bool getLigthSensorStatus(void);

const void enableVsys(void);
const void disableVsys(void);

#endif /* IO_BOARD_IO_H_ */
