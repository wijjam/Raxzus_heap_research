#include <stdint.h>

#define BITS_0_TO_15(x)   ((x) & 0xFFFF)
#define BITS_16_TO_23(x)  (((x) >> 16) & 0xFF)
#define BITS_24_TO_31(x)  (((x) >> 24) & 0xFF)
#define BITS_16_TO_19(x)  (((x) >> 16) & 0x0F)
#define TOP_NIBBLE(x)     ((x) & 0xF0)


uint32_t get_tss_stack(void);
void init_gdt(void);
void tss_set_kernel_stack(uint32_t stack);

// A GDT entry is 8 bytes
struct gdt_entry {
    uint16_t limit_low;    // Lower 16 bits of limit
    uint16_t base_low;     // Lower 16 bits of base
    uint8_t  base_mid;     // Middle 8 bits of base
    uint8_t  access;       // Access byte
    uint8_t  granularity;  // Flags + upper 4 bits of limit
    uint8_t  base_high;    // Upper 8 bits of base
} __attribute__((packed));

struct tss_entry {
    uint32_t prev_tss;  // ignore, zero it
    uint32_t esp0;      // ← kernel stack pointer (IMPORTANT)
    uint32_t ss0;       // ← kernel stack segment (0x10, your kernel data)

    // everything below: zero it out, you won't use it
    uint32_t esp1, ss1;
    uint32_t esp2, ss2;
    uint32_t cr3, eip, eflags;
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap, iomap_base;
} __attribute__((packed));


// The GDTR structure (pointer passed to lgdt)
struct gdt_ptr {
    uint16_t limit;        // Size of GDT - 1
    uint32_t base;         // Linear address of GDT
} __attribute__((packed));