#ifndef RTC_H
#define RTC_H

#include <stdint.h>

uint8_t read_cmos(uint8_t reg);
uint8_t bcd_to_binary(uint8_t bcd);
void print_rtc_time(void);

#endif