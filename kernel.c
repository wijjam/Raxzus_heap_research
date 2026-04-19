#include <stdarg.h>
#include <stdint.h>
#include "include/io.h"
#include "include/rtc.h"
#include "include/vga.h"
#include "include/interrupts.h"
#include "include/pic.h"
#include "include/keyboard.h"
#include "include/raxzus_shell.h"
#include "include/raxzus_shell_ui.h"
#include "include/memory.h"
#include "include/process_manager.h"
#include "include/forgeproc.h"
#include "include/paging_manager.h"
#include "include/pmm.h"
#include "include/gdt.h"
#include "include/ata_disk_driver.h"
#include "include/kutils.h"

void timer_process_worker() {

    while(1) {
        update_print_corner_time();
        //sleep(10); // 100 is 1 second. 
    }
}

void worker_process() {
    //pic_disable_irq(0);
    while(1) {
        //kprintf("We are printing with process: %x\n", current_process->PID);
        //sleep(100);
    }
}


void user_process() {
    
    while(1);
}

void idle_process() {
        // Do nothing, or print "IDLE" 
        // This keeps the CPU busy when all real processes sleep

            //create_process(&timer_process_worker, 0);


           // create_process(&worker_process);
    __asm__ volatile ("sti"); // opens the flood gates.
    //create_process(&worker_process, 0);
          
    pic_enable_irq(0); // Enable timer
    //kprintf("The user process kernel eip is: %x \n", &user_process);
    //kprintf("The kernel tss stack before user process is: %x\n", get_tss_stack());
    //create_process(&user_process, 1);
    

    
    kprintf_cyan("RaxzusOS > ");
    
    while(1) {
        __asm__ volatile("hlt");
    }
}





void kernel_main(void) {
    draw_box_top(70, COLOR_WHITE, COLOR_MAGENTA, "");
    
    print_text_align("RaxzusOS Kernel v1.0 - Boot Sequence", 70, ALIGN_CENTER, COLOR_BLACK, COLOR_WHITE, 2);
    draw_box_bottom(70, COLOR_WHITE);
    print_text_align("Inizializing system programs.....", 70, ALIGN_LEFT, COLOR_BLACK, COLOR_YELLOW, 2);
    pic_remap(32, 40);  // Remap IRQs: Master to 32-39, Slave to 40-47

    pic_disable_irq(46);
    
    pic_disable_irq(0); // Disable timer for now (would overwhelm us)
    pic_enable_irq(1);  // Enable keyboard
    init_keyboard();
    kernel_heap_init(); // Inizialize the heap for the kernel (PS: The processes has a seperate heap init function)
    init_gdt();
    init_interrupts();  // Setup the IDT and connect the interrupts to stubs
    init_paging();      // Initializes paging where we flip the bit, save to CR3.
    init_pmm();         // Maps the virtual memory so we an have dynamic paging

    kprintf("\n");

    draw_box_top(30, COLOR_CYAN, COLOR_LIGHT_GRAY, "[ RaxzusOS Boot Menu ]");
    draw_box_sides(30, 2, COLOR_CYAN);
    print_text_align("[1] Normal Boot", 30, ALIGN_LEFT, COLOR_CYAN, COLOR_GREEN, 2);
    print_text_align("[2] Safe Mode", 30, ALIGN_LEFT, COLOR_CYAN, COLOR_YELLOW, 2);
    print_text_align("[3] Recovery", 30, ALIGN_LEFT, COLOR_CYAN, COLOR_RED, 2);
    draw_box_sides(30, 2, COLOR_CYAN);
    draw_box_bottom(30, COLOR_CYAN);
    
 
    





    
    //kprintf("The current_process before idle is: %x\n", current_process);  
    kprintf("Jumping to IDLE.... \n");
    init_process_scheduler(&idle_process);

    kprintf("dfsfsdf");


    // Main kernel loop - just wait for interrupts
    while (1) {
        // Do nothing - let interrupts handle everything
        // This is where your kernel "idles"
        __asm__ volatile("hlt");
    }



}