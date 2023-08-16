/**
 * @file bq35100.c
 *
 * @brief Driver to interface with the bq35100 battery monitor.
 *
 */

#include "../Inc/bq35100.h"

static I2C_HandleTypeDef bq35100Handle;

void bq35100_init(I2C_HandleTypeDef *i2cHandle)
{
  bq35100Handle = *i2cHandle;
}
