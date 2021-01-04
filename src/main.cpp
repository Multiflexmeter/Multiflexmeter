#include "main.h"

#include <Arduino.h>
#include "sleep.h"
#include "sensors.h"
#include "rom_conf.h"
#include "debug.h"
#include "config.h"

const lmic_pinmap lmic_pins = {
    .nss = PIN_NSS,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = PIN_RST,
    .dio = {PIN_DIO_0, PIN_DIO_1, PIN_DIO_2},
};
osjob_t job;
tx_pkt_t pkt = {0};

void power_save_setup(void)
{
  // Avoid leak currents through floating pins
  DDRB = 0;
  DDRC = 0;
  DDRD = 0;
  PORTB = 1;
  PORTC = 1;
  PORTD = 1;
  MCUCR &= ~(1 << PUD);

  DIDR0 = 1;
  DIDR1 = 1;

  // Disable peripherals
  PRR |= (1 << PRTWI)    // turn off TWI
         | (1 << PRTIM1) // turn off Timer/Counter1
         | (1 << PRTIM2) // turn off Timer/Counter1
         | (1 << PRADC); // turn off ADC

#ifndef DEBUG
  PRR |= (1 << PRUSART0); // turn off USART
#endif
}

/**
 * 
 */
void setup(void)
{
  power_save_setup();
  pinMode(17, OUTPUT);
  digitalWrite(17, LOW);
#ifdef DEBUG
  Serial.begin(115200);
#endif
#ifdef PRINT_BUILD_DATE_TIME
  _debug(F("Build at: "));
  _debug(F(__DATE__));
  _debug(" ");
  _debug(F(__TIME__));
  _debug("\r\n");
#endif

  // Retrieve keys from EEPROM
  if (!conf_load())
  {
    _debug(F("Invalid EEPROM, did you flash the EEPROM?\r\n"));
    for (;;)
      ;
  }

  // Initialize sensors
  initialize_sensors();

  // Initialize lmic
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

void job_measure(osjob_t *job)
{
  _debug(F("job_measure\r\n"));

  enable_sensors();
  pkt.air_temperature = get_air_temperature();
  pkt.distance_to_water = get_distance_to_water();
  disable_sensors();

  os_setCallback(job, job_queue);
}

/**
 * Prepare and queue a new measurement and transmission
 */
void job_queue(osjob_t *job)
{
  _debug(F("job_queue\r\n"));
  if (LMIC.opmode & OP_TXRXPEND)
  {
    return;
  }
  LMIC_setTxData2(1, (uint8_t *)&pkt, sizeof(pkt), 0);
}

/**
 * Put the device into sleep and schedule a new transmission
 */
void job_sleep(osjob_t *job)
{
  sleep(SLEEP_DURATION);
  os_setCallback(job, job_measure);
}

/*
  Events fired by the LMIC library.
  We are interested in just three:

    - EV_JOINED
        Device just joined through OTAA and can now start the measurement
        and send cycle

    - EV_TXCOMPLETE
        Device just finished sending a measurement and should now sleep

    - EV_JOIN_TXCOMPLETE
        Device could not join through OTAA, manually lower DR (increase SF)
        and sleep
*/
void onEvent(ev_t ev)
{
  switch (ev)
  {
  /*
    Fired when the device succesfully finishes the OTAA join procedure
    and is accepted
  */
  case EV_JOINED:
    _debug(F("Joined\r\n"));
    LMIC_setLinkCheckMode(0);
    LMIC_setAdrMode(1);
    os_setCallback(&job, job_measure);
    break;

  /*
    Fired when the device has sent an uplink message
    This also includes receiving downlink message in RX1 & 2
  */
  case EV_TXCOMPLETE:
    _debug(F("Uplink complete\r\n"));
    if (LMIC.dataLen)
    {
      // Data received
    }
    // Schedule next transmission
    os_setCallback(&job, job_sleep);
    break;

  /*
    Fired when the device has completed a join request but did not 
    receive a (valid) response and thus failed to join
  */
  case EV_JOIN_TXCOMPLETE:
    _debug(F("Join failed\r\n"));
    /*
      LMIC tries every default channel once before going to the next datarate.
      This is tracked through the txCnt property. When txCnt reaches the default
      channel count for the bandplan then the DR is lowered. Unfortunately we 
      cannot directly reference the default channel count.

      Instead we prevent it from reaching the default channel count and manually
      lower the DR each time
    */
    LMIC.txCnt = 0;
    // If we are at the lowest DR we allow then only join every measurement interval
    // to save battery
    if (LMIC.datarate <= MIN_LORA_DR)
    {
      LMIC.datarate = MIN_LORA_DR;
      sleep(SLEEP_DURATION);
    }
    else
    {
      // As long as we arent at the lowest datarate, lower it and retry asap
      LMIC.datarate = decDR((dr_t)LMIC.datarate);
      // The sleep function will choose either the given sleep time or the next
      // possible TX time, depending on which is later to avoid being awake unnecessary
      sleep(0);
    }
    break;

  default:
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