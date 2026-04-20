#ifndef PAGING_MANAGER_H
#define PAGING_MANAGER_H

#define END_OF_ADDRESSABLE_MEMORY 0xFFFFFFFF

#define PAGE_KERNEL  0x3   // present + read/write, kernel only
#define PAGE_USER    0x7   // present + read/write + userspace
#define PAGE_READONLY 0x1  // present, read only
#define PAGE_GLOBAL  0x100 // global bit — survives CR3 switches (requires CR4.PGE)

void setup_entries();

uint32_t get_physical_address(uint32_t viritual_adress);

void set_CR3_register();

void set_CR0_32_bit_register(int on);

void init_paging();

uint32_t* create_process_page_directory();



#endif