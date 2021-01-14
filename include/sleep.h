#if !defined(_SLEEP_H_)
#define _SLEEP_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif
  void sleep(uint32_t ms);
  uint32_t _sleep(uint32_t delay);
  void get_sleep_period(uint32_t period, uint32_t &wdto, uint32_t &actual);
#ifdef __cplusplus
}
#endif

#endif // _SLEEP_H_