#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#define SLEEP_SYSCALL_MAGIC 555555
#define MAX_PROCESSES 100

/* Per-process virtual address layout.
 * Each process gets its own physical pages mapped at these fixed virtual
 * addresses inside that process's own page directory.  Because user-space
 * entries (dir indices 0-767) are NOT copied from the kernel page directory,
 * these mappings are completely private to each process.
 */
#define PROC_STACK_VIRT  0xBFFFF000   /* 4 KB private stack, grows down */
#define PROC_STACK_SIZE  4096
#define PROC_HEAP_VIRT   0x40000000   /* 4 KB private heap, grows up   */
#define PROC_HEAP_SIZE   4096
#define PROC_CODE_VIRT 0x10000000


#include <stdint.h>

typedef struct registers { // This is a map for the stack. Nothing more

    uint32_t edi;       // [0]
    uint32_t esi;       // [1]
    uint32_t ebp;       // [2]
    uint32_t esp;       // [3]
    uint32_t ebx;       // [4]
    uint32_t edx;       // [5]
    uint32_t ecx;       // [6]
    uint32_t eax;       // [7]
    
    uint32_t err_code;  // [8]

    uint32_t eip;       // [9]
    uint32_t cs;        // [10]
    uint32_t eflags;    // [11]

    uint32_t user_esp;  // [12]
    uint32_t ss;        // [13]

} __attribute__((packed)); // __attribute__((packed)) just tells the  compiler to not add padding

typedef struct PCB {
    uint32_t saved_esp;
    uint32_t* page_dir;
    uint32_t heap_start;
    uint32_t heap_end;
    uint32_t stack_top;
    uint32_t next_virt;
    struct registers reg;
    uint32_t PID;
    int sleep_time;
    uint32_t kernel_stack_top;
    struct PCB* next;
    struct PCB* prev;
};

extern struct PCB* current_process;
extern struct PCB* next_process;

int copy_process(uint32_t* esp_stack);

void create_process(void (*func)(), int is_user);

void init_process_scheduler(void (*func)());

void schedule();

void debug_print_esp(uint32_t esp_val);


#endif