#include "sensors.h"

#include <avr/io.h>
#include <util/twi.h>
#include <Arduino.h>
#include <Wire.h>
#include "debug.h"
#include "config.h"

void twi_stop(void) {
  TWCR = _BV(TWSTO) | _BV(TWINT) | _BV(TWEN);
}

uint8_t twi_start(uint8_t addr, uint8_t rw) {
  TWCR = _BV(TWSTA) | _BV(TWINT) | _BV(TWEN);
  while ((TWCR & _BV(TWINT)) == 0);
  // If start condition failed, return
  if (TW_STATUS != TW_START && TW_STATUS != TW_REP_START)
    return 0;

  TWDR = (addr << 1) | rw;
  TWCR = _BV(TWINT) | _BV(TWEN);
  while ((TWCR & _BV(TWINT)) == 0);
  // Check if slave ack'ed
  if (TW_STATUS != TW_MT_SLA_ACK && TW_STATUS != TW_MR_SLA_ACK)
    return 0;
  return 1;
}

uint8_t twi_tx(uint8_t data) {
  TWDR = data;
  TWCR = _BV(TWINT) | _BV(TWEN);
  while ((TWCR & _BV(TWINT)) == 0);
  // Check if slave ack'ed
  if (TW_STATUS != TW_MT_DATA_ACK)
    return 0;
  return 1;
}

uint8_t twi_rx(uint8_t *data, uint8_t ack) {
  uint8_t cr = _BV(TWINT) | _BV(TWEN);
  if (ack) {
    cr |= _BV(TWEA);
  }
  TWCR = cr;

  while ((TWCR & _BV(TWINT)) == 0);
  // Check if slave ack'ed
  // if (TW_STATUS != TW_MT_DATA_ACK)
  //   return 0;

  *data = TWDR;
  return 1;
}

void smbus_init(void)
{
  // Clear TWINT after every TWCR!

  // TWCR = Start
  // TWSR Indicate Start
  // TWDR = SLA+RW && TWCR = ? 
  // TWSR is ACK set? TWDR data, TWCR
  // TWSR is data send?
  // TWCR = STOP

  TWBR = ((F_CPU / 80000L) - 16) / 2;
}

void smbus_doMeasurements(void)
{
  // Generate start condition
  if (!twi_start(0x36, TW_WRITE)) return twi_stop();
  if (!twi_tx(0x10)) return twi_stop();
  twi_stop();
}

uint8_t smbus_getMeasurement(uint8_t *buf)
{
  uint8_t byte = 0;
  if (!twi_start(0x36, TW_WRITE)) {
    twi_stop();
    return 0;
  }
  if (!twi_tx(0x11)) {
    twi_stop();
    return 0;
  }
  if (!twi_start(0x36, TW_READ)) {
    twi_stop();
    return 0;
  }

  if (!twi_rx(&byte, 1)) {
    twi_stop();
    return 0;
  }

  uint8_t count = byte;
  for (uint8_t i = 0; i < count; i++)
  {
    if (!twi_rx(&buf[i], (i < count - 1))) {
      twi_stop();
      return 0;
    }
  }
  twi_stop();
  return count;
}
