#include "sensors.h"

#include <avr/io.h>
#include <util/twi.h>
#include <Arduino.h>
#include <Wire.h>
#include "debug.h"
#include "config.h"
#include "smbus.h"

#define CMD_PERFORM 0x10
#define CMD_READ 0x11

error_t sensors_init(void)
{
  return smbus_init();
}

error_t sensors_performMeasurement(void)
{
  return smbus_sendByte(0x36, CMD_PERFORM);
}

error_t sensors_readMeasurement(uint8_t *buf, uint8_t *length)
{
  return smbus_blockRead(0x36, CMD_READ, buf, length);
}

