#include <am.h>
#include <nemu.h>

void __am_timer_init() {
}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  // ATTENTION: `rtc_io_handler` in `nemu/src/device/timer.c` updates rtc only when offset is 4.
  //             So we MUST read RTC_ADDR+4 first to ensure the low 32 bits are latest.
  uint64_t rtc_hi = (uint64_t)inl(RTC_ADDR + 4);
  uint64_t rtc_lo = (uint64_t)inl(RTC_ADDR);
  uptime->us = (rtc_hi << 32) | rtc_lo;
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour   = 0;
  rtc->day    = 0;
  rtc->month  = 0;
  rtc->year   = 1900;
}
