#include "../include/process_manager.h"
#include "../include/vga.h"
#include "../include/pic.h"
#include "../include/heap_domain.h"
#include "../include/paging_manager.h"
#include "../include/pmm.h"
#include "../include/gdt.h"

struct PCB* current_process;
struct PCB* next_process;

struct PCB* process_lists;

uint16_t current_index = 0;

uint32_t blink_counter = 0;

//struct PCB process_lists[1000];

uint32_t* esp_address_variable;

void timer_interrupt_handler() {
            
        // The round robbin functionality
        //struct registers* stack = (struct registers *)esp_address_variable; 
    

    blink_counter++;
    if (blink_counter >= 50) {
        cursor_blinker();
        blink_counter = 0;
    }

    schedule();

 //kprintf("This is 10ms=======================================================================\n");

    pic_send_eoi(0);
}

void create_process(void (*func)(), int is_user){

    struct PCB* new_process = (struct PCB*) kmalloc(sizeof(struct PCB));
    if (new_process == (void*)0) { return; }

    new_process->PID = current_index;
    new_process->sleep_time = 0;
    new_process->next = (void*)0;
    new_process->prev = (void*)0;

    // Create page directory
    uint32_t* proc_dir = create_process_page_directory();
    new_process->page_dir = proc_dir;

    // Allocate stack
    uint32_t stack_phys = get_next_free_process_frame();
    uint32_t* sp;
    
    if (is_user) {
        map_page(proc_dir, PROC_STACK_VIRT, stack_phys, PAGE_USER);
        sp = (uint32_t*)(stack_phys + 0xC0000000 + PROC_STACK_SIZE);

        uint32_t code_phys = get_next_free_process_frame();
        map_page(proc_dir, PROC_CODE_VIRT, code_phys, PAGE_USER);

        // Copy function
        uint8_t* dst = (uint8_t*)(code_phys + 0xC0000000);
        uint8_t* src = (uint8_t*)func;
        for (int i = 0; i < 4096; i++)
            dst[i] = src[i];

        *(--sp) = 0x23;         // SS
        *(--sp) = PROC_STACK_VIRT + PROC_STACK_SIZE;  // ESP
        *(--sp) = 0x202;        // EFLAGS
        *(--sp) = 0x1B;         // CS
        *(--sp) = PROC_CODE_VIRT; // EIP
    } else {
        map_page(proc_dir, PROC_STACK_VIRT, stack_phys, PAGE_KERNEL);
        sp = (uint32_t*)(stack_phys + 0xC0000000 + PROC_STACK_SIZE);
        
        *(--sp) = 0x202;        // EFLAGS
        *(--sp) = 0x08;         // CS
        *(--sp) = (uint32_t)func; // EIP
    }

    // POPA frame
    *(--sp) = 0;  // EAX
    *(--sp) = 0;  // ECX
    *(--sp) = 0;  // EDX
    *(--sp) = 0;  // EBX
    *(--sp) = 0;  // ESP
    *(--sp) = 0;  // EBP
    *(--sp) = 0;  // ESI
    *(--sp) = 0;  // EDI

    uint32_t sp_offset = (uint32_t)sp - (stack_phys + 0xC0000000);
    new_process->saved_esp = PROC_STACK_VIRT + sp_offset;
    new_process->stack_top = PROC_STACK_VIRT + PROC_STACK_SIZE;

    // Allocate kernel stack using NEW heap!
    void *kstack = kmalloc(4096);
    if (kstack == (void*)0) {
        kprintf_red("Failed to allocate kernel stack\n");
        while(1) asm volatile("hlt");
    }
    new_process->kernel_stack_top = (uint32_t)kstack + 4096;

    // Add to process list
    if (current_index == 0) {
        process_lists = new_process;
        current_index++;
        return;
    }

    current_index++;
    struct PCB* current = process_lists;
    while (current->next != (void*)0) {
        current = current->next;
    }
    current->next = new_process;
    new_process->prev = current;
}

