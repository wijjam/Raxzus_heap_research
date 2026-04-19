#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>

// IDT entry structure
struct idt_entry {
    uint16_t base_low;   // Lower 16 bits of handler address
    uint16_t selector;   // Kernel segment selector
    uint8_t  always0;    // Always 0
    uint8_t  flags;      // Flags
    uint16_t base_high;  // Upper 16 bits of handler address
} __attribute__((packed));

// IDT pointer structure
struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

void init_interrupts(void);

extern void isr_wrapper_33(void); // new keyboard wrapper
extern void isr_wrapper_32(void); // new timer wrapper
extern void isr_wrapper_129(void); // new system_call wrapper
extern void isr_wrapper_130(void); // The process switch interrupt int 0x82
extern void isr_wrapper_0(void); // new divide by 0 wrapper
extern void isr_wrapper_13(void); // new general purpose wrapper
extern void isr_wrapper_14(void); // new page fault wrapper
extern void isr_wrapper_8(void); // new double fault wrapper

#endif