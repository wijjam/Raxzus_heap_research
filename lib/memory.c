#include "../include/memory.h"
#include "../include/vga.h"
#include "../include/process_manager.h"

struct heap* start_kernel_heap_memory;
struct heap* kernel_heap_end;


// ╔══════════════════════════════════════╗
// ║         KERNEL HEAP                  ║
// ╚══════════════════════════════════════╝

void kernel_heap_init() {

    extern _heap_start[];
    uint32_t* kernel_start_heap_memory = (uint32_t*) _heap_start;

    start_kernel_heap_memory = (struct heap*)kernel_start_heap_memory;

    // kernel_heap_end is now 8-bytes for the header memory and 8-bytes for data memory from the end
    kernel_heap_end = ((struct heap*)((char*)((struct heap*) kernel_start_heap_memory) + HEAP_SIZE - sizeof(struct heap) - OFFSET));
    kprintf("%x", start_kernel_heap_memory);
    
    start_kernel_heap_memory->size = HEAP_SIZE; // Here we give the heap 1MB, which includes all the headers
    start_kernel_heap_memory->prev_size = 0;
    set_flag(&(start_kernel_heap_memory->size), FREE);
    set_magic(&(start_kernel_heap_memory->size), MAGIC_FIRST);

    kprintf_white("[MEM] Initial heap size: %d\n", start_kernel_heap_memory->size);
    kprintf_white("[MEM] The sizeof heap: %d\n", sizeof(struct heap));
}

void* kmalloc(uint32_t size) {
    struct heap* current_block = start_kernel_heap_memory;
    //full_size_request is the real size the user is asking for. So they size they want for memory and then the hidden head.
    size = (size + 7) & ~7;
    uint32_t full_size_request = size + sizeof(struct heap);

    full_size_request = (full_size_request + 7) & ~7;

    if (full_size_request > HEAP_SIZE-sizeof(struct heap)) {
        kprintf_red("Requested size exceeds available heap size.\n");
        __asm__ volatile("hlt");
        return (void*)0;  // Too big for heap
    }

    if (size <= 0) { // This stays as just a size check since if the user writes 0 we will stop it and not assume.
        kprintf_red("Excuse me but you can't do anything with a memory with 0 byte. Good try but no..\n");
        kprintf_red("Problem encountered in the malloc size <= 0 check\n");
        __asm__ volatile("hlt");
        return (void*)0;
    }
   
    while (current_block < kernel_heap_end) {


        uint8_t flag = get_flag(current_block->size);
        uint8_t magic = get_magic(current_block->size);

        uint32_t data_size = current_block->size - sizeof(struct heap);
        
        // If the current block is free we pass into this check.
        // // We do not check for magic last because kernel_heap_end should not be able to get down here. But extra percasun

        if (flag == 0 && (magic == MAGIC_FIRST || magic == MAGIC_MIDDLE)) {
            
            // if the free block has memory enough for the requested size then we pass into this check.
            if (data_size >= full_size_request) {
                
                 uint32_t old_full_size = current_block->size;
                 //kprintf("The old full size when initialized: %d\n", old_full_size);
                // We check if the first block is the one we are checking. if it is we do not change the magic.
                
                if (current_block->prev_size == 0) {
                    
                    // The old full size is what will save and the be used to get the new size for the new block being split.
                    
                    current_block->prev_size = 0;
                    current_block->size = full_size_request;
                    set_flag(&(current_block->size), ALLOCATED);
                    set_magic(&(current_block->size), MAGIC_FIRST);

                } else {
                    // Implement logic to continue spliting and changing when not at the begining later here.
                    
                    current_block->size = full_size_request;
                    set_flag(&(current_block->size), ALLOCATED);
                    set_magic(&(current_block->size), MAGIC_MIDDLE);

                }
                // With this we move forward with just the requested size.
                current_block = (struct heap*)((char*)current_block + get_true_size(current_block->size));


                flag = get_flag(current_block->size);
                magic = get_magic(current_block->size);
                // Set middle here since this will always be after the first block and therefore always middle or last.
                // We check last below so this magic is safe and if we are on the last block then we change it to last.
                // We asume middle.

                current_block->prev_size = full_size_request;
                current_block->size = old_full_size - full_size_request;

                set_flag(&(current_block->size), FREE);
                set_magic(&(current_block->size), MAGIC_MIDDLE);



                struct heap* neighbor_right = (struct heap*)((char*)current_block + get_true_size(current_block->size));
                // We have to find the neighboor and check that it is inside of the heap end boundary.
                if (neighbor_right <= kernel_heap_end) {
                    // Here we make sure the neighboors previous_size also is updated so we do not point to some random old merged head or something.
                    neighbor_right->prev_size = get_true_size(current_block->size);
                    
                }

                //kprintf("\nThe addresses we get from malloc are: %d\n", ((char*)current_block - current_block->prev_size) + sizeof(struct heap));
                return ((char*)current_block - current_block->prev_size) + sizeof(struct heap);
            }
            

        }

        
        

        //kprintf("We are stuck WEEEEEEEE");

        // No check needed since the only place a next does not exist is on the kernel_heap_end
        // Since the while stops if we are on it, we wont have to make a check here for it.
        // Therefore we only go forward once and check again to find flag = 1;
        //kprintf("The size of the blocks being passed: %d, \n", current_block->size);
        current_block = (struct heap*)((char*)current_block + get_true_size(current_block->size));


    }

    return (void*)0;
}



