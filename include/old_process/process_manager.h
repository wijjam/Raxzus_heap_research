#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#define SLEEP_SYSCALL_MAGIC 555555


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


} __attribute__((packed)); // __attribute__((packed)) just tells the  compiler to not add padding

typedef struct PCB {
    uint32_t saved_esp;
    struct registers reg;
    uint32_t PID;
    int sleep_time;

};
extern struct PCB* current_process;
extern struct PCB* next_process;

int copy_process();

void create_process(void (*function_address)());

void init_process_scheduler(void);

void schedule();


#endif