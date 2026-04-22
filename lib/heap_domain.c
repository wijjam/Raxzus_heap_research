// Copyright (c) 2026 William Berglind
// Raxzus Flow — MMU-backed domain heap allocator
// Licensed under the Apache License 2.0

// heap_domain.c
#include "heap_domain.h"
#include "../include/pmm.h"
#include "../include/paging_manager.h"
#include "../include/vga.h"

#define NULL (void*)0
#define PREMAP_PAGES 256 // To map 1MB of pages on each domain at start and when a process is made

// Global heap domains
heap_domain_t heap_64;
heap_domain_t heap_128;
heap_domain_t heap_256;
heap_domain_t heap_512;
heap_domain_t heap_1k;
heap_domain_t heap_2k;
heap_domain_t heap_4k;

// Flush a single TLB entry.
static inline void invlpg(uint32_t virt) {
    asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

// This function initializes the different size classes and pre-maps 1MB of virtual pages.

void init_heap_domain(heap_domain_t* domain, uint32_t block_size, uint32_t virt_base) {
    kprintf("[HEAP_DBG] init_heap_domain: block_size=%d virt_base=0x%x\n", block_size, virt_base);

    // 1. Allocate page directory for this size class
    uint32_t phys_dir = get_next_free_kernel_frame();
    if (phys_dir == 0) {
        kprintf_red("[HEAP] FATAL: PMM out of frames for page directory (size=%d)\n", block_size);
        while(1);
    }
    kprintf("[HEAP_DBG] phys_dir = 0x%x  virt = 0x%x\n", phys_dir, phys_dir + 0xC0000000);
    domain->page_directory = (uint32_t*)(phys_dir + 0xC0000000);

    // 2. Zero the page directory
    for (int i = 0; i < 1024; i++) {
        domain->page_directory[i] = 0;
    }
    kprintf("[HEAP_DBG] page directory zeroed\n");

    // 3. Copy kernel mappings (entries 768-1023) so CR3 switches don't lose
    //    access to kernel code/data/stack.
    extern uint32_t _kernel_page_dir[];
    uint32_t* kernel_dir = (uint32_t*)_kernel_page_dir;
    for (int i = 768; i < 1024; i++) {
        domain->page_directory[i] = kernel_dir[i];
    }
    kprintf("[HEAP_DBG] kernel entries copied\n");

    // 4. Metadata
    domain->block_size    = block_size;
    domain->blocks_per_page = 4096 / block_size;
    domain->next_virt_addr  = virt_base;
    domain->next_free_block = NULL;

    kprintf("[HEAP_DBG] init_heap_domain done\n");

    extern uint32_t _kernel_page_dir[];

    for (uint32_t p = 0; p < PREMAP_PAGES; p++) {
        uint32_t phys = get_next_free_process_frame();
        if (phys == 0) {
            kprintf_red("[HEAP] FATAL: PMM out of frames durring pre-map");
            while(1);
        }

        // double map domain CR3 and kernel CR3
        uint32_t virt = domain->next_virt_addr;
        map_page(domain->page_directory, virt, phys, PAGE_KERNEL);
        map_page((uint32_t*)_kernel_page_dir, virt, phys, PAGE_KERNEL);
        invlpg(virt); // we do the flush

            // Build free list inside this page
        for (uint32_t i = 0; i < domain->blocks_per_page - 1; i++) {
            uint32_t block_addr = virt + (i * domain->block_size);
            uint32_t next_block = virt + ((i + 1) * domain->block_size);
            *(uint32_t*)block_addr = next_block;
        }

            // Last block points to current free list head (chains pages together)
        uint32_t last_block = virt + ((domain->blocks_per_page - 1) * domain->block_size);
        *(uint32_t*)last_block = (uint32_t)domain->next_free_block;

        domain->next_free_block = (void*)virt;
        
        domain->next_virt_addr += 4096;
    }

}

// Function takes care of the heap allocations that are equal to or less than 4KB
// Fast path pop next_free_block from LIFO
void* heap_domain_alloc(heap_domain_t* domain) {



    if (domain->next_free_block != NULL) {
        // Fast path — no CR3 switch needed
        void* ptr = domain->next_free_block;
        domain->next_free_block = (void*)(*(uint32_t*)ptr);
        return ptr;
    }



    // Convert the domain's virtual page directory pointer to its physical address.
    // CR3 must always hold a physical address — the CPU uses it before virtual
    // translation is possible.
    uint32_t phys_cr3 = (uint32_t)domain->page_directory - 0xC0000000;



    // If the free list is empty we need to back this domain with a new physical page.
    if (domain->next_free_block == NULL) {

        // Ask the PMM for a free physical frame.
        uint32_t phys = get_next_free_process_frame();
        if (phys == 0) {
            kprintf_red("[HEAP] FATAL: PMM out of frames during alloc\n");
            //asm volatile("mov %0, %%cr3" : : "r"(old_cr3) : "memory");
            return NULL;
        }

        // The virtual address where this page will appear inside this domain.
        // Each domain owns a contiguous virtual region (e.g. 0x10000000+ for 64B).
        // next_virt_addr advances by one page each time we expand the domain.
        uint32_t virt = domain->next_virt_addr;

        // Map the physical frame into this domain's page directory so it is
        // reachable when this domain's CR3 is loaded.  This is what gives each
        // size class its own isolated, virtually-contiguous address space.
        map_page(domain->page_directory, virt, phys, PAGE_KERNEL);
        invlpg(virt);

        // Also map the same physical frame into the kernel's page directory so
        // the returned pointer remains valid after we switch CR3 back.  Without
        // this the caller would page-fault on the very first dereference.
        extern uint32_t _kernel_page_dir[];
        map_page((uint32_t*)_kernel_page_dir, virt, phys, PAGE_KERNEL);
        invlpg(virt);

        // Initialise the free list inside the newly mapped page.
        // No separate metadata is allocated — each free block stores the virtual
        // address of the next free block in its own first 4 bytes.
        // When the block is handed to the caller those 4 bytes become user data.
        // This is why per-allocation overhead is exactly zero bytes.
        for (uint32_t i = 0; i < domain->blocks_per_page - 1; i++) {
            uint32_t block_addr = virt + (i * domain->block_size);
            uint32_t next_block = virt + ((i + 1) * domain->block_size);
            *(uint32_t*)block_addr = next_block; // block[i] -> block[i+1]
        }

        // The last block in the page terminates the list with NULL.
        uint32_t last_block = virt + ((domain->blocks_per_page - 1) * domain->block_size);
        *(uint32_t*)last_block = 0; // NULL — end of free list

        // Point the domain's free list head at the first block in the new page.
        domain->next_free_block = (void*)virt;

        // Advance the domain's virtual cursor so the next page expansion lands
        // immediately after this one — preserving virtual contiguity.
        domain->next_virt_addr += 4096;
    }

    // POP the head of the free list — O(1), one pointer read.
    // ptr is the block we are about to return to the caller.
    void* ptr = domain->next_free_block;

    // The first 4 bytes of ptr hold the address of the next free block.
    // Advance the head — this is the entire allocation algorithm.
    domain->next_free_block = (void*)(*(uint32_t*)ptr);

    return ptr;
}



void heap_domain_free(heap_domain_t* domain, void* ptr) {

    if (!ptr) { 
        return;
    }

    *(uint32_t*)ptr = (uint32_t)domain->next_free_block;
    domain->next_free_block = ptr;

}

// -----------------------------------------------------------------------
// LARGE ALLOCATION DOMAIN
// Handles allocations > 4 KB by mapping N contiguous 4 KB pages.
// Virtual base: 0x80000000  (well above the seven size-class domains).
// Layout: [4-byte page-count header][user data ...]
// kfree_large reads the header to know how many pages to unmap.
// -----------------------------------------------------------------------

#define LARGE_VIRT_BASE 0x80000000

typedef struct {
    uint32_t* page_directory;
    uint32_t  next_virt_addr;
    uint32_t* next_free_virt;
} large_domain_t;

static large_domain_t large_domain;

static void init_large_domain(void) {
    uint32_t phys_dir = get_next_free_kernel_frame();
    if (phys_dir == 0) {
        kprintf_red("[HEAP] FATAL: no frame for large domain page directory\n");
        while(1);
    }
    large_domain.page_directory = (uint32_t*)(phys_dir + 0xC0000000);

    for (int i = 0; i < 1024; i++)
        large_domain.page_directory[i] = 0;

    extern uint32_t _kernel_page_dir[];
    uint32_t* kernel_dir = (uint32_t*)_kernel_page_dir;
    for (int i = 768; i < 1024; i++)
        large_domain.page_directory[i] = kernel_dir[i];

    large_domain.next_virt_addr = LARGE_VIRT_BASE;
    large_domain.next_free_virt = NULL;
}

// Allocate `size` bytes as N contiguous 4 KB pages.
// Returns a pointer to usable memory; the 4 bytes before it hold the
// page count so kfree_large can unmap exactly the right number of pages.
void* kmalloc_large(uint32_t size) {
    if (size == 0) return NULL;

    // How many 4 KB pages do we need for the user data plus the 4-byte header?
    uint32_t pages = (size + 4 + 4095) / 4096;


    uint32_t phys_cr3 = (uint32_t)large_domain.page_directory - 0xC0000000;


    uint32_t virt_start;

    if (large_domain.next_free_virt != NULL) {
        uint32_t candidate = (uint32_t)large_domain.next_free_virt;
        uint32_t freed_pages = *((uint32_t*)candidate + 1);
        // Only reuse if the freed slot has enough virtual space.
        if (freed_pages >= pages) {
            virt_start = candidate;
            large_domain.next_free_virt = (void*)(*(uint32_t*)candidate);
            large_unmap_page(virt_start, 1);  // free the hostage frame
        } else {
            // Slot too small — skip it, fall through to fresh virtual space.
            // The slot stays in the LIFO for a future smaller allocation.
            virt_start = large_domain.next_virt_addr;
            large_domain.next_virt_addr += pages * 4096;
        }
    } else {
        virt_start = large_domain.next_virt_addr;
        large_domain.next_virt_addr += pages * 4096;
    }

    extern uint32_t _kernel_page_dir[];

    // Map each page physically and into both the large domain's CR3 and the
    // kernel's CR3 so the returned pointer is always dereferenceable.
    for (uint32_t i = 0; i < pages; i++) {
        uint32_t phys = get_next_free_process_frame();
        if (phys == 0) {
            kprintf_red("[HEAP] FATAL: PMM out of frames in kmalloc_large\n");
            //asm volatile("mov %0, %%cr3" : : "r"(old_cr3) : "memory");
            return NULL;
        }
        uint32_t virt = virt_start + (i * 4096);
        map_page(large_domain.page_directory, virt, phys, PAGE_KERNEL);
        map_page((uint32_t*)_kernel_page_dir,  virt, phys, PAGE_KERNEL);
        invlpg(virt);
    }

    // Write the page count into the hidden 4-byte header at the very start.
    *(uint32_t*)virt_start = pages;


    // Return pointer past the header — this is what the caller sees.
    return (void*)(virt_start + sizeof(uint32_t));
}

// Free a large allocation.  Reads the hidden page count, zeroes out the
// page table entries, and flushes the TLB for each page.
void large_unmap_page(uint32_t virt, int free_frame) {
    extern uint32_t _kernel_page_dir[];
    uint32_t dir_idx   = virt >> 22;
    uint32_t table_idx = (virt >> 12) & 0x3FF;

    if (large_domain.page_directory[dir_idx] & 0x1) {
        uint32_t  tbl_phys = large_domain.page_directory[dir_idx] & ~0xFFF;
        uint32_t* tbl_virt = (uint32_t*)(tbl_phys + 0xC0000000);
        tbl_virt[table_idx] = 0;
    }

    uint32_t* kdir = (uint32_t*)_kernel_page_dir;
    if (kdir[dir_idx] & 0x1) {
        uint32_t  tbl_phys = kdir[dir_idx] & ~0xFFF;
        uint32_t* tbl_virt = (uint32_t*)(tbl_phys + 0xC0000000);
        if (free_frame) clear_frame(tbl_virt[table_idx] & ~0xFFF);
        tbl_virt[table_idx] = 0;
    }

    invlpg(virt);
}

void kfree_large(void* ptr) {
    if (!ptr) return;

    uint32_t real_start = (uint32_t)ptr - sizeof(uint32_t);
    uint32_t pages = *(uint32_t*)real_start;

    // Free pages 1..N-1 — page 0 stays mapped so we can write the LIFO
    // chain pointer into it while it still has physical backing.
    for (uint32_t i = 1; i < pages; i++)
        large_unmap_page(real_start + (i * 4096), 1);

    // Page 0 is still mapped — store chain pointer at offset 0, page count at offset 4.
    *(uint32_t*)real_start       = (uint32_t)large_domain.next_free_virt;
    *((uint32_t*)real_start + 1) = pages;
    large_domain.next_free_virt  = (void*)real_start;
}

void init_all_heaps() {
    kprintf_white("[HEAP] Initializing heap domains........\n");

    init_heap_domain(&heap_64,  64,   0x10000000);
    init_heap_domain(&heap_128, 128,  0x20000000);
    init_heap_domain(&heap_256, 256,  0x30000000);
    init_heap_domain(&heap_512, 512,  0x40000000);
    init_heap_domain(&heap_1k,  1024, 0x50000000);
    init_heap_domain(&heap_2k,  2048, 0x60000000);
    init_heap_domain(&heap_4k,  4096, 0x70000000);
    init_large_domain();

    kprintf_green("[HEAP] All heap domains ready\n");
}

void* kmalloc(uint32_t size) {
    if (size <= 64)   return heap_domain_alloc(&heap_64);
    if (size <= 128)  return heap_domain_alloc(&heap_128);
    if (size <= 256)  return heap_domain_alloc(&heap_256);
    if (size <= 512)  return heap_domain_alloc(&heap_512);
    if (size <= 1024) return heap_domain_alloc(&heap_1k);
    if (size <= 2048) return heap_domain_alloc(&heap_2k);
    if (size <= 4096) return heap_domain_alloc(&heap_4k);
    return kmalloc_large(size);
}

void kfree_heap(void* ptr) {
    uint32_t addr = (uint32_t)ptr;
    if      (addr >= 0x10000000 && addr < 0x20000000) heap_domain_free(&heap_64,  ptr);
    else if (addr >= 0x20000000 && addr < 0x30000000) heap_domain_free(&heap_128, ptr);
    else if (addr >= 0x30000000 && addr < 0x40000000) heap_domain_free(&heap_256, ptr);
    else if (addr >= 0x40000000 && addr < 0x50000000) heap_domain_free(&heap_512, ptr);
    else if (addr >= 0x50000000 && addr < 0x60000000) heap_domain_free(&heap_1k,  ptr);
    else if (addr >= 0x60000000 && addr < 0x70000000) heap_domain_free(&heap_2k,  ptr);
    else if (addr >= 0x70000000 && addr < 0x80000000) heap_domain_free(&heap_4k,  ptr);
    else if (addr >= 0x80000000 && addr)                       kfree_large(ptr);
}
