#include <am.h>

void __am_timer_init() {
}

#define RTC_MMIO 0x02000000

#define RTC_READ(offset) *(volatile uint32_t*)(RTC_MMIO + offset)

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  uint32_t lo = RTC_READ(0);
  uint32_t hi = RTC_READ(4);
  uint64_t mtime = ((uint64_t)hi << 32) | lo;

#ifdef TRACE
  const uint64_t cycle_per_us = 7;
#else
  const uint64_t cycle_per_us = 1;
#endif

  uptime->us = mtime * cycle_per_us;
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour   = 0;
  rtc->day    = 0;
  rtc->month  = 0;
  rtc->year   = 1900;
}
