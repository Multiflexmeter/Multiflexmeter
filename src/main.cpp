#include "main.h"

#include <Arduino.h>
#include <avr/power.h>
#include "sensors.h"
#include "rom_conf.h"
#include "debug.h"
#include "board.h"
#include "wdt.h"

#define MEASUREMENT_SEND_DELAY_S 5
#define PING_MEASUREMENT_DELAY_s 25

const lmic_pinmap lmic_pins = {
    .nss = PIN_NSS,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = PIN_RST,
    .dio = {PIN_DIO_0, PIN_DIO_1, PIN_DIO_2},
};

/**
 * 
 */
void setup(void)
{
  board_setup();

  // Serial.begin(115200);
  // Serial.println("Hello world!");

  // sensors_init();
  // sensors_performMeasurement();
  // delay(1000);
  // uint8_t data[16] = {};
  // uint8_t count = 0;
  // error_t err = ERR_NONE;
  // for (;;) {
  //   if ((err = sensors_readMeasurement(data, &count)) != ERR_NONE) {
  //     Serial.printf("ERROR: %d\n", err);
  //   }
  //   delay(200);
  // }

#if defined(DEBUG) || defined(PRINT_BUILD_DATE_TIME)
  Serial.begin(115200);
#endif
#ifdef PRINT_BUILD_DATE_TIME
  Serial.print(F("Build at: "));
  Serial.print(F(__DATE__));
  Serial.print(" ");
  Serial.print(F(__TIME__));
  Serial.print("\n");
  Serial.flush();
#ifndef DEBUG
  Serial.end();
#endif
#endif

  sensors_init();

  // Start the OS and reset the LoRaWAN stack
  os_init();
  LMIC_reset();
  LMIC_setClockError(MAX_CLOCK_ERROR * 2 / 100);

  // Retrieve keys from EEPROM, if it fails we exit the setup and enter low-power mode by scheduling a never ending task
  if (!conf_load())
  {
    _debug(F("Invalid EEPROM, did you flash the EEPROM?\n"));
    os_setCallback(&main_job, FUNC_ADDR(job_error));
    return;
  }

  LMIC_startJoining();
}

void loop(void)
{
  os_runloop_once();
}

void job_reset(osjob_t *job)
{
  _debugTime();
  _debug(F("RESETTING\n"));
  _debugFlush();
  mcu_reset();
}

void job_error(osjob_t *job)
{
  _debug(F("System errored entering sleep!\n"));
  // Loops indefinitely
  os_setTimedCallback(job, os_getTime() + sec2osticks(60), FUNC_ADDR(job_error));
}

/**
 * @brief Verifies if the requested_time is available for the next measurement, otherwise returns the earliest available time.
 * 
 * @param req_time an absolute os_time value indicating when the measurement should be taken 
 * @return ostime_t the absolute os_time of the requested time or the earliest available time
 */
ostime_t getTransmissionTime(ostime_t req_time)
{
  ostime_t now = os_getTime();
  ostime_t earliest_time = LMICbandplan_nextTx(os_getTime());
  if (earliest_time - now > req_time - now)
    return earliest_time;
  return req_time;
}

/**
 * @brief Transmit the firmware and hardware versions
 */
void job_pingVersion(osjob_t *job) 
{
  _debugTime();
  _debug(F("job_pingVersion\n"));
  uint16_t fw = versionToUint16(conf_getFirmwareVersion());
  uint16_t hw = versionToUint16(conf_getHardwareVersion());

  uint8_t data[5] = {
    0x10, // 0x01 on fport 2 indicates a version response
    (uint8_t)(fw >> 8),
    (uint8_t)(fw & 0xFF),
    (uint8_t)(hw >> 8),
    (uint8_t)(hw & 0xFF),
  };
  
  LMIC_setTxData2(2, data, sizeof(data), 0);
  os_setTimedCallback(job, getTransmissionTime(os_getTime() + sec2osticks(PING_MEASUREMENT_DELAY_s)), FUNC_ADDR(job_performMeasurements));
}

/**
 * @brief Notify module to start performing a measurement. Will schedule readout and send.
 */
void job_performMeasurements(osjob_t *job) {
  _debugTime();
  _debug(F("job_performMeasurements\n"));
  sensors_performMeasurement();
  os_setTimedCallback(job, os_getTime() + sec2osticks(MEASUREMENT_SEND_DELAY_S), FUNC_ADDR(job_fetchAndSend));
}

uint8_t dataBuf[32] = {0};
/**
 * @brief Fetches latest measurements from modules and schedules them for transmission
 */
void job_fetchAndSend(osjob_t *job)
{
  _debugTime();
  _debug(F("job_fetchAndSend\n"));

  // Make sure LMIC is not busy with TX/RX and that there is no
  // TX data pending anymore. If so, then this is most likely
  // due to us trying to send before our previous message was
  // transmitted. This happens when trying to sent to frequently.
  if (LMIC.opmode & OP_TXRXPEND)
  {
    _debug(F("TXRX Pending...\n"));
    scheduleNextMeasurement();
    return;
  }

  // Fetch measurements results
  uint8_t count = 0;
  sensors_readMeasurement(dataBuf, &count);

  // Queue transmission
  lmic_tx_error_t err = LMIC_setTxData2(1, dataBuf, count, 0);
  if (err != 0)
  {
    _debugTime();
    _debug(F("TX error occured: "));
    _debug(err);
    _debug("\n");
  }
  scheduleNextMeasurement();
}

