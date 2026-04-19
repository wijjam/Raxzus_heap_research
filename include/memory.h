#ifndef MEMORY_H
#define MEMORY_H
#include "../include/process_manager.h"
#include <stdint.h>
#define kfree(pointer) do {  \
               \
    free(pointer);            \
    pointer = (void*)0;\
               \
               \
               \
               \
               \
} while (0)

#define pfree(pointer) do {  \
               \
    process_free(pointer);            \
    pointer = (void*)0;\
               \
               \
               \
               \
               \
} while (0)

#define HEAP_SIZE (1024 * 1024)
#define MAGIC_MIDDLE 0x1
#define MAGIC_FIRST 0x2
#define MAGIC_GONE 0x3

#define FLAG 0x4
#define MAGIC 0x3

#define ALLOCATED 0x1
#define FREE 0

#define OFFSET 8

 
// We will construct the heap like a linked list.
typedef struct heap {
    uint32_t size; // How big is this memory block.
    uint32_t prev_size; // The previous blocks size

};

typedef struct free_list {
    uint32_t size;
    uint32_t prev_size; 
    uint32_t next_free_ptr;
    uint32_t prev_free_ptr;

};

int get_flag(uint32_t size);
int get_magic(uint32_t size);
int get_true_size(uint32_t size);

uint32_t get_heap_size();

void __attribute__((noinline)) set_flag(uint32_t* size, uint32_t value);
void __attribute__((noinline)) set_magic(uint32_t* size, uint32_t value);

void __attribute__((noinline)) heap_init(struct PCB* new_process);
void kernel_heap_init();
void* kmalloc(uint32_t size);
void free(void* pointer);

void* pmalloc(uint32_t size);
void process_free(void* pointer);

void test_kmalloc_kfree();
void print_heap();


#endif