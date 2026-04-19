#ifndef VGA_H
#define VGA_H
#include <stdint.h>


#define kprintf_black(...)         do { set_color(COLOR_BLACK); kprintf(__VA_ARGS__); } while(0)
#define kprintf_blue(...)          do { set_color(COLOR_BLUE); kprintf(__VA_ARGS__); } while(0)
#define kprintf_green(...)         do { set_color(COLOR_GREEN); kprintf(__VA_ARGS__); } while(0)
#define kprintf_cyan(...)          do { set_color(COLOR_CYAN); kprintf(__VA_ARGS__); } while(0)
#define kprintf_red(...)           do { set_color(COLOR_RED); kprintf(__VA_ARGS__); } while(0)
#define kprintf_magenta(...)       do { set_color(COLOR_MAGENTA); kprintf(__VA_ARGS__); } while(0)
#define kprintf_brown(...)         do { set_color(COLOR_BROWN); kprintf(__VA_ARGS__); } while(0)
#define kprintf_light_gray(...)    do { set_color(COLOR_LIGHT_GRAY); kprintf(__VA_ARGS__); } while(0)
#define kprintf_dark_gray(...)     do { set_color(COLOR_DARK_GRAY); kprintf(__VA_ARGS__); } while(0)
#define kprintf_light_blue(...)    do { set_color(COLOR_LIGHT_BLUE); kprintf(__VA_ARGS__); } while(0)
#define kprintf_light_green(...)   do { set_color(COLOR_LIGHT_GREEN); kprintf(__VA_ARGS__); } while(0)
#define kprintf_light_cyan(...)    do { set_color(COLOR_LIGHT_CYAN); kprintf(__VA_ARGS__); } while(0)
#define kprintf_light_red(...)     do { set_color(COLOR_LIGHT_RED); kprintf(__VA_ARGS__); } while(0)
#define kprintf_light_magenta(...) do { set_color(COLOR_LIGHT_MAGENTA); kprintf(__VA_ARGS__); } while(0)
#define kprintf_yellow(...)        do { set_color(COLOR_YELLOW); kprintf(__VA_ARGS__); } while(0)
#define kprintf_white(...)         do { set_color(COLOR_WHITE); kprintf(__VA_ARGS__); } while(0)

#define COLOR_BLACK         0x00
#define COLOR_BLUE          0x01
#define COLOR_GREEN         0x02
#define COLOR_CYAN          0x03
#define COLOR_RED           0x04
#define COLOR_MAGENTA       0x05
#define COLOR_BROWN         0x06
#define COLOR_LIGHT_GRAY    0x07
#define COLOR_DARK_GRAY     0x08
#define COLOR_LIGHT_BLUE    0x09
#define COLOR_LIGHT_GREEN   0x0A
#define COLOR_LIGHT_CYAN    0x0B
#define COLOR_LIGHT_RED     0x0C
#define COLOR_LIGHT_MAGENTA 0x0D
#define COLOR_YELLOW        0x0E
#define COLOR_WHITE         0x0F



char make_color(char fg, char bg);
void set_color(char choosen_color);
void print_char(char c);
void print(const char* message);
char* int_to_string(int value);
char* double_to_string(double value);
void kprintf(char* start_text, ...);
void clearScreen();
void backspace();
void update_print_corner_time();
void cursor_blinker();
void remove_stuck_blinker();
void dump_stack_before_iret(uint32_t* sp);

#endif