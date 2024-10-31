#ifndef PRODUCTIONTEST_PRODUCTIONIO_H_
#define PRODUCTIONTEST_PRODUCTIONIO_H_

#include "ADS7138.h"
#include "i2c.h"

#define ADS7138_U9  0x20
#define ADS7138_U10 0x2E

typedef enum{
  BLANK,
  INT3 = 3,
  INT4 = 4,
  INT5 = 5,
  INT6 = 6,
  VBUS,
  VBAT,
  VUSB,
}ProductionIO;

typedef enum{
  CONFIG_ADC,
  CONFIG_GPIO,
}ADS7138_PinConfig;

typedef struct{
  ProductionIO name;
  I2C_HandleTypeDef *I2C_handle;
  uint8_t address;
  uint8_t channel;
  ADS7138_PinConfig pinconfig;
  GPIO_Direction direction;
  GPIO_driveMethod driveMethod;
  GPIO_PinState Pinstate;
}struct_ProductionIO;

void productionIO_init(void);
void productionIOTurnOff(ProductionIO IO);
void productionIOTurnOn(ProductionIO IO);


#endif /* PRODUCTIONTEST_PRODUCTIONIO_H_ */
