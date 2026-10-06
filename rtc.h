#ifndef KERNEL_RTC_H
#define KERNEL_RTC_H
#include <stdint.h>
struct rtc_time { uint16_t year; uint8_t month, day, hour, minute, second; };
void rtc_read(struct rtc_time *t);
#endif
