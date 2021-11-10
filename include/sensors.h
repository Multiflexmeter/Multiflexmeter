#if !defined(_SENSORS_H_)
#define _SENSORS_H_

#include <stdint.h>
#include "errors.h"

#ifdef __cplusplus
extern "C"
{
#endif

  error_t sensors_init(void);
  error_t sensors_performMeasurement(void);
  error_t sensors_readMeasurement(uint8_t *buf, uint8_t *length);

#ifdef __cplusplus
}
#endif

#endif // _SENSORS_H_