#include "rom_conf.h"

#include <Arduino.h>
#include <EEPROM.h>

static rom_conf_t config = {0};

/**
 * 
 */
bool conf_load(void)
{
  EEPROM.get(0, config);
  return strcmp((const char *)config.MAGIC, "MFM") == 0;
}

/**
 * 
 */
void conf_save(void)
{
  EEPROM.put(0, config);
}

void conf_getAppEui(uint8_t *buf)
{
  memcpy(buf, &config.APP_EUI, 8);
}

void conf_getDevEui(uint8_t *buf)
{
  memcpy(buf, &config.DEV_EUI, 8);
}

void conf_getAppKey(uint8_t *buf)
{
  memcpy(buf, &config.APP_KEY, 16);
}

uint8_t conf_getUseTTNFairUsePolicy() {
  return config.USE_TTN_FAIR_USE_POLICY;
}

/**
 * @brief Get the measurement interval in seconds
 * 
 * @return uint16_t The measurement interval in seconds
 */
uint16_t conf_getMeasurementInterval()
{
  // Set bounds for rom measurement interval, worst-case scenario the device keeps functioning
  if (config.MEASUREMENT_INTERVAL < MIN_INTERVAL)
    return MIN_INTERVAL;
  else if (config.MEASUREMENT_INTERVAL > MAX_INTERVAL)
    return MAX_INTERVAL;
  return config.MEASUREMENT_INTERVAL;
}

/**
 * @brief Returns the firmware version
 * 
 * @return uint16_t 
 */
version conf_getHardwareVersion()
{
  uint16_t v = config.HW_VERSION.MSB << 8 | config.HW_VERSION.LSB;
  return (version){
    .proto = (uint8_t)((v >> 15) & 0x01),
    .major = (uint8_t)((v >> 10) & 0x1F),
    .minor = (uint8_t)((v >> 5) & 0x1F),
    .patch = (uint8_t)((v >> 0) & 0x1F),
  };
}

version conf_getFirmwareVersion()
{
  return (version){
    .proto= FW_VERSION_PROTO,
    .major = FW_VERSION_MAJOR,
    .minor = FW_VERSION_MINOR,
    .patch = FW_VERSION_PATCH,
  };
}

uint16_t versionToUint16(version v)
{
  uint16_t dat = ((v.proto & 0x01) << 15) |
    ((v.major & 0x1F) << 10) |
    ((v.minor & 0x1F) << 4) |
    ((v.patch & 0x1F) << 0);
  return dat;
}