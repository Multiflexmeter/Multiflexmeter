#if !defined(_ROM_CONF_H_)
#define _ROM_CONF_H_

#include <stdint.h>

struct __attribute__((packed)) rom_conf_t
{
  uint8_t MAGIC[4];
  uint16_t VERSION;
  uint8_t APP_EUI[8];
  uint8_t DEV_EUI[8];
  uint8_t APP_KEY[16];
  uint32_t MEASUREMENT_INTERVAL;
};

#ifdef __cplusplus
extern "C"
{
#endif

  bool conf_load(void);
  void conf_save(void);
  void conf_getAppEui(uint8_t *buf);
  void conf_getDevEui(uint8_t *buf);
  void conf_getAppKey(uint8_t *buf);
  uint32_t conf_getMeasurementInterval(void);

#ifdef __cplusplus
}
#endif

#endif // _ROM_CONF_H_