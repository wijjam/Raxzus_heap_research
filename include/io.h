#ifndef IO_H
#define IO_H

#include <stdint.h>

void outb(uint16_t port, uint8_t value); // b in inb stands for bits. same with outb
uint8_t inb(uint16_t port);

void outw(uint16_t port, uint16_t value);
uint16_t inw(uint16_t port); // w in inw stands for word and in x86-32 architecture is 16-bit same with outb

#endif