/**
  ******************************************************************************
  * @file           : led.h
  * @brief          : Header for led.c file.
  * @author         : P.Kwekkeboom
  * @date           : Oct 11, 2023
  ******************************************************************************
  */

#ifndef IO_LED_H_
#define IO_LED_H_

typedef enum
{
  LED_1,
  LED_2,
  NR_LED,
}ENUM_LED;

typedef enum
{
  LED_OFF,
  LED_ON,
  NR_LED_STATE,
}ENUM_LED_STATE;

typedef enum
{
  LED_ON_OFF,
  LED_TOGGLE,
  LED_BLINK,
  LED_DISABLED,
  NR_LED_MODE,
}ENUM_LED_MODE;

typedef struct{
    ENUM_LED_STATE state;
    ENUM_LED_MODE mode;
    ENUM_LED_STATE toggleState;
    uint32_t toggleTimer;
    uint32_t toggleInterval;
    uint32_t blinkOnTime;
    uint32_t blinkOffTime;
    ENUM_IO_ITEM item;
}struct_LedConfig;

const void initLedTimer(void);
const void setLed(ENUM_LED led, ENUM_LED_MODE ledmode, ENUM_LED_STATE ledstate, uint32_t interval, uint32_t onTime, uint32_t offTime);
const void setLedTest(int8_t test);

#endif /* IO_LED_H_ */
