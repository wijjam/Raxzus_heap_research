#include "../include/process_manager.h"

volatile int need_reschedule = 0;

void sleep(int time) {

    if (time <= 0) {
        kprintf("%e you can not make a process sleep: %d ms", time*10);
        return;
    }

    asm volatile(
        "movl $1, %%eax\n"        // Syscall number: 1 = sleep
        "movl %0, %%ebx\n"        // Argument: time
        "int $0x81\n"             // Call syscall interrupt
        :
        : "r"(time)
        : "eax", "ebx"
    );
}


int fork() {

        asm volatile(
        "movl $2, %%eax\n"        // Syscall number: 2 = fork
        "int $0x81\n"             // Call syscall interrupt
        :
        : 
        : "eax"
    );
    


}


void system_call_interrupt_handler(struct registers* regs) {


    kprintf("The adress is: %x\n", regs);
    kprintf("The adress eax is: %x\n", regs->eax);
        __asm__ volatile("hlt");


    switch (regs->eax) {
        case 1:
            //current_process->sleep_time = regs->ebx;
            //kprintf("%d", current_process->sleep_time);
            //schedule();
            //need_reschedule = 1;
        break;

        case 2:

            //kprintf("The values we got are: %d\n", copy_process(regs));

        //__asm__ volatile("hlt");
        break;

        case 3:
            kprintf("fgsdffdfsds\n");
        __asm__ volatile("hlt");
            
            
        break;
            
    }
    

}