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
  uint32_t time_remaining = sleep_time_ms;

  _debug(F("Sleeping for: "));
  _debug(sleep_time_ms / 1000);
  _debug(F(" seconds\r\n"));
#ifdef DEBUG
  Serial.flush();
#endif

  while (time_remaining > 0)
  {
    uint32_t slept;
    if (time_remaining >= 8000)
    {
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
      slept = 8000;
    }
    else if (time_remaining >= 4000)
    {
      LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
      slept = 4000;
    }
    else if (time_remaining >= 2000)
    {
      LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
      slept = 2000;
    }
    else if (time_remaining >= 1000)
    {
      LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
      slept = 1000;
    }
    else
    {
      slept = time_remaining;
    }
    time_remaining -= slept;

    // Fix timing
    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
      extern volatile unsigned long timer0_overflow_count;
      extern volatile unsigned long timer0_millis;
      // timer0 overflows every 64 * 256 clock cycles
      // https://github.com/arduino/ArduinoCore-avr/blob/6ec80154cd2ca52bce443afbbed8a1ad44d049cb/cores/arduino/wiring.c#L27
      timer0_overflow_count += microsecondsToClockCycles(slept * 1000) / (64 * 256);
      timer0_millis += slept;
    }
  }

  _debug(F("Woke up\r\n"));
}