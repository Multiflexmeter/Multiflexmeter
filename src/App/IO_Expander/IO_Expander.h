/**
  ******************************************************************************
  * @file           : IO_Expander.h
  * @brief          : Header for IO_Expander.c file.
  * @author         : P.Kwekkeboom
  * @date           : Oct 04, 2023
  * @copyright      : 2023 Dekimo Goes
  ******************************************************************************
  */

#ifndef IO_EXPANDER_IO_EXPANDER_H_
#define IO_EXPANDER_IO_EXPANDER_H_

extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;

#define HI2C_IO_EXPANDER_BUS  &hi2c2
#define HI2C_IO_EXPANDER_SYS  &hi2c1

#define ADDRESS_IO_EXPANDER_SYS     0x21
#define ADDRESS_IO_EXPANDER_BUS_INT 0x21
#define ADDRESS_IO_EXPANDER_BUS_EXT 0x20


typedef enum{
  IO_EXPANDER_NONE,
  IO_EXPANDER_SYS,
  IO_EXPANDER_BUS_INT,
  IO_EXPANDER_BUS_EXT,
  NR_IO_EXPANDER,
}ENUM_IO_EXPANDER;


typedef enum
{
  IO_EXP_FRAM_CS,
  IO_EXP_FLASH_SD_CS,
  IO_EXP_FLASH_NOR_CS,
  IO_EXP_GE_EN,
  IO_EXP_VALWAYS_SENS,
  IO_EXP_RST,
  IO_EXP_USB_CONNECTED,
  IO_EXP_SENSOR_INTX,
  IO_EXP_BAT_ALERT,
  IO_EXP_VSENSOR_EN,
  IO_EXP_VSYS_EN,
  IO_EXP_VALWAYS_EN,
  IO_EXP_DEBUG_SW1,
  IO_EXP_DEBUG_SW2,
  IO_EXP_SC_EN,
  IO_EXP_RTC_PWR_EN,
} ENUM_IO_EXPANDER_SYS_PIN;

typedef enum
{
  IO_EXP_LOADSW1,
  IO_EXP_LOADSW2,
  IO_EXP_SLOT1_GPIO0,
  IO_EXP_SLOT1_GPIO1,
  IO_EXP_SLOT1_GPIO2,
  IO_EXP_SLOT2_GPIO0,
  IO_EXP_SLOT2_GPIO1,
  IO_EXP_SLOT2_GPIO2,
  IO_EXP_INT1,
  IO_EXP_INT2,
  IO_EXP_INT3,
  IO_EXP_INT4,
  IO_EXP_INT5,
  IO_EXP_INT6,
  IO_EXP_SPARE1,
  IO_EXP_SPARE2,
} ENUM_IO_EXPANDER_BUS_INT_PIN;

typedef enum
{
  IO_EXP_LOADSW3,
  IO_EXP_LOADSW4,
  IO_EXP_LOADSW5,
  IO_EXP_LOADSW6,
  IO_EXP_SLOT3_GPIO0,
  IO_EXP_SLOT3_GPIO1,
  IO_EXP_SLOT3_GPIO2,
  IO_EXP_SLOT4_GPIO0,
  IO_EXP_SLOT4_GPIO1,
  IO_EXP_SLOT4_GPIO2,
  IO_EXP_SLOT5_GPIO0,
  IO_EXP_SLOT5_GPIO1,
  IO_EXP_SLOT5_GPIO2,
  IO_EXP_SLOT6_GPIO0,
  IO_EXP_SLOT6_GPIO1,
  IO_EXP_SLOT6_GPIO2,
} ENUM_IO_EXPANDER_BUS_EXT_PIN;


typedef enum
{
  IO_EXT_INPUT,
  IO_EXT_OUTPUT,
  MAX_IO_EXT_DIRECTION,
}ENUM_IO_ExtDirection;

typedef enum
{
  IO_EXT_HIGH_ACTIVE,
  IO_EXT_LOW_ACTIVE,
  MAX_IO_EXT_ACTIVE,
}ENUM_IO_ExtACTIVE;

typedef struct{
    bool enabled;
    I2C_HandleTypeDef * I2C_handle;
    uint8_t address;
}struct_IO_ExpanderChipConfig;

int8_t init_IO_ExpanderPin(ENUM_IO_EXPANDER device, ENUM_IO_ExtDirection direction, uint16_t pinMask, ENUM_IO_ExtACTIVE active );
void init_IO_ExpanderData(ENUM_IO_EXPANDER device);
int init_IO_Expander(ENUM_IO_EXPANDER device);
void deinit_IO_Expander(ENUM_IO_EXPANDER device);
void update_IO_Expander(bool input, bool output);
int8_t setOutput_IO_Expander(ENUM_IO_EXPANDER device, uint16_t pinMask, GPIO_PinState state);
int8_t getInput_IO_Expander(ENUM_IO_EXPANDER device, uint16_t pinMask);
int8_t writeOutput_IO_Expander(ENUM_IO_EXPANDER device, uint16_t pinMask, GPIO_PinState state);
int8_t readInput_IO_Expander(ENUM_IO_EXPANDER device, uint16_t pinMask);
uint32_t getUpdateFailedCount_IO_Expander(void);
void resetUpdateFailedCount_IO_Expander(void);

#endif /* IO_EXPANDER_IO_EXPANDER_H_ */
