#ifndef PIC_H
#define PIC_H

#include <stdint.h>

// PIC port addresses
#define PIC1_COMMAND    0x20    // Master PIC command port
#define PIC1_DATA       0x21    // Master PIC data port
#define PIC2_COMMAND    0xA0    // Slave PIC command port  
#define PIC2_DATA       0xA1    // Slave PIC data port

// PIC commands
#define PIC_EOI         0x20    // End of Interrupt signal

void pic_remap(uint8_t offset1, uint8_t offset2);
void pic_enable_irq(uint8_t irq);
void pic_disable_irq(uint8_t irq);
void pic_send_eoi(uint8_t irq);

#endif