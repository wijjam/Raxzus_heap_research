#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

#define KEYBOARD_DATA_PORT   0x60
#define KEYBOARD_STATUS_PORT 0x64

void keyboard_interrupt_handler(void);
void init_keyboard(void);
int get_buffer_position();

#endif