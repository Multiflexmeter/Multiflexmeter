#if !defined(_DEBUG_H_)
#define _DEBUG_H_

#include <Arduino.h>
#include <lmic/oslmic.h>
#include <hal/hal.h>
#include "config.h"

#ifdef DEBUG
#define _debugTime()                     \
  Serial.print("[");                     \
  Serial.print(osticks2ms(hal_ticks())); \
  Serial.print("] ");
#define _debug(...) Serial.print(__VA_ARGS__);
#define _debugf(...) Serial.printf(__VA_ARGS__);
#else
#define _debugTime()
#define _debug(msg)
#endif

#endif // _DEBUG_H_