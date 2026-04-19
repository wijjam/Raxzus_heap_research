#ifndef PMM_H
#define PMM_H
#include <stdint.h>

#define IS_VIRTUAL(addr)  ((addr) >= 0xC0000000)
#define IS_PHYSICAL(addr) ((addr) <  0xC0000000)

#define ASSERT_VIRTUAL(addr)  if (!IS_VIRTUAL(addr))  { kprintf("ASSERT FAILED: Expected VIRTUAL got PHYSICAL: %x in %s\n", addr, __func__); return 0; }
#define ASSERT_PHYSICAL(addr) if (!IS_PHYSICAL(addr)) { kprintf("ASSERT FAILED: Expected PHYSICAL got VIRTUAL: %x in %s\n", addr, __func__); return 0; }

#define ALLOCATE 1
#define FREE 0

void set_frame(uint32_t frame_addr); // Here we turn a 0 in the bitmap to 1 because we allocate that frame/page
uint32_t clear_frame(uint32_t frame_addr); // Here we turn a 1 to 0 since we have freed a page and stoped using it
void init_pmm();
uint32_t* allocate_page(uint32_t* process_dir, uint32_t virtual_address, uint32_t flags);
void increment_used_frames();
void decrement_used_frames();
uint32_t get_next_free_kernel_frame();
void map_page(uint32_t* process_dir, uint32_t virtual_address, uint32_t available_physical, uint32_t flags); // Finds what bit out of the 32 bits we need to flip.
uint32_t get_next_free_process_frame();

#endif