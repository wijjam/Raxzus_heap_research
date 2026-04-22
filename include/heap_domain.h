// Copyright (c) 2026 William Berglind
// Raxzus Flow — MMU-backed domain heap allocator
// Licensed under the Apache License 2.0
// https://github.com/wijjam/Raxzus_heap_research

// heap_domain.h
#ifndef HEAP_DOMAIN_H
#define HEAP_DOMAIN_H

#include <stdint.h>


#define NULL (void*)0
#define kfree(ptr) do { kfree_heap(ptr); (ptr) = NULL; } while(0) // This is our reason why multiple free of the same pointer can't happen.

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
void  kfree_heap(void* ptr);
void* kmalloc_large(uint32_t size);
void  kfree_large(void* ptr);

#endif