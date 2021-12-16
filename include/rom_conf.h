#if !defined(_ROM_CONF_H_)
#define _ROM_CONF_H_

#include <stdint.h>
#include "config.h"
#ifndef __cplusplus
#include <stdbool.h>
#endif

#define INTERVAL_COUNT 6

#ifndef FW_VERSION_PROTO
#define FW_VERSION_PROTO 1
#endif

#ifndef FW_VERSION_MAJOR
#define FW_VERSION_MAJOR 0
#endif

#ifndef FW_VERSION_MINOR
#define FW_VERSION_MINOR 0
#endif

#ifndef FW_VERSION_PATCH
#define FW_VERSION_PATCH 0
#endif

/*
    16-bit version number:
    [15]      : 0 - development version
                1 - release version
    [14:10]     Major version
    [9:4]       Minor version
    [3:0]       Patch version
*/
typedef struct
{
  uint8_t proto : 1;
  uint8_t major : 5;
  uint8_t minor : 5;
  uint8_t patch : 5;
} version;

struct __attribute__((packed)) rom_conf_t
{
  uint8_t MAGIC[4];
  struct {
    uint8_t MSB;
    uint8_t LSB;
  } HW_VERSION;
  uint8_t APP_EUI[8];
  uint8_t DEV_EUI[8];
  uint8_t APP_KEY[16];
  uint16_t MEASUREMENT_INTERVAL[INTERVAL_COUNT];
  uint8_t USE_TTN_FAIR_USE_POLICY;
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
  uint16_t conf_getMeasurementInterval(uint8_t dr);
  version conf_getHardwareVersion(void);
  version conf_getFirmwareVersion(void);
  uint8_t conf_getUseTTNFairUsePolicy(void);
  uint16_t versionToUint16(version v);

#ifdef __cplusplus
}
#endif

#endif // _ROM_CONF_H_