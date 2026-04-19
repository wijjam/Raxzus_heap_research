#include "../include/rtc.h"
#include "../include/io.h"
#include "../include/vga.h"

uint8_t read_cmos(uint8_t reg) {
    outb(0x70, reg);
    return inb(0x71);
}

uint8_t bcd_to_binary(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

void print_rtc_time(void) {
    uint8_t seconds = bcd_to_binary(read_cmos(0x00));
    uint8_t minutes = bcd_to_binary(read_cmos(0x02));
    uint8_t hours   = bcd_to_binary(read_cmos(0x04)) + 1;
    uint8_t day     = bcd_to_binary(read_cmos(0x07));
    uint8_t month   = bcd_to_binary(read_cmos(0x08));
    
    uint8_t raw_year = read_cmos(0x09);
    uint16_t year = bcd_to_binary(raw_year);
    year += 2000;
    
    kprintf("  Current time: %d/%d/%d %d:%d   \n", 
            day, month, (int)year, hours, minutes);
}