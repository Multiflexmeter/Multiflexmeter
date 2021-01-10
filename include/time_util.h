#if !defined(_TIME_UTIL_H_)
#define _TIME_UTIL_H_

#define is_time_before(t1, t2) ((t1 - t2) & 0x80000000)

#endif // _TIME_UTIL_H_