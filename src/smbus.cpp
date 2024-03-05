#include "smbus.h"

#include <avr/io.h>
#include <util/twi.h>

error_t twi_error() {
  switch(TW_STATUS) {
    case TW_MT_SLA_NACK:
    case TW_MT_DATA_NACK:
    case TW_MR_SLA_NACK:
    case TW_MR_DATA_NACK:
      return ERR_SMBUS_SLAVE_NACK;
    case TW_MT_ARB_LOST: // TW_MR_ARB_LOST (same value)
      return ERR_SMBUS_ARB_LOST;
    case TW_NO_INFO:
    case TW_BUS_ERROR:
      return ERR_SMBUS_ERR;
    default:
      return ERR_SMBUS_ERR;
  }
}

void twi_stop(void) {
  TWCR = _BV(TWSTO) | _BV(TWINT) | _BV(TWEN);
}

error_t twi_start(uint8_t addr, uint8_t rw) {
  TWCR = _BV(TWSTA) | _BV(TWINT) | _BV(TWEN);
  while ((TWCR & _BV(TWINT)) == 0);
  // If start condition failed, return
  if (TW_STATUS != TW_START && TW_STATUS != TW_REP_START)
    return twi_error();

  TWDR = (addr << 1) | rw;
  TWCR = _BV(TWINT) | _BV(TWEN);
  while ((TWCR & _BV(TWINT)) == 0);
  // Check if slave ack'ed
  if (TW_STATUS != TW_MT_SLA_ACK && TW_STATUS != TW_MR_SLA_ACK)
    return twi_error();

  return ERR_NONE;
}

error_t twi_tx(uint8_t data) {
  TWDR = data;
  TWCR = _BV(TWINT) | _BV(TWEN);
  while ((TWCR & _BV(TWINT)) == 0);
  // Check if slave ack'ed
  if (TW_STATUS != TW_MT_DATA_ACK)
    return twi_error();
  return ERR_NONE;
}

error_t twi_rx(uint8_t *data, uint8_t ack) {
  uint8_t reg = _BV(TWINT) | _BV(TWEN);
  if (ack) {
    reg |= _BV(TWEA);
  }
  TWCR = reg;

  while ((TWCR & _BV(TWINT)) == 0);
  // Check if slave ack'ed
  // if (TW_STATUS != TW_MT_DATA_ACK)
  //   return 0;

  *data = TWDR;
  return ERR_NONE;
}

#define F_SMBUS 80000L
error_t smbus_init(void) {
  TWBR = ((F_CPU / F_SMBUS) - 16) / 2;
  return ERR_NONE;
}

error_t smbus_sendByte(uint8_t addr, uint8_t byte)
{
  error_t status = ERR_NONE;

  if((status = twi_start(addr, TW_WRITE)) != ERR_NONE) {
    twi_stop();
    return status;
  }
  if((status = twi_tx(byte)) != ERR_NONE) {
    twi_stop();
    return status;
  }
  twi_stop();

  return status;
}

error_t smbus_blockRead(uint8_t addr, uint8_t cmd, uint8_t *rx_buf, uint8_t *rx_length) {
  error_t status = ERR_NONE;

  // Start transaction and write command
  if((status = twi_start(addr, TW_WRITE)) != ERR_NONE) {
    twi_stop();
    return status;
  }
  if((status = twi_tx(cmd)) != ERR_NONE) {
    twi_stop();
    return status;
  }

  // Restart transaction for reading
  if((status = twi_start(addr, TW_READ)) != ERR_NONE) {
    twi_stop();
    return status;
  }
  // Read block
  if((status = twi_rx(rx_length, true)) != ERR_NONE) {
    twi_stop();
    return status;
  }
  for(uint8_t ix = 0; ix < *rx_length; ix++) {
    // Nack the last byte
    uint8_t ack = ix + 1 != *rx_length;
    if((status = twi_rx(&rx_buf[ix], ack)) != ERR_NONE) {
      return status;
    }
  }

  twi_stop();
  return status;
}

error_t smbus_blockWrite(uint8_t addr, uint8_t cmd, uint8_t *tx_buf, uint8_t tx_length) {
    error_t status = ERR_NONE;
  // Start transaction and write command
  if((status = twi_start(addr, TW_WRITE)) != ERR_NONE) {
    twi_stop();
    return status;
  }
  if((status = twi_tx(cmd)) != ERR_NONE) {
    twi_stop();
    return status;
  }
  // write count and write buffer
  if((status = twi_tx(tx_length)) != ERR_NONE) {
    twi_stop();
    return status;
  }
  for(uint8_t ix = 0; ix < tx_length; ix++) {
    if((status = twi_tx(tx_buf[ix])) != ERR_NONE) {
  twi_stop();
      return status;
    }
  }
  twi_stop();
    return status;
}

error_t smbus_alertAddress(uint8_t *addr)
{
  error_t err = ERR_NONE;
  if ((err = twi_start(SMBUS_ADDR_ARA, TW_READ)) != ERR_NONE) {
    twi_stop();
    // If we receive a NAck, that means there is no alert pending
    if (err == ERR_SMBUS_SLAVE_NACK) {
      return ERR_SMBUS_NO_ALERT;
    }
    return err;
  }

  // Receive alert address
  if ((err = twi_rx(addr, false)) != ERR_NONE) {
    twi_stop();
    return err;
  }
  // Remove R/W bit
  *addr >>= 1;
  return err;
}
