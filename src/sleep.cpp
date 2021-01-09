#include "sleep.h"

#include <util/atomic.h>
#include <avr/power.h>
#include <Arduino.h>
#include <arduino_lmic.h>
// #include <LowPower.h>
#include <Adafruit_SleepyDog.h>
#include "debug.h"

/**
 * 
 */
void sleep(uint32_t sleep_time)
{
  _debugTime();
  _debug(F("Entering sleep for: "));
  _debug(sleep_time);
  _debug("ms \n");
#ifdef DEBUG
  Serial.flush();
#endif

  // Disable ADC and all peripherals not required
  ADCSRA &= ~(1 << ADEN);
  power_all_disable();

  uint32_t start = millis();
  uint32_t elapsed = 0;
  while (elapsed < sleep_time)
  {
    uint32_t slept = Watchdog.sleep(sleep_time - elapsed);

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