void free(void* pointer) {

    
    // Checks the inputed pointer for NILL, exeding heap memory
    if (pointer == (void*)0 || pointer < (void*)start_kernel_heap_memory || pointer > (void*)kernel_heap_end) {
        kprintf_red("\n Sorry but you are trying to free a block that is out of bounds, or just trying to free a null 1\n");
        
        return;
    }
    // Checking if the current header gotten from the pointer is in the heap memory
    struct heap* current_block = (struct heap*)((char*)pointer - sizeof(struct heap));
    if (current_block < start_kernel_heap_memory) {
        kprintf_red("\n Sorry but you are trying to free a block that is out of bounds 2\n");
        
        return;
    }
    if (current_block->size == 0) {
        kprintf_red("\n Error: Block size is 0, indicating corruption.\n");
        __asm__ volatile("hlt");
        return;
    }

    uint8_t flag = get_flag(current_block->size);
    uint8_t magic = get_magic(current_block->size);
        
    

    // check if the current pointer is a merged and unusable header, if it is then print error and return. !
    //kprintf("Current_block magic: %d\n", current_block->magic);
    if (!(magic == MAGIC_FIRST || magic == MAGIC_MIDDLE)) {
        kprintf_red("\nEhm sorry but... freeing a invlid header is not really allowed so...... yeah\n");
        kprintf_red("\nTried to free a corrupted pointer in kfree()\n");
        __asm__ volatile("hlt");
        return;
    }
    if (flag == 0) {
        kprintf_red("\n Warning: Double-free attempt detected.\n");
        
        return;
    }

    // check if the size in current head is overflowing, if it is then print a error and return.
    if (current_block->size > HEAP_SIZE) {
        kprintf_red("Corrupted size in heap block exeding heap end");
        __asm__ volatile("hlt");
        return;
    }

    //current_block->flag = 0;




    set_flag(&(current_block->size), FREE);
    
    // We make sure that we only go backwards if we are not on the first block, because backwards from there is outside of the heap.
    if (magic != MAGIC_FIRST) {
        struct heap* prev_block = (struct heap*)((char*)current_block - current_block->prev_size);
        
        // Stop once we reach the left most block
        //kprintf("\n=========Just before the first loop =============\n");
        while (get_flag(prev_block->size) == 0) {
            flag = get_flag(prev_block->size);
            magic = get_magic(prev_block->size);
            //kprintf("The current size is: %d and the address is: %x\n", get_true_size(prev_block->size), prev_block);

            //kprintf("\nThe current_block->prev_size is: %d\n", current_block->prev_size);
            //kprintf("\nThe prev_block->size is: %d\n", get_true_size(prev_block->size));
            //kprintf("STARTING NEW LOOP===\n");
            if (current_block->prev_size != get_true_size(prev_block->size) && magic != MAGIC_FIRST) {
                kprintf_red(" The heap chain is corrupted, full kernel shutdown is needed.\n");
                __asm__ volatile("hlt");
                return;
            }

            // If we are on the first one we stop
            if (magic == MAGIC_FIRST) {
                break;
                // We just continue if we are on the middle.
            } else if (magic == MAGIC_MIDDLE) {

            } else {
                // If we reach this then there is a error since the block is useless.
                kprintf("The magic is: %d\n", magic);
                kprintf_red("Block chain error detected. in the back loop\n");
                __asm__ volatile("hlt");
                return;
            }

            current_block = prev_block;
            prev_block = (struct heap*)((char*)current_block - current_block->prev_size);

            struct heap* prev_prev_block = (struct heap*)((char*)prev_block - prev_block->prev_size);
            //kprintf("2 The current size is: %d and the address is: %x\n", get_true_size(prev_block->size), prev_block);
            //kprintf("2 The current size is: %d and the address is: %x\n", get_true_size(prev_prev_block->size), prev_prev_block);
        }
    }

    //kprintf("\nThe block size is: %d\n the flag is: %d\n the magic is: %d\n", get_true_size(current_block->size), flag, magic);

    // Check if the next header block to the right has a valid address inside the heap and if it's previous variable is valid.
    



    struct heap* next_block = ((struct heap*)((char*)current_block + get_true_size(current_block->size)));
    // Save the current left most block in start_block.
    struct heap* start_block = current_block;
    if ((char*)next_block - next_block->prev_size != (char*)current_block) {
        kprintf_red("UGH next blocks previous is not current. error..\n");
        __asm__ volatile("hlt");
        return;
    } 

    //kprintf("The start block and current block size is: %d\n", next_block->size);

    while (next_block < kernel_heap_end) { 
            flag = get_flag(next_block->size);
            magic = get_magic(next_block->size);
            
        //kprintf("2 The start block and current block size is: %d the next block size is: %d\n", current_block->size, next_block->size);
        
        //kprintf("The flag of next is: %d and magic of next is: %d, and size: %d", flag, magic,get_true_size(next_block->size));
        if (flag == 0 && magic != MAGIC_GONE) {
            //kprintf("The next block size is: %d\n", next_block->size);
            start_block->size += get_true_size(next_block->size);
            set_magic(&(next_block->size), MAGIC_GONE);



            ((struct heap*)((char*)next_block + (get_true_size(next_block->size))))->prev_size = get_true_size(start_block->size);

            if (get_true_size(start_block->size) > HEAP_SIZE) {
                kprintf_red(" This is illegal!!!\n");
                kprintf("For some reason in the second loop the size of start_block is: %d\n", start_block->size);
                __asm__ volatile("hlt");
            }
            
            current_block = next_block;
            next_block = ((struct heap*)((char*)current_block + (get_true_size(next_block->size))));

        } else {
            break;
        }    
    }

    // ADD TO LIST TODO.

}


