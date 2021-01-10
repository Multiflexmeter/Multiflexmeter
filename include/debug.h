#if !defined(_DEBUG_H_)
#define _DEBUG_H_

#include <Arduino.h>
#include "config.h"

#ifdef DEBUG
#define _debugTime()      \
  Serial.print("[");      \
  Serial.print(millis()); \
  Serial.print("] ");
#define _debug(...) Serial.print(__VA_ARGS__);
#else
#define _debugTime()
#define _debug(msg)
#endif

#endif // _DEBUG_H_