
#include <stdint.h>
#include <stdarg.h>
#include "../include/vga.h"
#include "../include/memory.h"
#include "../include/pmm.h"
#include "../include/paging_manager.h"

extern uint32_t _kernel_page_dir[];
extern uint32_t _kernel_page_table[];
extern uint32_t _heap_start[];
extern uint32_t _kernel_page_tables_base[];
volatile uint32_t* kernel_page_dir = (uint32_t*) _kernel_page_dir; // This is the hardcoded in linker page directory for the kernel. 
volatile uint32_t* kernel_page_table = (uint32_t*) _kernel_page_table; // This is the hardcoded in linker page table for the kernel.

void map_kernel_memory();

void test_mapping() {
    kprintf("=== MAPPING TEST ===\n");
    
    // Test 1: kernel_page_dir physical address
    uint32_t phys_dir = (uint32_t)kernel_page_dir - 0xC0000000;
    kprintf("Page dir physical: %x\n", phys_dir);
    
    // Test 2: Check entry 0 (identity map) points to valid table
    uint32_t table0_phys = kernel_page_dir[0] & ~0xFFF;
    uint32_t* table0 = (uint32_t*)(table0_phys + 0xC0000000);
    kprintf("Table0 first entry: %x\n", table0[0]);
    kprintf("Table0 should be: 0x3 (addr 0x0 + flags)\n");

    // Test 3: Check entry 768 (0xC0000000) points to valid table
    uint32_t table768_phys = kernel_page_dir[768] & ~0xFFF;
    uint32_t* table768 = (uint32_t*)(table768_phys + 0xC0000000);
    kprintf("Table768 first entry: %x\n", table768[0]);
    kprintf("Table768 should map to physical 0x0\n");

    // Test 4: Make sure kernel code virtual address maps correctly
    uint32_t test_virt = (uint32_t)&test_mapping;
    uint32_t test_dir_idx = test_virt >> 22;
    uint32_t test_tbl_idx = (test_virt >> 12) & 0x3FF;
    uint32_t test_table_phys = kernel_page_dir[test_dir_idx] & ~0xFFF;
    uint32_t* test_table = (uint32_t*)(test_table_phys + 0xC0000000);
    kprintf("test_mapping() virt: %x\n", test_virt);
    kprintf("dir_index: %x table_index: %x\n", test_dir_idx, test_tbl_idx);
    kprintf("page entry: %x\n", test_table[test_tbl_idx]);
    kprintf("should be physical: %x\n", test_virt - 0xC0000000);

    kprintf("entry for 0xC0121000: %x\n", table768[289]);
    

    kprintf("=== END TEST ===\n");
}

void setup_entries() {
    for (int i = 0; i < 1024; i++) { // Here we go through the entier dir and table and default them to 0 so we can use them and make sure no trash presists
        kernel_page_dir[i] = 0x0; // Not present
        kernel_page_table[i] = 0x0; // Not presen
    }
}

void set_CR3_register() {

    //test_mapping();
    uint32_t phys = (uint32_t)kernel_page_dir - 0xC0000000;
    asm volatile("movl %0, %%cr3\n" : : "r"(phys));
}

void set_CR0_32_bit_register(int on) {
    uint32_t value;
    

    asm volatile("movl %%cr0, %0" : "=r"(value));  // Läs FRÅN CR0

    if (on == 1) {
        value = (value) | 0x80000000;
    } else {
        value = (value) & 0x7FFFFFFF;
    }


    asm volatile("movl %0, %%cr0" : : "r"(value)); // Skriv TILL CR0

}






void map_kernel_memory() {
    uint32_t dir_index;
    uint32_t table_index;
    uint32_t* current_page_table = (void*)0;
    uint32_t last_dir_index = 0xFFFFFFFF;

    for (uint32_t addr = 0x0; addr < 0x40000000; addr += 4096) {
        uint32_t virtual_addr = addr + 0xC0000000;
        dir_index = (virtual_addr >> 22);
        table_index = ((virtual_addr >> 12) & 0x3FF);

        if (dir_index != last_dir_index) {
            /*
             * Use the pre-allocated BSS slot for this page table instead of
             * calling get_next_free_kernel_frame().
             *
             * dir_index runs from 768 to 1023 (256 values).  Slot 0 belongs
             * to dir_index 768, slot 1 to 769, etc.  Each slot is exactly
             * one page (4 KB = 1024 uint32_t entries) wide.
             *
             * The BSS is zeroed by the bootloader, so unused entries are
             * already 0 (not-present), which is correct.
             */
            uint32_t slot = dir_index - 768;
            current_page_table = (uint32_t*)((uint8_t*)_kernel_page_tables_base
                                             + slot * 4096);
            uint32_t phys = (uint32_t)current_page_table - 0xC0000000;
            kernel_page_dir[dir_index] = phys | PAGE_KERNEL;
            last_dir_index = dir_index;
        }

        current_page_table[table_index] = addr | PAGE_KERNEL;
    }
}

// Returns a virtual address
uint32_t* create_process_page_directory() {
    uint32_t phys = get_next_free_kernel_frame();
    uint32_t* proc_page_dir = (uint32_t*)(phys + 0xC0000000);
    
    // Zero it out
    for (int i = 0; i < 1024; i++)
        proc_page_dir[i] = 0;
    
    // Copy kernel entries (upper 256 PDEs = 0xC0000000 and above)
    for (int i = 768; i < 1024; i++)
        proc_page_dir[i] = kernel_page_dir[i];
    
    return proc_page_dir;
}


void init_paging() {


    kprintf_white("[MEM] Creating blank entries........");
    setup_entries();
    kprintf_green("[OK]\n");
    kprintf_white("[MEM] Mapping memory........");
    map_kernel_memory();
    //kprintf("kernel_page_dir[0]: %x\n", kernel_page_dir[0]);
    kprintf_green("[OK]\n");
    kprintf_white("[MEM] Assigning CR3 to paging dir........");
    set_CR3_register();
    kprintf_green("[OK]\n");
    kprintf_white("[MEM] Fliping the CR0 bit........");
    set_CR0_32_bit_register(1);
    kprintf_green("[OK]\n");
    
}






// KARANTÄN SOON





/*
void map_virtual_to_physical_address(uint32_t virtual_address, uint32_t choosen_physical_address) {

    uint32_t dir_index = (virtual_address >> 22);
    uint32_t table_index = ((virtual_address >> 12) & 0b00000000001111111111);

    // check if page is present
    if ((kernel_page_dir[dir_index] & 0x1) == 1) {
        // do nothing
    }

    if ((kernel_page_dir[dir_index] & 0x1) == 0) {
        kernel_page_dir[dir_index] = (uint32_t)paging_table1 | 0x3;    // Here we inbue it what kind of access it the process has. 0x3 means this has kernel access.
    }

    paging_table1[table_index] = (choosen_physical_address | 0x3);
}
    */
