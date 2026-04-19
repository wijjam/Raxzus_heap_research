#include "../include/process_manager.h"
#include "../include/vga.h"
#include "../include/pic.h"
#include "../include/memory.h"
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

    struct PCB* new_process = (void*) kmalloc(sizeof(struct PCB));
    if (new_process == (void*)0) { return; }

    new_process->PID = current_index;
    new_process->sleep_time = 0;
    new_process->next = (void*)0;
    new_process->prev = (void*)0;

    // =========================================================
    // FIX 1: Give every process its own page directory.
    //
    // create_process_page_directory() allocates a fresh 4 KB
    // physical frame, zeroes it, then copies the kernel's upper
    // 256 entries (indices 768-1023, i.e. 0xC0000000 and above).
    // That means every process can still reach kernel code/data,
    // but the lower 3 GB are completely private to this process.
    // =========================================================
    uint32_t* proc_dir = create_process_page_directory();
    new_process->page_dir = proc_dir;

    // =========================================================
    // FIX 2: Give every process its own private stack.
    //
    // We allocate a PHYSICAL page and map it at PROC_STACK_VIRT
    // inside proc_dir.  That mapping does NOT exist in the kernel
    // page directory or any other process's page directory, so the
    // stack is truly isolated.
    //
    // While we are still running under the kernel's CR3 we can
    // reach that physical page at (stack_phys + 0xC0000000).
    // We build the fake interrupt frame there, then compute what
    // the CPU will see as the stack pointer when it runs this
    // process under proc_dir.
    // ========================================================

    uint32_t stack_phys = get_next_free_process_frame();
    uint32_t* sp;
    // IRET frame (highest address - pushed first)
    if (is_user) {
        map_page(proc_dir, PROC_STACK_VIRT, stack_phys, PAGE_USER);

    // Access the physical page through the kernel's identity window
        sp = (uint32_t*)(stack_phys + 0xC0000000 + PROC_STACK_SIZE);

            uint32_t code_phys = get_next_free_process_frame();
        map_page(proc_dir, PROC_CODE_VIRT, code_phys, PAGE_USER);

        kprintf("PROC_CODE_VIRT is: %x\n", PROC_CODE_VIRT);

            // copy the function into it
        uint8_t* dst = (uint8_t*)(code_phys + 0xC0000000);
        uint8_t* src = (uint8_t*)func;
        for (int i = 0; i < 4096; i++)
            dst[i] = src[i]; // Since we are in the same bin for our kernel code and user since we do not have file system yet. We instead copy the function code into a maped user memory.


        *(--sp) = 0x23;         // SS
        *(--sp) = PROC_STACK_VIRT + PROC_STACK_SIZE;  // ESP
        *(--sp) = 0x202;        // EFLAGS
        *(--sp) = 0x1B;         // CS
        *(--sp) = PROC_CODE_VIRT; // EIP
    } else {
                
        map_page(proc_dir, PROC_STACK_VIRT, stack_phys, PAGE_KERNEL);

    // Access the physical page through the kernel's identity window
        sp = (uint32_t*)(stack_phys + 0xC0000000 + PROC_STACK_SIZE);
        *(--sp) = 0x202;        // EFLAGS
        *(--sp) = 0x08;         // CS
        *(--sp) = (uint32_t)func; // EIP
    }

    // POPA frame (lower address - pushed second)
    // popa pops: EDI, ESI, EBP, (skip ESP), EBX, EDX, ECX, EAX
    *(--sp) = 0;  // EAX
    *(--sp) = 0;  // ECX
    *(--sp) = 0;  // EDX
    *(--sp) = 0;  // EBX
    *(--sp) = 0;  // ESP  (popa discards this dword, value irrelevant)
    *(--sp) = 0;  // EBP
    *(--sp) = 0;  // ESI
    *(--sp) = 0;  // EDI  <- sp points here; this is saved_esp

    // Convert the kernel-virtual sp to the process-virtual equivalent.
    // The offset within the physical page is the same in both address
    // spaces; only the base address differs.
    uint32_t sp_offset = (uint32_t)sp - (stack_phys + 0xC0000000);
    new_process->saved_esp = PROC_STACK_VIRT + sp_offset;
    new_process->stack_top = PROC_STACK_VIRT + PROC_STACK_SIZE;

    // =========================================================
    // FIX 3: Give every process its own private heap.
    //
    // Same idea as the stack: allocate a physical page, map it
    // at PROC_HEAP_VIRT inside proc_dir, and initialise the heap
    // header through the kernel's physical window.
    //
    // We CANNOT write to PROC_HEAP_VIRT directly here because
    // that virtual address only exists in proc_dir, not in the
    // currently active kernel page directory.
    // =========================================================
    uint32_t heap_phys = get_next_free_process_frame();
    map_page(proc_dir, PROC_HEAP_VIRT, heap_phys, PAGE_KERNEL);

    struct heap* proc_heap_hdr = (struct heap*)(heap_phys + 0xC0000000);
    proc_heap_hdr->size     = PROC_HEAP_SIZE;
    proc_heap_hdr->prev_size = 0;
    set_flag (&proc_heap_hdr->size, FREE);
    set_magic(&proc_heap_hdr->size, MAGIC_FIRST);

    new_process->heap_start = PROC_HEAP_VIRT;
    new_process->heap_end   = PROC_HEAP_VIRT + PROC_HEAP_SIZE;
    new_process->next_virt  = PROC_HEAP_VIRT + PROC_HEAP_SIZE;

        void *kstack = kmalloc(4096);
    if (!kstack) {
        kprintf_red("Failed to allocate kernel stack for process");
        asm volatile("hlt");
    }
    new_process->kernel_stack_top = (uint32_t)kstack + 4096;


    // =========================================================
    // Add to the circular linked list of processes
    // =========================================================
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