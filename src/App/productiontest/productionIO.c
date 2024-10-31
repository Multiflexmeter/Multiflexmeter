#include "productionIO.h"

#define IO_EXPANDER_SYS  &hi2c1

static struct_ProductionIO prodIO_PinConfig[]=
{
    {BLANK, IO_EXPANDER_SYS, ADS7138_U9,  ADS7138_CHANNEL0, CONFIG_ADC, 0, 0, 0}, // GPIO0/AIN0
    {BLANK, IO_EXPANDER_SYS, ADS7138_U9,  ADS7138_CHANNEL1, CONFIG_ADC, 0, 0, 0}, // GPIO1/AIN1
    {BLANK, IO_EXPANDER_SYS, ADS7138_U9,  ADS7138_CHANNEL2, CONFIG_ADC, 0, 0, 0}, // GPIO2/AIN2
    {BLANK, IO_EXPANDER_SYS, ADS7138_U9,  ADS7138_CHANNEL3, CONFIG_ADC, 0, 0, 0}, // GPIO3/AIN3
    {BLANK, IO_EXPANDER_SYS, ADS7138_U9,  ADS7138_CHANNEL4, CONFIG_ADC, 0, 0, 0}, // GPIO4/AIN4
    {BLANK, IO_EXPANDER_SYS, ADS7138_U9,  ADS7138_CHANNEL5, CONFIG_ADC, 0, 0, 0}, // GPIO5/AIN5
    {VBAT,  IO_EXPANDER_SYS, ADS7138_U9,  ADS7138_CHANNEL6, CONFIG_GPIO, GPIO_OUTPUT, OPEN_DRAIN, GPIO_PIN_SET}, // GPIO6/AIN6
    {VBUS,  IO_EXPANDER_SYS, ADS7138_U9,  ADS7138_CHANNEL7, CONFIG_GPIO, GPIO_OUTPUT, PUSH_PULL,  GPIO_PIN_SET}, // GPIO7/AIN7

    {VUSB,  IO_EXPANDER_SYS, ADS7138_U10, ADS7138_CHANNEL0, CONFIG_GPIO, GPIO_OUTPUT, OPEN_DRAIN, GPIO_PIN_SET}, // UV Enable
    {BLANK, IO_EXPANDER_SYS, ADS7138_U10, ADS7138_CHANNEL1, CONFIG_ADC, 0, 0, 0}, // GPIO8/AIN8
    {BLANK, IO_EXPANDER_SYS, ADS7138_U10, ADS7138_CHANNEL2, CONFIG_ADC, 0, 0, 0}, // GPIO9/AIN9
    {INT3,  IO_EXPANDER_SYS, ADS7138_U10, ADS7138_CHANNEL3, CONFIG_GPIO, GPIO_OUTPUT, PUSH_PULL,  GPIO_PIN_RESET}, // GPIO10/AIN10
    {INT4,  IO_EXPANDER_SYS, ADS7138_U10, ADS7138_CHANNEL4, CONFIG_GPIO, GPIO_OUTPUT, PUSH_PULL,  GPIO_PIN_RESET}, // GPIO11/AIN11
    {INT5,  IO_EXPANDER_SYS, ADS7138_U10, ADS7138_CHANNEL5, CONFIG_GPIO, GPIO_OUTPUT, PUSH_PULL,  GPIO_PIN_RESET}, // GPIO12/AIN12
    {INT6,  IO_EXPANDER_SYS, ADS7138_U10, ADS7138_CHANNEL6, CONFIG_GPIO, GPIO_OUTPUT, PUSH_PULL,  GPIO_PIN_RESET}, // GPIO13/AIN13
    {BLANK, IO_EXPANDER_SYS, ADS7138_U10, ADS7138_CHANNEL7, CONFIG_GPIO, GPIO_OUTPUT, PUSH_PULL,  GPIO_PIN_RESET}, // RED LED
};


void productionIO_init(void)
{
  for(uint8_t i=0; i<sizeof(prodIO_PinConfig)/sizeof(prodIO_PinConfig[0]); i++ )
  {
    // If pin needs to be configured as GPIO
    if(prodIO_PinConfig[i].pinconfig == CONFIG_GPIO)
    {
      // Configure as input
      if(prodIO_PinConfig[i].direction == GPIO_INPUT)
        ADS7138setGPIO_Input(prodIO_PinConfig[i].I2C_handle, prodIO_PinConfig[i].address, prodIO_PinConfig[i].channel);

      // Configure as output
      else if(prodIO_PinConfig[i].direction == GPIO_OUTPUT)
      {
        ADS7138setGPIO_Output(prodIO_PinConfig[i].I2C_handle, prodIO_PinConfig[i].address, prodIO_PinConfig[i].channel, prodIO_PinConfig[i].driveMethod);
        ADS7138writeGPIO(prodIO_PinConfig[i].I2C_handle, prodIO_PinConfig[i].address, prodIO_PinConfig[i].channel, prodIO_PinConfig[i].Pinstate);
      }

    }

    // If pin needs to be configured as ADC
    else if(prodIO_PinConfig[i].pinconfig == CONFIG_ADC)
    {
      ADS7138setADC(prodIO_PinConfig[i].I2C_handle, prodIO_PinConfig[i].address, prodIO_PinConfig[i].channel);
    }
  }
}

void productionIOTurnOff(ProductionIO IO)
{
  for(uint8_t i=0; i<sizeof(prodIO_PinConfig)/sizeof(prodIO_PinConfig[0]); i++ )
  {
    if(prodIO_PinConfig[i].name == IO)
    {
      ADS7138writeGPIO(prodIO_PinConfig[i].I2C_handle, prodIO_PinConfig[i].address, prodIO_PinConfig[i].channel, GPIO_PIN_RESET);
      return;
    }
  }
}

void productionIOTurnOn(ProductionIO IO)
{
  for(uint8_t i=0; i<sizeof(prodIO_PinConfig)/sizeof(prodIO_PinConfig[0]); i++ )
  {
    if(prodIO_PinConfig[i].name == IO)
    {
      ADS7138writeGPIO(prodIO_PinConfig[i].I2C_handle, prodIO_PinConfig[i].address, prodIO_PinConfig[i].channel, GPIO_PIN_SET);
      return;
    }
  }
}