// ╔══════════════════════════════════════╗========================================================================
// ║         PROCESS HEAP                 ║                                                                       =
// ╚══════════════════════════════════════╝========================================================================



void heap_init(struct PCB* new_process) {
    struct heap* start_process_heap_memory = (struct heap*)new_process->heap_start;
    uint32_t heap_size = new_process->heap_end - new_process->heap_start;
    
    ((struct heap*)new_process->heap_start)->size = heap_size; 
    ((struct heap*)new_process->heap_start)->prev_size = 0;
    set_flag(&(((struct heap*)new_process->heap_start)->size), FREE);
    set_magic(&(((struct heap*)new_process->heap_start)->size), MAGIC_FIRST);

}


void* pmalloc(uint32_t size) {
    kprintf("[PMALLOC] entered\n");
    struct heap* current_block = (struct heap*)current_process->heap_start;
    struct heap* heap_end = (struct heap*)((char*)current_process->heap_end - sizeof(struct heap) - OFFSET);

        kprintf("[PMALLOC] heap_end: %x current_block size: %x\n", heap_end, current_block->size);


    uint32_t heap_size = (uint32_t)heap_end - (uint32_t)current_block;

    //full_size_request is the real size the user is asking for. So they size they want for memory and then the hidden head.
    size = (size + 7) & ~7;
    uint32_t full_size_request = size + sizeof(struct heap);

    full_size_request = (full_size_request + 7) & ~7;




    if (full_size_request > heap_size-sizeof(struct heap)) {
        kprintf_red("Requested size exceeds available heap size.\n");
        __asm__ volatile("hlt");
        return (void*)0;  // Too big for heap
    }

    if (size <= 0) { // This stays as just a size check since if the user writes 0 we will stop it and not assume.
        kprintf_red("Excuse me but you can't do anything with a memory with 0 byte. Good try but no..\n");
        kprintf_red("Problem encountered in the malloc size <= 0 check\n");
        __asm__ volatile("hlt");
        return (void*)0;
    }
   
    while (current_block < heap_end) {


        uint8_t flag = get_flag(current_block->size);
        uint8_t magic = get_magic(current_block->size);

        uint32_t data_size = current_block->size - sizeof(struct heap);
        
        // If the current block is free we pass into this check.
        // // We do not check for magic last because heap_end should not be able to get down here. But extra percasun

        if (flag == 0 && (magic == MAGIC_FIRST || magic == MAGIC_MIDDLE)) {
            
            // if the free block has memory enough for the requested size then we pass into this check.
            if (data_size >= full_size_request) {
                
                 uint32_t old_full_size = current_block->size;
                 //kprintf("The old full size when initialized: %d\n", old_full_size);
                // We check if the first block is the one we are checking. if it is we do not change the magic.
                
                if (current_block->prev_size == 0) {
                    
                    // The old full size is what will save and the be used to get the new size for the new block being split.
                    
                    current_block->prev_size = 0;
                    current_block->size = full_size_request;
                    set_flag(&(current_block->size), ALLOCATED);
                    set_magic(&(current_block->size), MAGIC_FIRST);

                } else {
                    // Implement logic to continue spliting and changing when not at the begining later here.
                    
                    current_block->size = full_size_request;
                    set_flag(&(current_block->size), ALLOCATED);
                    set_magic(&(current_block->size), MAGIC_MIDDLE);

                }
                // With this we move forward with just the requested size.
                current_block = (struct heap*)((char*)current_block + get_true_size(current_block->size));


                flag = get_flag(current_block->size);
                magic = get_magic(current_block->size);
                // Set middle here since this will always be after the first block and therefore always middle or last.
                // We check last below so this magic is safe and if we are on the last block then we change it to last.
                // We asume middle.

                current_block->prev_size = full_size_request;
                current_block->size = old_full_size - full_size_request;

                set_flag(&(current_block->size), FREE);
                set_magic(&(current_block->size), MAGIC_MIDDLE);



                struct heap* neighbor_right = (struct heap*)((char*)current_block + get_true_size(current_block->size));
                // We have to find the neighboor and check that it is inside of the heap end boundary.
                if (neighbor_right <= heap_end) {
                    // Here we make sure the neighboors previous_size also is updated so we do not point to some random old merged head or something.
                    neighbor_right->prev_size = get_true_size(current_block->size);
                    
                }

                //kprintf("\nThe addresses we get from malloc are: %d\n", ((char*)current_block - current_block->prev_size) + sizeof(struct heap));
                return ((char*)current_block - current_block->prev_size) + sizeof(struct heap);
            }
            

        }

        
        

        //kprintf("We are stuck WEEEEEEEE");

        // No check needed since the only place a next does not exist is on the heap_end
        // Since the while stops if we are on it, we wont have to make a check here for it.
        // Therefore we only go forward once and check again to find flag = 1;
        //kprintf("The size of the blocks being passed: %d, \n", current_block->size);
        current_block = (struct heap*)((char*)current_block + get_true_size(current_block->size));


    }

    return (void*)0;
}



