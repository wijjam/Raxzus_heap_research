#include "../include/pic.h"
#include "../include/io.h"
#include "../include/vga.h"

void pic_remap(uint8_t offset1, uint8_t offset2) {


    

    // Save current interrupt masks (which IRQs are enabled/disabled)

    kprintf_white("[INIT] Establishing connection with PIC");

    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);
    
    // Start initialization sequence for both PICs
    outb(PIC1_COMMAND, 0x11);  // Initialize Master PIC
    outb(PIC2_COMMAND, 0x11);  // Initialize Slave PIC
    kprintf_green(".........[OK]\n");
    
    kprintf_white("[INIT] Mapping with Vector offset");

    // Set interrupt vector offsets
    outb(PIC1_DATA, offset1);  // Master PIC: IRQ 0-7 → interrupts offset1 to offset1+7
    outb(PIC2_DATA, offset2);  // Slave PIC: IRQ 8-15 → interrupts offset2 to offset2+7
    
    // Tell Master PIC that Slave PIC is connected to IRQ 2
    outb(PIC1_DATA, 0x04);     // Binary: 00000100 (bit 2 set)
    
    // Tell Slave PIC it's connected to Master PIC's IRQ 2
    outb(PIC2_DATA, 0x02);     // Slave identity: 2
    
    // Set both PICs to 8086 mode (vs older 8080 mode)
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);
    
    // Restore the interrupt masks
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);


    kprintf_green(".........[OK]\n");

}

void pic_enable_irq(uint8_t irq) {
    uint16_t port;

    // Determine which PIC and get its data port
    if (irq < 8) {
        port = PIC1_DATA; // Master PIC handles IRQ 0-7
    } else {
        port = PIC2_DATA; // Slave PIC handles IRQ 8-15
        irq -= 8; // Convert IRQ 8-15 to bit positions 0-7
    }

    // Read current mask, clear the bit for this IRQ, write back
    uint8_t mask = inb(port);
    mask &= ~(1 << irq);      // Clear bit = enable IRQ
    outb(port, mask);
}

void pic_disable_irq(uint8_t irq) {
    uint16_t port;
    
    // Determine which PIC and get its data port
    if (irq < 8) {
        port = PIC1_DATA;  // Master PIC handles IRQ 0-7
    } else {
        port = PIC2_DATA;  // Slave PIC handles IRQ 8-15
        irq -= 8;          // Convert IRQ 8-15 to bit positions 0-7
    }
    
    // Read current mask, set the bit for this IRQ, write back
    uint8_t mask = inb(port);
    mask |= (1 << irq);       // Set bit = disable IRQ
    outb(port, mask);
}

void pic_send_eoi(uint8_t irq) {
    // If IRQ came from Slave PIC (IRQ 8-15), send EOI to both PICs
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);  // Tell Slave PIC: "I handled it"
    }
    
    // Always send EOI to Master PIC
    outb(PIC1_COMMAND, PIC_EOI);      // Tell Master PIC: "I handled it"
}