#if !defined(_DEBUG_H_)
#define _DEBUG_H_

#include <Arduino.h>
#include "config.h"

#ifdef DEBUG
#define _debug(msg) Serial.print(msg)
#else
#define _debug(msg)
#endif

#endif // _DEBUG_H_