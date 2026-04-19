
#include <stdint.h>
#include <stdarg.h>
#include "../include/vga.h"
#include "../include/memory.h"
#include "../include/pmm.h"
#include "../include/paging_manager.h"

extern uint32_t _paging_dir[];
extern uint32_t _paging_table1[];
extern uint32_t _end[];
volatile uint32_t* page_dir = (uint32_t*) _paging_dir; // This is the hardcoded in linker page directory for the kernel. 
volatile uint32_t* paging_table1 = (uint32_t*) _paging_table1; // This is the hardcoded in linker page table for the kernel.
volatile uint32_t* start_of_heap = (uint32_t*) _end;


void setup_entries() {
    for (int i = 0; i < 1024; i++) { // Here we go through the entier dir and table and default them to 0 so we can use them and make sure no trash presists
        page_dir[i] = 0x0; // Not present
        paging_table1[i] = 0x0; // Not presen
    }
}

uint32_t get_physical_address(uint32_t virtual_address) {
    uint32_t dir_index = (virtual_address >> 22); // The 10 left most bits are the directory index.
    uint32_t table_index = ((virtual_address >> 12) & 0b00000000001111111111); // The 12 middle bits are the table index

    // Kolla om directory entry finns
    if ((page_dir[dir_index] & 0x1) == 0) {
        return 0; // Inte mappat
    }
    
    // Kolla om table entry finns
    if ((paging_table1[table_index] & 0x1) == 0) {
        return 0; // Inte mappat
    }


    uint32_t physical_page = paging_table1[table_index] & ~0xFFF;
    uint32_t offset = virtual_address & 0xFFF; // 0xFFF = right 12 bits of the address
    return physical_page + offset;
}

void set_CR3_register() {
    asm volatile("movl %0, %%cr3\n" : : "r"(page_dir));
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

void map_virtual_to_physical_address(uint32_t virtual_address, uint32_t choosen_physical_address) {

    uint32_t dir_index = (virtual_address >> 22);
    uint32_t table_index = ((virtual_address >> 12) & 0b00000000001111111111);

    // check if page is present
    if ((page_dir[dir_index] & 0x1) == 1) {
        // do nothing
    }

    if ((page_dir[dir_index] & 0x1) == 0) {
        page_dir[dir_index] = (uint32_t)paging_table1 | 0x3;    // Here we inbue it what kind of access it the process has. 0x3 means this has kernel access.
    }

    paging_table1[table_index] = (choosen_physical_address | 0x3);
}

void map_kernel_memory() {
    uint32_t heap_end = (uint32_t)start_of_heap + get_heap_size();

    for (uint32_t addr = 0; addr < 0x01000000; addr += 4096) {
        map_page((uint32_t*)page_dir, addr, addr, PAGE_KERNEL);
    }
}


uint32_t* create_process_page_directory() {
    uint32_t* proc_page_dir = allocate_page();


    for (int i = 0; i < 1024; i++) {
        proc_page_dir[i] = page_dir[i];
    }


    return proc_page_dir;

}


void map_page(uint32_t* process_dir, uint32_t virtual_address, uint32_t choosen_physical_address, uint32_t flags) {

    uint32_t dir_index = (virtual_address >> 22);
    uint32_t table_index = ((virtual_address >> 12) & 0b00000000001111111111);
    uint32_t* process_table;

    // check if page table is present
    if ((process_dir[dir_index] & 0x1) == 1) {
        process_table = (uint32_t*)(process_dir[dir_index] & ~0xFFF); // Strips the flags off the directory entry leaving just the physical address
    }
    // check if page table is present
    if ((process_dir[dir_index] & 0x1) == 0) {
        process_table = allocate_page();
        process_dir[dir_index] = (uint32_t)process_table | flags;
        
    }
    // writes to the page in the table.
    process_table[table_index] = (choosen_physical_address | flags);

}

void alloc_page(uint32_t* process_dir, uint32_t virtual_address, uint32_t type) { // type: 1 = kernel, 2 = user, 3 = read only
    uint32_t phys = allocate_page();
    if (type == 1) {
        // Kernel only
        map_page((uint32_t*)page_dir, virtual_address, phys, PAGE_KERNEL);
    } else if (type == 2) {
        // Process (map in both kernel and process)
        map_page(process_dir, virtual_address, phys, PAGE_USER);  // Process can access
    } else if (type == 3) {
        // Read-only (both kernel and process)
        map_page(process_dir, virtual_address, phys, PAGE_READONLY);
    }
}



void init_paging() {
    
    kprintf_white("[MEM] Creating blank entries........");
    setup_entries();
    kprintf_green("[OK]\n");
    kprintf_white("[MEM] Mapping memory........");
    map_kernel_memory();
    kprintf("page_dir[0]: %x\n", page_dir[0]);
    kprintf_green("[OK]\n");
    kprintf_white("[MEM] Assigning CR3 to paging dir........");
    set_CR3_register();
    kprintf_green("[OK]\n");
    kprintf_white("[MEM] Fliping the CR0 bit........");
    set_CR0_32_bit_register(1);
    kprintf_green("[OK]\n");

    kprintf("The reserved_end is: %x", get_reserved_end());
}
