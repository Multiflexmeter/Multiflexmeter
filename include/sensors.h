#if !defined(_SENSORS_H_)
#define _SENSORS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

  bool initialize_sensors(void);
  void enable_sensors(void);
  void disable_sensors(void);
  float get_air_temperature(void);
  uint16_t get_distance_to_water(void);

#ifdef __cplusplus
}
#endif

#endif // _SENSORS_H_