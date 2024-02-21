/**
  ******************************************************************************
  * @addtogroup     : App
  * @{
  * @file           : led.c
  * @brief          : Board IO functions
  * @author         : P.Kwekkeboom
  * @date           : Oct 11, 2023
  * @}
  ******************************************************************************
  */

#include "main.h"
#include "sys_app.h"

#include "stm32_timer.h"
#include "board_io.h"

#include "led.h"

static UTIL_TIMER_Object_t LedTimer;
static UTIL_TIMER_Time_t LedPeriod = 10; //ms

static struct_LedConfig ledConfig[NR_LED];
static const struct_LedConfig ledConfigDefault[]=
{
    {
        .item = INT_IO_DEBUG_LED1,
        .state = LED_ON,
        .mode = LED_BLINK,
        .toggleInterval = 0,
        .blinkOnTime = 1,
        .blinkOffTime = 199,
        .toggleTimer = 0,
        .toggleState = LED_OFF,
    }
    ,
    {
        .item = INT_IO_DEBUG_LED2,
        .state = LED_OFF,
        .mode = LED_BLINK,
        .toggleInterval = 0,
        .blinkOnTime = 1,
        .blinkOffTime = 99,
        .toggleTimer = 0,
        .toggleState = LED_OFF,
    }
};

/**
 * @fn const void ledInterval(void*)
 * @brief function which control the LED outputs, is triggered by timer
 *
 * @param context
 */
static const void ledInterval(void *context)
{
  int i;
  for (i = 0; i < sizeof(ledConfig) / sizeof(ledConfig[0]); i++)
  {
    if( ledConfig[i].toggleTimer )
    {
      ledConfig[i].toggleTimer--;
    }
    else
    {
      switch( ledConfig[i].mode )
      {
        case LED_ON_OFF:

          writeOutput_board_io( ledConfig[i].item, ledConfig[i].state );

          break;

        case LED_BLINK:

          if( ledConfig[i].toggleState == LED_ON ) //check previous state is LED_ON
          {
            ledConfig[i].toggleInterval = ledConfig[i].blinkOffTime;//next time is off time
          }
          else //check previous state is LED_OFF
          {
            ledConfig[i].toggleInterval = ledConfig[i].blinkOnTime;//next time is on time
          }
          //no break, continue with handling same as toggle

        case LED_TOGGLE:

          ledConfig[i].toggleState = !ledConfig[i].toggleState; //invert current state

          if( ledConfig[i].state == LED_ON )
          {
            writeOutput_board_io( ledConfig[i].item, ledConfig[i].toggleState ); //drive based on toggleState
          }
          else
          {
            writeOutput_board_io( ledConfig[i].item, ledConfig[i].state ); //drive based on state, OFF
          }

          break;
        default:
        case LED_DISABLED:

          //nothing, control by direct I/O

          break;
      }

      ledConfig[i].toggleTimer = ledConfig[i].toggleInterval; //take over new timer for next period
    }
  }
}

/**
 * @fn void initLedTimer(void)
 * @brief function to start LED timer
 *
 */
const void initLedTimer(void)
{
  static bool init = false;
  memcpy(&ledConfig, &ledConfigDefault, sizeof(ledConfig));
  if( init == false )
  {
    UTIL_TIMER_Create(&LedTimer, LedPeriod, UTIL_TIMER_PERIODIC, ledInterval, NULL); //create timer
    UTIL_TIMER_Start(&LedTimer); //start timer
    init = true;
  }
}

/**
 * @fn const void setLed(ENUM_LED, ENUM_LED_MODE, ENUM_LED_STATE, uint32_t, uint32_t, uint32_t)
 * @brief function to configure the led mode / state
 *
 * @param led \ref ENUM_LED
 * @param ledmode \ref ENUM_LED_MODE
 * @param ledstate \ref ENUM_LED_STATE
 * @param interval interval in timer steps \ref LedPeriod
 * @param onTime only for mode LED_BLINK
 * @param offTime only for mode LED_BLINK
 */
const void setLed(ENUM_LED led, ENUM_LED_MODE ledmode, ENUM_LED_STATE ledstate, uint32_t interval, uint32_t onTime, uint32_t offTime)
{
  assert_param(led < NR_LED);
  assert_param(ledmode < NR_LED_MODE);
  assert_param(ledstate < NR_LED_STATE);

  if( led >= NR_LED )
  {
    return;
  }

  if (ledmode >= NR_LED_MODE)
  {
    return;
  }

  if (ledstate >= NR_LED_STATE)
  {
    return;
  }

  switch( ledmode )
  {
    case LED_BLINK:
      ledConfig[led].blinkOnTime = onTime;
      ledConfig[led].blinkOffTime = offTime;
      ledConfig[led].toggleTimer = interval;
      //no break, continue
    case LED_TOGGLE:
      ledConfig[led].toggleInterval = interval;
      //no break, continue
    case LED_ON_OFF:
    default:
      ledConfig[led].mode = ledmode;
      ledConfig[led].state = ledstate;
      ledConfig[led].toggleState = ledstate;
      break;
  }
}

/**
 * @fn const void setGreenLedOnOf(bool)
 * @brief function to swith on/off the green LED
 *
 * @param ledState
 */
const void setGreenLedOnOf(bool ledState)
{
  setLed(LED_1, LED_BLINK, ledState == true ? LED_ON : LED_OFF, 200, 1, 199);
}

/**
 * @fn const void setOrangeLedOnOf(bool)
 * @brief function to swith on/off the green LED
 *
 * @param ledState
 */
const void setOrangeLedOnOf(bool ledState)
{
  setLed(LED_2, LED_BLINK, ledState == true ? LED_ON : LED_OFF, 0, 1, 49);
}

/**
 * @fn const void setLedTest(int8_t)
 * @brief function to set LED test
 *
 * @param test
 */
const void setLedTest(int8_t test)
{
  switch(test)
  {
    case 0: //led test off

      setLed(LED_1, LED_ON_OFF, LED_OFF, 1000, 0, 0);
      setLed(LED_2, LED_ON_OFF, LED_OFF, 1000, 0, 0);

      break;

    case 1: //led test blink

      setLed(LED_1, LED_BLINK, LED_ON, 0,50,50); //Blink led, start directly.
      setLed(LED_2, LED_BLINK, LED_ON, 50,50,50); //Blink led, start after 5x interval

      break;

    case 10: //led normal functionality

      memcpy(&ledConfig, &ledConfigDefault, sizeof(ledConfig));

      break;

    case 99:

      setLed(LED_1, LED_DISABLED, 0, 0,0,0); //Disable LED control, for direct I/O control
      setLed(LED_2, LED_DISABLED, 0, 0,0,0); //Disable LED control, for direct I/O control

      break;

    default:
      break;
  }
}
