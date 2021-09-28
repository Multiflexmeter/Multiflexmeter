#include "main.h"

#include <Arduino.h>
#include <avr/power.h>
#include "sensors.h"
#include "rom_conf.h"
#include "debug.h"
#include "board.h"

const lmic_pinmap lmic_pins = {
    .nss = PIN_NSS,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = PIN_RST,
    .dio = {PIN_DIO_0, PIN_DIO_1, PIN_DIO_2},
};
osjob_t job;
osjob_t sleep_job;

/**
 * 
 */
void setup(void)
{
  board_setup();

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

  // Retrieve keys from EEPROM
  if (!conf_load())
  {
    _debug(F("Invalid EEPROM, did you flash the EEPROM?\n"));
    for (;;)
      ;
  }

  initialize_sensors();

  os_init();
  LMIC_reset();
  LMIC_setClockError(MAX_CLOCK_ERROR * 2 / 100);

  LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
  LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI); // g-band
  LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
  LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
  LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
  LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
  LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
  LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
  LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK, DR_FSK), BAND_MILLI);   // g2-band

  LMIC_startJoining();
}

void loop(void)
{
  os_runloop_once();
}

void job_measure_and_send(osjob_t *job)
{
  _debugTime();
  _debug(F("job_measure_and_send\n"));

  // Make sure LMIC is not busy with TX/RX and that there is no
  // TX data pending anymore. If so, then this is most likely
  // due to us trying to send before our previous message was
  // transmitted. This happens when trying to sent to frequently.
  if (LMIC.opmode & OP_TXRXPEND)
  {
    _debug(F("TXRX Pending...\n"));
    scheduleNextMeasurement(job);
    return;
  }

  // Perform measurement and save in packet
  tx_pkt_t pkt = {0};
  enable_sensors();
  pkt.air_temperature = get_air_temperature();
  pkt.distance_to_water = get_distance_to_water_median(13);
  disable_sensors();

  // Queue transmission
  lmic_tx_error_t err = LMIC_setTxData2(1, (uint8_t *)&pkt, sizeof(pkt), 0);
  if (err != 0)
  {
    _debugTime();
    _debug(F("TX error occured: "));
    _debug(err);
    _debug("\n");
  }
  scheduleNextMeasurement(job);
}

void scheduleNextMeasurement(osjob_t *job) {
  // Schedule our next measurement and send
  ostime_t now = os_getTime();
  ostime_t next_send = sec2osticks(os_getMeasurementInterval(LMIC.datarate));
  ostime_t next_possible = LMIC.txend - now;

  _debugTime();
  _debug("Next: ");
  _debug(LMIC.txend);
  _debug(" : ");
  _debug(next_send);
  _debug("\n");

  
  if (next_send < next_possible) {
    next_send = next_possible;
  }

  os_setTimedCallback(job, now + next_send, job_measure_and_send);
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
    os_clearCallback(&job);
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
    os_setCallback(&job, job_measure_and_send);
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
        LMIC_unjoinAndRejoin();
        _debugTime();
        _debug(F("REJOINING\n"));
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
  return 20;
  return conf_getMeasurementInterval(dr);
}