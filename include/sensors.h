#if !defined(_SENSORS_H_)
#define _SENSORS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

  void smbus_init(void);
  void smbus_doMeasurements(void);
  uint8_t smbus_getMeasurement(uint8_t *buf);

#ifdef __cplusplus
}
#endif

#endif // _SENSORS_H_