#ifndef PAGING_MANAGER_H
#define PAGING_MANAGER_H

#define END_OF_ADDRESSABLE_MEMORY 0xFFFFFFFF

#define PAGE_KERNEL  0x3  // present + read/write, kernel only 0d000000000011
#define PAGE_USER    0x7  // present + read/write + userspace  0d000000000111
#define PAGE_READONLY 0x1 // present, read only                0d000000000001

void setup_entries();

uint32_t get_physical_address(uint32_t viritual_adress);

void set_CR3_register();

void set_CR0_32_bit_register(int on);

void init_paging();

uint32_t* create_process_page_directory();



#endif