#include "../include/gdt.h"
#include "../include/vga.h"

static struct gdt_entry gdt[6];
static struct tss_entry tss;
static struct gdt_ptr   gdtr;

static void gdt_set_entry(int i, uint32_t base, uint32_t limit,
                           uint8_t access, uint8_t gran)
{
    gdt[i].base_low    = BITS_0_TO_15(base);
    gdt[i].base_mid    = BITS_16_TO_23(base);
    gdt[i].base_high   = BITS_24_TO_31(base);
    gdt[i].limit_low   = BITS_0_TO_15(limit);
    gdt[i].granularity = BITS_16_TO_19(limit) | TOP_NIBBLE(gran);
    gdt[i].access      = access;
}

static void tss_set_entry(int i, uint32_t kernel_stack) {
    uint32_t base = (uint32_t)&tss;
    uint32_t limit = sizeof(struct tss_entry)-1;

    gdt_set_entry(i, base, limit, 0x89, 0x00);
    
    kmemset(&tss, 0, sizeof(tss));

    tss.ss0 = 0x10; // Setting the kernel descriptor which in our case is 0x10 because it is not cs it is ss which tells us where our esp0 is.

    tss.esp0 = kernel_stack;        // Setting the esp0 to the kernel stack for that user proces.

    tss.iomap_base = sizeof(struct tss_entry); // Essencialy says taht the I/O mapping is outside of the tss entry which means that the user process has no rights to it.

}

There once was a PIT told to inform his friend the PIC to send out nagging signals every 10ms (These nagging signals are what we call interrupts) to the cpu. The cpu when he gets these signals sigh frustrated, he takes a breath looks at his assigned interrupt table and sees that this interrupt wants to run a assembly stub which is a assembly file filled with different assembly codes (Like main functions but for assembly.) And one of these are called timer interrupt the same one named in the interrupt table. So the cpu stops what he is doing and goes to this timer interrupt function. As he enters it he is forced to reveal all his internal registers at that time, then it calls the c function timer_interrupt which sends the CPU to a c file where it finds the next process that should run by looking at the processes sleeping and the once awake and choosing the next awake process. When that is done, he leaves the c code timer_interrupt and return to the assembly where the magic happens, He is begins to optimize and work checking if the next and current process is the same, he does not want to spend time changing his internal registers when nothing changes, but also he does not want to accidentaly overwrite and corrupt his internal registers either so if the current and next are the same he skips them and instead returns with iret. which means he puts everything back inside of him slowly. But there is one thing, when he puts it back he puts it back in a order of how he will work. He makes a plan, Depending on if it is a change in rings then he might have to do more, so first lets see how he runs in a normal kernel process, he just has EIP (Where he currently is right now.) CS(What he can do right now) EFLAGS(If interrupt status is ok) 


uint32_t get_tss_stack(void) {
    return tss.esp0;
}


void tss_set_kernel_stack(uint32_t stack) {
    tss.esp0 = stack;
}


static inline void gdt_load(void)
{
    asm volatile (
        "lgdt (%0)\n"
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        "ljmp $0x08, $1f\n"   // Far jump to reload CS
        "1:\n"
        : : "r"(&gdtr) : "ax", "memory"
    );
}

static inline void tss_load(void) {
    asm volatile ("ltr %0" : : "r"((uint16_t)0x28));
}


void init_gdt(void)
{
    // Null descriptor
    gdt_set_entry(0, 0, 0, 0, 0);

    // Kernel Code: base=0, limit=4GB, ring 0, executable, readable
    // Access: 0x9A = Present | Ring0 | Code | Executable | Readable
    // Gran:   0xCF = 4KB granularity | 32-bit
    gdt_set_entry(1, 0x00000000, 0xFFFFF, 0x9A, 0xCF);

    // Kernel Data: base=0, limit=4GB, ring 0, writable
    // Access: 0x92 = Present | Ring0 | Data | Writable
    // Gran:   0xCF = 4KB granularity | 32-bit
    gdt_set_entry(2, 0x00000000, 0xFFFFF, 0x92, 0xCF);

    gdt_set_entry(3, 0x00000000, 0xFFFFF, 0xFA, 0xCF);

    gdt_set_entry(4, 0x00000000, 0xFFFFF, 0xF2, 0xCF);


    void *stack = kmalloc(4096);
    if (!stack) {
    // kernel panic, you can't continue without this
    kprintf_red("Failed to allocate kernel stack for TSS");
        asm volatile("hlt");
}
    tss_set_entry(5, (uint32_t)stack + 4096);

    

    // Set up GDTR
    gdtr.limit = (sizeof(struct gdt_entry) * 6) - 1;
    gdtr.base  = (uint32_t)&gdt;

    gdt_load();
    tss_load();
}