void process_free(void* pointer) {

    struct heap* heap_end = (struct heap*)((char*)current_process->heap_end - sizeof(struct heap) - OFFSET);
    struct heap* start_process_heap_memory = ( struct heap*)current_process->heap_start;
    uint32_t heap_size = (uint32_t)heap_end - (uint32_t)current_process->heap_start;
    
    // Checks the inputed pointer for NILL, exeding heap memory
    if (pointer == (void*)0 || pointer < (void*)start_process_heap_memory || pointer > (void*)heap_end) {
        kprintf_red("\n Sorry but you are trying to free a block that is out of bounds, or just trying to free a null 1\n");
        
        return;
    }
    // Checking if the current header gotten from the pointer is in the heap memory
    struct heap* current_block = (struct heap*)((char*)pointer - sizeof(struct heap));
    if (current_block < start_process_heap_memory) {
        kprintf_red("\n Sorry but you are trying to free a block that is out of bounds 2\n");
        
        return;
    }
    if (current_block->size == 0) {
        kprintf_red("\n Error: Block size is 0, indicating corruption.\n");
        __asm__ volatile("hlt");
        return;
    }

    uint8_t flag = get_flag(current_block->size);
    uint8_t magic = get_magic(current_block->size);
        
    

    // check if the current pointer is a merged and unusable header, if it is then print error and return. !
    //kprintf("Current_block magic: %d\n", current_block->magic);
    if (!(magic == MAGIC_FIRST || magic == MAGIC_MIDDLE)) {
        kprintf_red("\nEhm sorry but... freeing a invlid header is not really allowed so...... yeah\n");
        kprintf_red("\nTried to free a corrupted pointer in kfree()\n");
        __asm__ volatile("hlt");
        return;
    }
    if (flag == 0) {
        kprintf_red("\n Warning: Double-free attempt detected.\n");
        
        return;
    }

    // check if the size in current head is overflowing, if it is then print a error and return.
    if (current_block->size > heap_size) {
        kprintf_red("Corrupted size in heap block exeding heap end");
        __asm__ volatile("hlt");
        return;
    }



    set_flag(&(current_block->size), FREE);
    
    // We make sure that we only go backwards if we are not on the first block, because backwards from there is outside of the heap.
    if (magic != MAGIC_FIRST) {
        struct heap* prev_block = (struct heap*)((char*)current_block - current_block->prev_size);
        
        // Stop once we reach the left most block
        //kprintf("\n=========Just before the first loop =============\n");
        while (get_flag(prev_block->size) == 0) {
            flag = get_flag(prev_block->size);
            magic = get_magic(prev_block->size);


            if (current_block->prev_size != get_true_size(prev_block->size) && magic != MAGIC_FIRST) {
                kprintf_red(" The heap chain is corrupted, full kernel shutdown is needed.\n");
                __asm__ volatile("hlt");
                return;
            }

            // If we are on the first one we stop
            if (magic == MAGIC_FIRST) {
                break;
                // We just continue if we are on the middle.
            } else if (magic == MAGIC_MIDDLE) {

            } else {
                // If we reach this then there is a error since the block is useless.
                kprintf("The magic is: %d\n", magic);
                kprintf_red("Block chain error detected. in the back loop\n");
                __asm__ volatile("hlt");
                return;
            }

            current_block = prev_block;
            prev_block = (struct heap*)((char*)current_block - current_block->prev_size);

            struct heap* prev_prev_block = (struct heap*)((char*)prev_block - prev_block->prev_size);
        }
    }

    //kprintf("\nThe block size is: %d\n the flag is: %d\n the magic is: %d\n", get_true_size(current_block->size), flag, magic);

    // Check if the next header block to the right has a valid address inside the heap and if it's previous variable is valid.
    



    struct heap* next_block = ((struct heap*)((char*)current_block + get_true_size(current_block->size)));
    // Save the current left most block in start_block.
    struct heap* start_block = current_block;
    if ((char*)next_block - next_block->prev_size != (char*)current_block) {
        kprintf_red("UGH next blocks previous is not current. error..\n");
        __asm__ volatile("hlt");
        return;
    } 

    //kprintf("The start block and current block size is: %d\n", next_block->size);

    while (next_block < heap_end) { 
            flag = get_flag(next_block->size);
            magic = get_magic(next_block->size);
            
        //kprintf("2 The start block and current block size is: %d the next block size is: %d\n", current_block->size, next_block->size);
        
        //kprintf("The flag of next is: %d and magic of next is: %d, and size: %d", flag, magic,get_true_size(next_block->size));
        if (flag == 0 && magic != MAGIC_GONE) {
            //kprintf("The next block size is: %d\n", next_block->size);
            start_block->size += get_true_size(next_block->size);
            set_magic(&(next_block->size), MAGIC_GONE);



            ((struct heap*)((char*)next_block + (get_true_size(next_block->size))))->prev_size = get_true_size(start_block->size);

            if (get_true_size(start_block->size) > heap_size) {
                kprintf_red(" This is illegal!!!\n");
                kprintf("For some reason in the second loop the size of start_block is: %d\n", start_block->size);
                __asm__ volatile("hlt");
            }
            
            current_block = next_block;
            next_block = ((struct heap*)((char*)current_block + (get_true_size(next_block->size))));

        } else {
            break;
        }    
    }

   

}