void init_process_scheduler(void (*func)()) {
    current_index = 0;
    create_process(func, 0);
    current_process = process_lists;
    next_process    = process_lists;

    // =========================================================
    // FIX 4: Actually start the first process.
    //
    // Previously this function returned to kernel_main, which
    // looped forever in hlt.  The idle_process PCB existed but
    // nobody ever ran it, so create_process(&timer_process_worker)
    // and pic_enable_irq(0) inside idle_process were never reached.
    //
    // We manually do what the timer ISR normally does:
    //   1. Load the process's page directory into CR3.
    //   2. Point ESP at the fake interrupt frame.
    //   3. popa  -> restore the zeroed registers.
    //   4. iret  -> jump to func() with IF=1.
    //
    // This function never returns.
    // =========================================================
    uint32_t phys_dir = (uint32_t)current_process->page_dir - 0xC0000000;
    uint32_t first_esp = current_process->saved_esp;

    __asm__ volatile(
        "movl %0, %%cr3\n"    /* switch to process page directory */
        "movl %1, %%esp\n"    /* load process stack pointer       */
        "popa\n"              /* restore fake register frame      */
        "iret\n"              /* jump to process entry point      */
        :
        : "r"(phys_dir), "r"(first_esp)
        : "memory"
    );

    /* Never reached; keeps the compiler happy. */
    while(1) { __asm__ volatile("hlt"); }
}

void schedule() {
    // The round robbin functionality
    struct PCB* sim_current = current_process;
    do  {

        if (sim_current->next != (void*)0) {
            next_process = sim_current->next;
            sim_current = next_process;
            //kprintf("Hello");
           
        } else {
            next_process = process_lists;
            sim_current = next_process;
            //kprintf("No");
            
        }
        
        if (sim_current->sleep_time > 0) {
            sim_current->sleep_time = sim_current->sleep_time - 1;
        }

    } while (sim_current->sleep_time > 0);

    tss_set_kernel_stack(sim_current->kernel_stack_top);

    //__asm__ volatile("hlt");


}


/*int copy_process(uint32_t* esp_stack) {
    struct PCB* new_process = (void*) kmalloc(sizeof(struct PCB));

    if (new_process == (void*)0) {return (void*)0;}

    new_process->PID = current_index;
    new_process->sleep_time = 0;
    new_process->next = (void*)0;
    new_process->prev = (void*)0;
    // Get ACTUAL CS from CPU

    uint16_t current_PID;
    uint16_t actual_cs;
    uint32_t eip_address;
    //uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t esp_value;
    uint32_t saved_ebp;

    struct registers* esp_ptr = (struct registers*)esp_stack;


    ecx = esp_ptr->ecx;
    edx = esp_ptr->edx;
    ebx = esp_ptr->ebx;
    ebp = esp_ptr->ebp;
    esi = esp_ptr->esi;
    edi = esp_ptr->edi;
    saved_ebp = esp_ptr->ebp;

    eip_address = esp_ptr->eip;


    

    __asm__ volatile("mov %%cs, %0" : "=r"(actual_cs)); // Gets the CS from the cpu, no Assumsion!


    current_PID = current_index;
    uint8_t *stack = kmalloc(4096); // We allocate our custom stack frame to the heap.
    if (stack == (void*)0) {return;} // we then check if it returns null, if the heap runs out we can not allocate a stack.
    uint32_t *child_ptr = (uint32_t*)(stack + 4096); // Since stack moves downward we make the stack pointer (esp) point at the top of the heap and then move down.
    uint32_t *sp = (uint32_t*)(current_process->saved_esp); // start process stuff fix later.
    uint32_t *new_esp;

    // IRET frame
    *(--sp) = 0x202;          // EFLAGS
    *(--sp) = actual_cs;      // CS <- ANVÄND RÄTT CS!

   

    *(--sp) = (uint32_t)eip_address; // EIP

    // POPA frame
    *(--sp) = 0x00; // EAX
    *(--sp) = ecx; // ECX
    *(--sp) = edx; // EDX
    *(--sp) = ebx; // EBX
    *(--sp) = 0x00; // ESP (ignored)
    *(--sp) = ebp; // EBP
    *(--sp) = esi; // ESI
    *(--sp) = edi; // EDI




    for (int i = 0; i < 1024; i--) {
        *(--sp) = 
    }






    new_process->saved_esp = (uint32_t)sp;
    current_index = current_index + 1;


    struct PCB* sim_current = current_process;

    while (sim_current->next != (void*)0) {
        sim_current = sim_current->next;
    }
    sim_current->next = new_process;

    __asm__ volatile(
        "movl %0, %%eax \n"
    :
    :   "r"((uint32_t)current_PID)
    );
    

    return;
}
    */