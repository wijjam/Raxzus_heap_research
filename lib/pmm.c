#include "../include/pmm.h"
#include "../include/vga.h"
#include "../include/memory.h"

extern uint32_t _kernel_page_dir[];
extern uint32_t _kernel_page_table[];
volatile uint32_t* kernel_page_dire = (uint32_t*) _kernel_page_dir; // This is the hardcoded in linker page directory for the kernel. 
volatile uint32_t* kernel_page_tablee = (uint32_t*) _kernel_page_table; // This is the hardcoded in linker page table for the kernel.
// Helper functions:

uint32_t set_bit(int index, int bit, int status); // Sets the page bit ALLOCATED/1 or FREE/0
uint32_t calculate_page(int frame_addr); // calculates which page it is we are going to FREE or ALLOCATE.
uint32_t find_index(int frame); // Finds which index in the bitmask our page lives in
uint32_t find_bit(int frame); // Finds what bit out of the 32 bits we need to flip.


// This below finds the end of kernel memory address
extern uint32_t _heap_start[];
volatile uint32_t* virtual_kernel_memory_end;


const uint32_t bitmap_size = 32768;
uint32_t bitmap[32768]; // logic 4GB / 4KB = 1 048 576 pages. 1 048 576/32 bits-per-int = 32768 integers 
uint32_t max_frames = 1048576; // total pages
uint32_t used_frames = 0; // Since we page the entier kernel memory at start this used frame will fork for the rest.


void set_frame(uint32_t frame_addr) {
    ASSERT_PHYSICAL((uint32_t)frame_addr);

    uint32_t frame =  calculate_page(frame_addr);
    uint32_t index = find_index(frame);
    uint32_t bit = find_bit(frame);

    set_bit(index, bit, ALLOCATE);
}


uint32_t clear_frame(uint32_t frame_addr) {
    ASSERT_PHYSICAL((uint32_t)frame_addr);
    uint32_t frame =  calculate_page(frame_addr);
    uint32_t index = find_index(frame);
    uint32_t bit = find_bit(frame);

   return set_bit(index, bit, FREE);
}


void init_pmm() {
    virtual_kernel_memory_end = (uint32_t*)((uint32_t)_heap_start + get_heap_size());
    uint32_t physical_kernel_memory_end = (uint32_t)virtual_kernel_memory_end - 0xC0000000;
    /*kprintf("_heap_start: %x\n", (uint32_t)_heap_start);
    kprintf("get_heap_size(): %d\n", get_heap_size());
    kprintf("virtual_kernel_memory_end: %x\n", (uint32_t)virtual_kernel_memory_end);
    kprintf("physical_kernel_memory_end: %x\n", physical_kernel_memory_end);*/
    for (uint32_t i = 0; i < bitmap_size; i++) {
        bitmap[i] = 0;
    }
    
    for (uint32_t addr = 0; addr < physical_kernel_memory_end; addr += 4096) {
        set_frame(addr);
        used_frames++;
    }

}

// Returns a virtual address
uint32_t* allocate_page(uint32_t* process_dir, uint32_t virtual_address, uint32_t flags) {

    ASSERT_VIRTUAL((uint32_t)process_dir);   // process_dir must be virtualw


    uint32_t address = get_next_free_process_frame();
    if (!address) {return 0;}
    map_page(process_dir, virtual_address, address, flags);
    return (uint32_t*)(address + 0xC0000000);
}

void increment_used_frames() {
    used_frames = used_frames + 1;
}

void decrement_used_frames() {
    used_frames = used_frames - 1;
}

// Reaturns a physical address
uint32_t get_next_free_kernel_frame() {
    

    extern uint32_t _heap_start[];
    uint32_t safe_start = ((uint32_t)_heap_start - 0xC0000000 + 4095) & ~0xFFF; // round up to next page
    //kprintf("The safe start is: 0x%x", safe_start);
    uint32_t start_index = (safe_start / 4096) / 32;

    for (uint32_t index = start_index; index < bitmap_size; index++) {
        if (bitmap[index] == 0xFFFFFFFF) { 
            continue;
        }
        
        uint32_t bit_check = 1;
        for (int bit_position = 0; bit_position < 32; bit_position++) {
            uint32_t frame = (index * 32) + bit_position;
            uint32_t address = (frame * 4096);


            if ((bitmap[index] & bit_check) == 0) {
                if (address < safe_start) {
                    bit_check = bit_check << 1;
                    continue;
                }
                set_frame(address);
                used_frames++;
                //kprintf("Returning kernel frame: %x, index: %d, bit: %d\n", address, index, bit_position);
                return address;
            }
            bit_check = bit_check << 1;
        }
    }
    return 0;
}

//Returns a physical address
uint32_t get_next_free_process_frame() {

    for (uint32_t index = 0; index < bitmap_size; index++) {
        if (bitmap[index] == 0xFFFFFFFF) {
            continue;
        }

        uint32_t bit_check = 1;
        for (int bit_position = 0; bit_position < 32; bit_position++) {
            uint32_t frame = (index * 32) + bit_position;
            uint32_t address = (frame * 4096);
            if (address == 0) {
                kprintf_red("PMM RETURNED PHYSICAL 0! SHUT DOWN EVERYTHING!");
                while(1);
            }
            if ((bitmap[index] & bit_check) == 0) {

                set_frame(address);
                used_frames++;
                return address;
            }
            bit_check = bit_check << 1;
        }
    }
    return 0;
}

// NEVER PASS A PHYSICAL ADDRESS IN VIRTUAL_ADDRESS
void map_page(uint32_t* process_dir, uint32_t virtual_address, uint32_t available_physical, uint32_t flags) {

    ASSERT_VIRTUAL((uint32_t)process_dir);   // process_dir must be virtual
    ASSERT_PHYSICAL(available_physical);               // physical must be physical
    uint32_t dir_index = (virtual_address >> 22);
    uint32_t table_index = ((virtual_address >> 12) & 0x3FF);
    uint32_t* process_table;

    if ((process_dir[dir_index] & 0x1) == 1) {
        // Table already exists, get virtual pointer to it
        uint32_t phys = process_dir[dir_index] & ~0xFFF;
        process_table = (uint32_t*)(phys + 0xC0000000);
    } else {
        // Need a new table
        if (process_dir == kernel_page_dire && dir_index == 0) {
            process_table = (uint32_t*)kernel_page_tablee;
            process_dir[dir_index] = ((uint32_t)process_table - 0xC0000000) | flags;
        } else {
            uint32_t phys = get_next_free_kernel_frame();
            process_table = (uint32_t*)(phys + 0xC0000000);
            process_dir[dir_index] = phys | flags;
        }
    }

    process_table[table_index] = available_physical | flags;
}

// ========================================================== HELP FUNCTIONS ===============================================================


uint32_t set_bit(int index, int bit, int status) {
    if (status == ALLOCATE) {
        return bitmap[index] |= (1 << bit);
    } else if (status == FREE) {
        return bitmap[index] &= ~(1 << bit);
    } else {
        kprintf_red("Error: you can't set a paging bit to anything but 0 and 1.");
        return 0;
    }
}

uint32_t calculate_page(int frame_addr) {
    return frame_addr / 4096;
}

uint32_t find_index(int frame) {
    return frame / 32;
}

uint32_t find_bit(int frame) {
    return frame % 32;
}




