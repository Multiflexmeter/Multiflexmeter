#include "sensors.h"

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SoftwareSerial.h>
#include "debug.h"
#include "config.h"

static OneWire wire(PIN_ONE_WIRE);
static DallasTemperature wire_sensors(&wire);
DeviceAddress temp_sensor_addr;

SoftwareSerial JSN(PIN_JSN_TX, PIN_JSN_RX);

bool initialize_sensors(void)
{
  pinMode(PIN_JSN_SDN, OUTPUT);
  wire_sensors.begin();

  // Find address for temp sensor
  if (!wire_sensors.getAddress(temp_sensor_addr, 0))
  {
    _debug(F("Could not find temperature sensor on index 0\r\n"));
    return false;
  }

  return true;
}

void enable_sensors(void)
{
  digitalWrite(PIN_JSN_SDN, HIGH);
  digitalWrite(PIN_JSN_TX, HIGH);
  digitalWrite(PIN_JSN_RX, HIGH);
  delay(100);
  JSN.begin(9600);
}

void disable_sensors(void)
{
  JSN.end();
  digitalWrite(PIN_JSN_SDN, LOW);
  digitalWrite(PIN_JSN_TX, LOW);
  digitalWrite(PIN_JSN_RX, LOW);
}

/**
 *  Perform an air temperature measurement and return the results
 */
float get_air_temperature(void)
{
  wire_sensors.requestTemperaturesByAddress(temp_sensor_addr);
  return wire_sensors.getTempC(temp_sensor_addr);
}

/**
 *  Perform a distance measurement and return the results
 */
uint16_t get_distance_to_water(void)
{
  uint8_t buffer[4];

  // send "Start Reading" command to JSN
  JSN.write(0x55);
  JSN.flush();

  // Wait until there is data available or the timeout is reached
  uint32_t timeout_ms = millis() + SENSOR_JSN_TIMEOUT;
  while (!JSN.available() && millis() < timeout_ms)
    ;

  // Nothing received...
  if (JSN.available() <= 0)
  {
    _debug(F("JSN time out\r\n"));
    return -1;
  }

  // There is data availabe, start parsing it.
  JSN.readBytes(buffer, 4);

  // Verify the read data through its checksum
  uint8_t checksum = ((uint16_t)buffer[0] + (uint16_t)buffer[1] + (uint16_t)buffer[2]) & 0xff;
  if (checksum != buffer[3])
  {
    _debug(F("JSN checksum error\r\n"));
    return -2;
  }

  // Calculate distance and convert from mm to cm
  uint16_t distance = (buffer[1] << 8) | buffer[2];
  return distance / 10;
}