#include "../include/io.h"

// Takes a 16-bit port number EDX and 8-bit value EAX to send to that port.
void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" 
                     : 
                     : "a"(value), "Nd"(port));
}
// "outb" %0, %1: Sends the value from EAX into the port located in EDX
// "a" (value): put the value parameter into EAX register
// "Nd" (port): Put port into EDX register or user immediate value


// Takes a port number, returns the byte from that port.
uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile ("inb %1, %0"
                     : "=a"(result)
                     : "Nd"(port));
    return result;
}
// "inb %1, %0": Reads the value from the port specified in EDX, stores result in EAX
// "=a": Stores the result in EAX register 
// (result): Copies the EAX value into our result variable.
// "Nd" (port): Put port into EDX register or user immediate value




void outw(uint16_t port, uint16_t value) {
    __asm__ volatile ("outw %0, %1" 
                     : 
                     : "a"(value), "Nd"(port));
}
// "outb" %0, %1: Sends the value from EAX into the port located in EDX
// "a" (value): put the value parameter into EAX register
// "Nd" (port): Put port into EDX register or user immediate value


// Takes a port number, returns the byte from that port.
uint16_t inw(uint16_t port) {
    uint16_t result;
    __asm__ volatile ("inw %1, %0"
                     : "=a"(result)
                     : "Nd"(port));
    return result;
}
// "inb %1, %0": Reads the value from the port specified in EDX, stores result in EAX
// "=a": Stores the result in EAX register 
// (result): Copies the EAX value into our result variable.
// "Nd" (port): Put port into EDX register or user immediate value