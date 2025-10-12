#include <am.h>

void __am_timer_init() {
}

#define RTC_MMIO 0xa0000048

#define RTC_READ(offset) *(volatile uint32_t*)(RTC_MMIO + offset)

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  // uint32_t lo = RTC_READ(0);
  uint32_t hi = RTC_READ(4);
  // uptime->us = ((uint64_t)hi << 32) | lo;
  uptime->us = hi;
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  rtc->second = RTC_READ(8);
  rtc->minute = RTC_READ(12);
  rtc->hour   = RTC_READ(16);
  rtc->day    = RTC_READ(20);
  rtc->month  = RTC_READ(24);
  rtc->year   = RTC_READ(28);
}
