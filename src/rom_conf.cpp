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

/**
 * @brief Get the measurement interval in seconds for a specific datarate
 * 
 * @param dr The datarate
 * @return uint16_t The measurement interval in seconds
 */
uint16_t conf_getMeasurementInterval(uint8_t dr)
{
  // Ensure we don't go out of bounds on the array
  if (dr >= INTERVAL_COUNT)
  {
    dr = INTERVAL_COUNT - 1;
  }

  uint16_t interval = config.MEASUREMENT_INTERVAL[dr];

  // In case anything goes wrong
  // measure at least once every 2 hours
  // but no faster than every 15 minutes
  if (interval < MIN_INTERVAL)
    interval = MIN_INTERVAL;
  else if (interval > MAX_INTERVAL)
    interval = MAX_INTERVAL;

  return interval;
}