#ifndef _YAOS_TIME_H
#define _YAOS_TIME_H
#include <asm/time.h>
struct rtcdate {
    uint second;
    uint minute;
    uint hour;
    uint day;
    uint month;
    uint year;
};
typedef void (*timer_callback) (u64 uptime, void *param);
extern int set_timeout_nsec(u64 nsec, timer_callback pfunc, void *param);
time64_t mktime64(const unsigned int year0, const unsigned int mon0,
                  const unsigned int day, const unsigned int hour,
                  const unsigned int min, const unsigned int sec);
#ifdef ARCH_HRT_UPTIME
#define hrt_uptime ARCH_HRT_UPTIME
#endif
#ifdef ARCH_HRT_SET_TIMEOUT
#define hrt_set_timeout ARCH_HRT_SET_TIMEOUT
#endif
#endif
