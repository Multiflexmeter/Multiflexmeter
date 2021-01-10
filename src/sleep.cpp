#include "sleep.h"

#include <util/atomic.h>
#include <avr/power.h>
#include <Arduino.h>
#include <arduino_lmic.h>
#include <hal/hal.h>
#include <Adafruit_SleepyDog.h>
#include "debug.h"
#include "time_util.h"

uint32_t next_job_time = 0;

// Returns the number of ticks until time. Negative values indicate that
// time has already passed.
static int32_t delta_time(uint32_t time)
{
  return (int32_t)(time - hal_ticks());
}

uint8_t hal_checkTimer(uint32_t time)
{
  // A job is scheduled within 100 ms so don't sleep
  if (delta_time(time) <= ms2osticks(100))
  {
    return 1;
  }
  next_job_time = time;
  return 0;
}

void hal_sleep()
{
  if (LMIC.opmode & (OP_TXRXPEND))
  {
    return;
  }
  sleep(osticks2ms(next_job_time));
}

/**
 * 
 */
void sleep(uint32_t wakeup_time)
{
  uint32_t start = millis();
  uint32_t duration = wakeup_time - start;

  if (is_time_before(wakeup_time, start))
  {
    return;
  }

  _debugTime();
  _debug(F("Entering sleep for: "));
  _debug(duration);
  _debug("ms...\n");
#ifdef DEBUG
  Serial.flush();
#endif

  // Disable ADC and all peripherals not required
  ADCSRA &= ~(1 << ADEN);
  power_all_disable();

  // Rollover compatible sleep loop
  uint32_t elapsed = 0;
  while (elapsed < duration)
  {
    uint32_t slept = Watchdog.sleep(duration - elapsed);

    // Fix timing
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
      extern volatile unsigned long timer0_overflow_count;
      extern volatile unsigned long timer0_millis;
      // timer0 overflows every 64 * 256 clock cycles
      // https://github.com/arduino/ArduinoCore-avr/blob/6ec80154cd2ca52bce443afbbed8a1ad44d049cb/cores/arduino/wiring.c#L27
      timer0_overflow_count += microsecondsToClockCycles(slept * 1000) / (64 * 256);
      timer0_millis += slept;
    }

    elapsed = millis() - start;
  }

  power_timer0_enable();
  power_spi_enable();
#ifdef DEBUG
  power_usart0_enable();
#endif

  _debugTime();
  _debug(F("Woke up\n"));
}