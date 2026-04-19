#include "../include/process_manager.h"
#include "../include/vga.h"
#include "../include/pic.h"
#include "../include/memory.h"
#include "../include/paging_manager.h"
#include "../include/pmm.h"

struct PCB* current_process;
struct PCB* next_process;

struct PCB* process_lists;

uint16_t current_index = 0;

uint32_t blink_counter = 0;

//struct PCB process_lists[1000];

void timer_interrupt_handler(uint32_t esp_address_variable) {
            
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

void create_process(void (*func)()) {
    struct PCB* new_process = (void*) kmalloc(sizeof(struct PCB));

    if (new_process == (void*)0) {return (void*)0;}

    new_process->PID = current_index;
    new_process->sleep_time = 0;
    new_process->next = (void*)0;
    new_process->prev = (void*)0;
    
    new_process->page_dir = create_process_page_directory();
    new_process->heap_start = get_process_heap_start();   // Puts heap at the bottom
    new_process->heap_end = new_process->heap_start + get_heap_size();
    new_process->stack_top = 0xEFFFF000;    // Puts stack at the top



    for (uint32_t addr = get_process_heap_start(); addr < new_process->heap_end; addr += 4096) { // Mapping heap pages 
        alloc_page(new_process->page_dir, addr, 2);   
    }

    for (int i = 0; i < 8; i++) {
        uint32_t virt = (0xEFFFF000 + 4096) - (i * 4096);
        kprintf("Mapping stack page at virt: %x\n", virt);
        alloc_page(new_process->page_dir, virt, 2);
        kprintf("Mapped successfully\n");
    }
    

    //heap_init(new_process); // initialize the process heap

    // Get ACTUAL CS from CPU
    uint16_t actual_cs;

    uint32_t *sp = (uint32_t*)(new_process->stack_top + 4096); // Since stack moves downward we make the stack pointer (esp) point at the top of the heap and then move down.

    // IRET frame
    *(--sp) = 0x202;          // EFLAGS
    *(--sp) = actual_cs;      // CS <- ANVÄND RÄTT CS!
    *(--sp) = (uint32_t)func; // EIP

    *(--sp) = 0x00; // ERROR CODE (ignored)

    // POPA frame
    *(--sp) = 0x00; // EAX
    *(--sp) = 0x00; // ECX
    *(--sp) = 0x00; // EDX
    *(--sp) = 0x00; // EBX
    *(--sp) = 0x00; // ESP (ignored)
    *(--sp) = 0x00; // EBP
    *(--sp) = 0x00; // ESI
    *(--sp) = 0x00; // EDI


    new_process->saved_esp = (uint32_t)sp;

    if (current_index == 0) {
        process_lists = new_process;
        current_index = current_index + 1;
        return;
    }


    current_index = current_index + 1;

    struct PCB* current = process_lists;

    while (current->next != (void*)0) {
        current = current->next;
    }

    current->next = new_process;
    new_process->prev = current;
}

void init_process_scheduler(void (*func)()) {
    current_index = 0;

    create_process(func);

    current_process = process_lists;
    next_process = process_lists;
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