/**
 * @brief Schedules the next measurement job based on measurement interval and allowed duty cycle
 */
void scheduleNextMeasurement() {
  // Schedule our next measurement and send
  ostime_t now = os_getTime();

  uint16_t interval = os_getMeasurementInterval(LMIC.datarate);
  // Account for TTN's arbitrary fair use policy
  if (conf_getUseTTNFairUsePolicy()) {
    // We are assuming our average payload size is 12 bytes w/ 12 bytes of MAC overhead
    uint32_t airtime_ms = osticks2ms(calcAirTime(LMIC.rps, 24));
    uint32_t tx_per_day = 30000 / airtime_ms; // Policy states max 30s tx time per day
    uint16_t interval_sec = (uint32_t)86400 / (tx_per_day + 1); // 86400 = 1 day in seconds, +1 to avoid divide by 0
    _debugTime();
    _debug(F("Fair use: "));
    _debug(F("airtime_ms: "));
    _debug(airtime_ms);
    _debug(F(" tx_per_day: "));
    _debug(tx_per_day);
    _debug(F(" interval_sec: "));
    _debug(interval_sec);
    _debug(F("\n"));

    if (interval < interval_sec) {
      interval = interval_sec;
    }
  }

  ostime_t next_send = getTransmissionTime(now + sec2osticks(interval));

  _debugTime();
  _debug("Measurement scheduled: ");
  _debug(next_send - os_getTime());
  _debug("\n");

  os_setTimedCallback(&main_job, next_send, FUNC_ADDR(job_performMeasurements));
}

/*
  Events fired by the LMIC library.
  We are interested in just three:

    - EV_JOINED
        Device just joined through OTAA and can now start the measurement
        and send cycle

    - EV_TXCOMPLETE
        Device just finished sending a measurement

    - EV_JOIN_TXCOMPLETE
        Device could not join through OTAA
*/
void onEvent(ev_t ev)
{
  switch (ev)
  {
  /*
    Fired when the device starts the OTAA join procedure
    This is important when Rejoining after a losing connection
  */
  case EV_JOINING:
    _debugTime();
    _debug(F("EV_JOINING\n"));
    os_clearCallback(&main_job);
    break;

  /*
    Fired when the device succesfully finishes the OTAA join procedure
    and is accepted
  */
  case EV_JOINED:
    _debugTime();
    _debug(F("EV_JOINED\n"));
    LMIC_setLinkCheckMode(0);
    LMIC_setAdrMode(1);
    os_setCallback(&main_job, FUNC_ADDR(job_pingVersion));
    break;

  /*
    Fired when the device has sent an uplink message
    This also includes receiving downlink message in RX1 & 2
  */
  case EV_TXCOMPLETE:
    _debugTime();
    _debug(F("EV_TXCOMPLETE"));
    if (LMIC.dataLen > 0)
    {
      _debug(F(" with "));
      _debug(LMIC.dataLen);
      _debug(F(" bytes RX\n"));
      if (LMIC.frame[LMIC.dataBeg] == 0xDE && LMIC.frame[LMIC.dataBeg + 1] == 0xAD)
      {
        _debugTime();
        _debug(F("Scheduling reset\n"));
        os_setTimedCallback(&main_job, os_getTime() + sec2osticks(5), FUNC_ADDR(job_reset));
        return;
      }
      if (LMIC.frame[LMIC.dataBeg] == 0x10 && LMIC.dataLen >= 3)
      {
        uint16_t interval = LMIC.frame[LMIC.dataBeg + 1] << 8 | LMIC.frame[LMIC.dataBeg + 2];
        _debugTime();
        _debug(F("Changing interval: "));
        _debug(interval);
        _debug(F("\n"));
        conf_setMeasurementInterval(interval);
        conf_save();
        os_clearCallback(&main_job);
        scheduleNextMeasurement();
        return;
      }
    }
    else
    {
      _debug("\n");
    }
    break;

  /*
    Fired when the device has completed a join request but did not 
    receive a (valid) response and thus failed to join
  */
  case EV_JOIN_TXCOMPLETE:
    _debugTime();
    _debug(F("EV_JOIN_TXCOMPLETE\n"));
    break;

  /*
    Fired when the device hasn't heard from the network in 48+4 uplinks...
    Causes the device to rejoin
  */
  case EV_LINK_DEAD:
    _debugTime();
    _debug(F("EV_LINK_DEAD\n"));
    break;

  default:
    _debugTime();
    _debug("EV: ");
    _debug(ev);
    _debug("\n");
    break;
  }
}

/*
  LoRaWAN keys for LMIC library
*/
void os_getArtEui(uint8_t *buf)
{
  conf_getAppEui(buf);
}

void os_getDevEui(uint8_t *buf)
{
  conf_getDevEui(buf);
}

void os_getDevKey(uint8_t *buf)
{
  conf_getAppKey(buf);
}

uint16_t os_getMeasurementInterval(uint8_t dr)
{
  return conf_getMeasurementInterval();
}