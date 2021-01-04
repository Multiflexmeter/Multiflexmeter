#include "sleep.h"

#include <util/atomic.h>
#include <Arduino.h>
#include <arduino_lmic.h>
#include <LowPower.h>
#include "debug.h"

/**
 * 
 */
void sleep(uint32_t ms)
{
  uint32_t now = millis();
  uint32_t next_tx = osticks2ms(LMICbandplan_nextTx(os_getTime()));
  if (next_tx < now)
  {
    next_tx = now;
  }
  uint32_t sleep_time_ms = max(ms, next_tx - now);

  _debug(F("Sleeping for: "));
  _debug(sleep_time_ms / 1000);
  _debug(F(" seconds\r\n"));
#ifdef DEBUG
  Serial.flush();
#endif

  while (sleep_time_ms > 0)
  {
    // For now sleeping happens per 8 seconds (8000 ms)
    uint32_t cycle_ms = 8000;
    // ADC will actually stay in current state and not turn on
    LowPower.powerDown(SLEEP_8S, ADC_ON, BOD_OFF);

    // Lower duration
    if (sleep_time_ms <= cycle_ms)
      sleep_time_ms = 0;
    else
      sleep_time_ms -= cycle_ms;

    // Fix timing
    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
      extern volatile unsigned long timer0_overflow_count;
      extern volatile unsigned long timer0_millis;
      // timer0 overflows every 64 * 256 clock cycles
      // https://github.com/arduino/ArduinoCore-avr/blob/6ec80154cd2ca52bce443afbbed8a1ad44d049cb/cores/arduino/wiring.c#L27
      timer0_overflow_count += microsecondsToClockCycles(cycle_ms * 1000) / (64 * 256);
      timer0_millis += cycle_ms;
    }
  }
  _debug(F("Woke up\r\n"));
}