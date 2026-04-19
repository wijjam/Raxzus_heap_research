#ifndef KEYMAP_H
#define KEYMAP_H

#include <stdint.h>

// Convert scancode to ASCII character
char scancode_to_ascii(uint8_t scancode, int shift_pressed);

#endif