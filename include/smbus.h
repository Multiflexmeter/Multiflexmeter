#if !defined(_SMBUS_H_)
#define _SMBUS_H_

#include <stdint.h>
#include "errors.h"

#define SMBUS_ADDR_GC 0x00
#define SMBUS_ADDR_ARA 0x0C

#ifdef __cplusplus
extern "C"
{
#endif

  error_t smbus_init(void);
  error_t smbus_sendByte(uint8_t addr, uint8_t byte);
  error_t smbus_blockRead(uint8_t addr, uint8_t cmd, uint8_t *rx_buf, uint8_t *rx_length);
  error_t smbus_blockWrite(uint8_t addr, uint8_t cmd, uint8_t *tx_buf, uint8_t tx_length);
  error_t smbus_alertAddress(uint8_t *addr);

#ifdef __cplusplus
}
#endif

#endif // _SMBUS_H_
