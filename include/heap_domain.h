// heap_domain.h
#ifndef HEAP_DOMAIN_H
#define HEAP_DOMAIN_H

#include <stdint.h>

typedef struct {
    uint32_t* page_directory;    // CR3 for this size class (VIRTUAL address!)
    uint32_t block_size;         // 64, 128, 256, 512, 1K, 2K, 4K
    uint32_t blocks_per_page;    // 4096 / block_size
    
    void* next_free_block;       // Next available block (VIRTUAL)
    uint32_t next_virt_addr;     // Where to map next page
    uint32_t* next_free_virt;
} heap_domain_t;

void init_heap_domain(heap_domain_t* domain, uint32_t block_size, uint32_t virt_base);
void* heap_domain_alloc(heap_domain_t* domain);
void heap_domain_free(heap_domain_t* domain, void* ptr);

void init_all_heaps(void);
void* kmalloc(uint32_t size);
void  kfree(void* ptr);
void* kmalloc_large(uint32_t size);
void  kfree_large(void* ptr);

#endif