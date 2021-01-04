#include "rom_conf.h"

#include <Arduino.h>
#include <EEPROM.h>

static rom_conf_t config = {0};

/**
 * 
 */
bool conf_load()
{
  EEPROM.get(0, config);
  return strcmp((const char *)config.MAGIC, "MFM") == 0;
}

/**
 * 
 */
void conf_save()
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