// HELPER FUNCTIONS =============================================================================================================================================================


int get_flag(uint32_t size) {
    //kprintf("%d", (size & FLAG));
    return ((size & FLAG)/4);
}

int get_magic(uint32_t size) {
    //kprintf("%d", (size & MAGIC));
    return (size & MAGIC);
}

int get_true_size(uint32_t size) {
    return size & ~0x7;
}

void set_flag(uint32_t* size, uint32_t value) {
    if (value == 1) {
        *size = (*size) | FLAG;   
    } else if (value == 0) {
        (*size) = (*size) & ~FLAG;
    }
}

void set_magic(uint32_t* size, uint32_t value) {

    (*size) = (*size) & ~MAGIC;
    *size = (*size) | value;

}

uint32_t get_heap_size() {
    return HEAP_SIZE;
}




// DEBUG AND STRESS TEST =========================================================================================================================================


void test_kmalloc_kfree() {
    
kprintf("\n========== MERGE STRESS TEST ==========\n");


    // Test 1: Simple merge - allokera 3, free alla
    kprintf("\n--- Test 1: Sequential merge ---\n");
    char* a = kmalloc(100);
    char* b = kmalloc(100);
    char* c = kmalloc(100);
    kprintf("Allocated 3 blocks\n");
    print_heap();
    
    kfree(a);
    kfree(b);
    kfree(c);
    kprintf("Freed all - should be one big block and processes remaining\n");
    print_heap();
    
    // Test 2: Merge backwards (free i omvänd ordning)
    kprintf("\n--- Test 2: Reverse order merge ---\n");
    a = kmalloc(50);
    b = kmalloc(50);
    c = kmalloc(50);
    kprintf("Allocated 3 blocks\n");
    
    kfree(c);  // Sista först
    kfree(b);
    kfree(a);
    kprintf("Freed in reverse - should merge\n");
    print_heap();
    
    // Test 3: Fragmentering sedan merge
    kprintf("\n--- Test 3: Fragmentation then merge ---\n");
    char* blocks[10];
    for (int i = 0; i < 10; i++) {
        blocks[i] = kmalloc(32);
    }
    kprintf("Allocated 10 blocks\n");
    print_heap();
    
    // Free varannan
    for (int i = 0; i < 10; i += 2) {
        kfree(blocks[i]);
    }
    kprintf("Freed every other (5 blocks) - should be fragmented\n");
    print_heap();
    
    // Free resten
    for (int i = 1; i < 10; i += 2) {
        kfree(blocks[i]);
    }
    kprintf("Freed remaining - should be ONE block again!\n");
    print_heap();
    
    // Test 4: Middle block merge
    kprintf("\n--- Test 4: Middle merge ---\n");
    a = kmalloc(100);
    b = kmalloc(100);
    c = kmalloc(100);
    char* d = kmalloc(100);
    char* e = kmalloc(100);
    
    kfree(b);  // Free middle
    kfree(d);  // Free another middle
    kprintf("Freed 2 middle blocks - fragmented\n");
    print_heap();
    
    kfree(c);  // Should merge with b
    kprintf("Freed block between them - should merge b+c\n");
    print_heap();
    
    kfree(a);
    kfree(e);
    kprintf("Freed all - should be one big block and processes remaining\n");
    print_heap();
    
    // Test 5: Reuse after merge
    kprintf("\n--- Test 5: Reuse merged space ---\n");
    a = kmalloc(500);
    kprintf("Allocated 500 bytes from merged heap and then free\n");
    kfree(a);
    print_heap();
    
    kprintf("\n========== MERGE TEST COMPLETE ==========\n");

    

}




void print_heap() {
    kprintf("\n");
    struct heap* current = start_kernel_heap_memory;
    struct heap* next = (struct heap*)((char*)current + get_true_size(current->size));
    while (current < kernel_heap_end) {

        kprintf("We are at address: [%x] with state: [%d] and the size is: [%d] \n", current, get_flag(current->size), get_true_size(current->size));
        current = next;
        next = (struct heap*)((char*)current + get_true_size(current->size));
        
    }
    kprintf("\n");
}
