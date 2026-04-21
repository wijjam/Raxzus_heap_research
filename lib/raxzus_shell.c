#include "../include/raxzus_shell.h"
#include "../include/raxzus_shell_ui.h"
#include "../include/vga.h"
#include "../include/rtc.h"
#include "../include/memory.h"
#include "../include/process_manager.h"
#include "../include/ata_disk_driver.h"
#include "../include/kutils.h"
#include "../include/heap_domain.h"
#include <stdarg.h>



void process_input(char* input) {

    int parser_count = 0;
    char** argv = tokinize_words(input, &parser_count);

    if (checkString(argv[0], "help")) {
        cmd_help();
    } 
    
    else if (checkString(argv[0], "neofetch")) {
        cmd_neofetch();
    } 
    
    else if (checkString(argv[0], "mem")) {
        //test_kmalloc_kfree();
    }

    else if (checkString(argv[0], "memtest")) {
        if (parser_count < 2) {
            kprintf("Usage: memtest 1  (fragmentation + WCET O(1) timing)\n");
            kprintf("       memtest 2  (large allocation test)\n");
        } else if (checkString(argv[1], "1")) {
            cmd_memtest_v1();
        } else if (checkString(argv[1], "2")) {
            cmd_memtest_v2();
        } else {
            kprintf("Unknown test. Use memtest 1 or memtest 2\n");
        }
    }
    
    else if (checkString(argv[0], "time")) {
        print_rtc_time();
    } 
    
    else if (checkString(argv[0], "clear")) {
        clearScreen();
    } 
    
    else if (checkString(argv[0], "diskW")) {
        
        if (parser_count != 4) {
            kprintf_red("\nThis commande requires 3 flag inputs (LBA, Segment, what to save)\n");
            return;
        }

        if (!is_valid_number(argv[1]) || !is_valid_number(argv[2])) {
            kprintf_red("\nThis commande requires the first inputs to be integers and the last to be a pointer\n");
            return;
        }

        if (string_to_int(argv[1]) > 2000 || string_to_int(argv[1])  < 0) {
            kprintf_red("\nThis commande requires the first input to be in disk size\n");
            return;
        }

        if (string_to_int(argv[2]) < 0 || string_to_int(argv[2]) > 255) {
            kprintf_red("\nThis commande requires the first input to \n");
            return;
        }

        int param2 = string_to_int(argv[2]);
        int param1 = string_to_int(argv[1]);


        char* buffer = argv[3];



        ata_write_sector(param1, param2, buffer);
        kprintf_green("\nSuccessfully wrote to disk at \nLBA: %d, \nsector size: %d, \nthe data: %s \n", param1, param2, buffer);
    } 
    
    else if (checkString(argv[0], "diskR")) {
        if (parser_count != 3) {
            kprintf_red("\nThis commande requires 2 flag inputs (LBA, Segment)\n");
            return;
        }

        if (!is_valid_number(argv[1]) || !is_valid_number(argv[2])) {
            kprintf_red("\nThis commande requires the first inputs to be integers and the last to be a pointer\n");
            return;
        }

        if (string_to_int(argv[1]) > 2000 || string_to_int(argv[1])  < 0) {
            kprintf_red("\nThis commande requires the first input to be in disk size\n");
            return;
        }

        if (string_to_int(argv[2]) < 0 || string_to_int(argv[2]) > 255) {
            kprintf_red("\nThis commande requires the first input to \n");
            return;
        }

        int param2 = string_to_int(argv[2]);
        int param1 = string_to_int(argv[1]);


        char* buffer = kmalloc(param2*512);



        ata_read_sector(49, 10, buffer);
        //uint8_t* byte_buffer = (uint8_t*) buffer;

        for (int i = 0; i < 1024; i++) {
            char c = buffer[i];
            if (c >= 32 && c <= 126) {
                kprintf("%c", buffer[i]);
            }
        }

        //kprintf_green("\nSuccessfully READ from disk at \nLBA: %d, \nsector size: %d, \nthe data: %s \n", param1, param2, buffer);
        
    } 
    
    else {
        kprintf("\n%eCommand not found\n");
    }

    kfree(argv);

}






