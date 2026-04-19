#include "../include/raxzus_shell_ui.h"
#include "../include/vga.h"


int checkString(char* input, char* compareString) {
    
    int i = 0;
    while (input[i] != '\0' && compareString[i] != '\0') {
        if (input[i] != compareString[i]) {
            return 0;
        }
        i++;
    }

    if (input[i] != '\0' && compareString[i] == '\0') {
        return 0;
    }

    if (input[i] == '\0' && compareString[i] != '\0') {
        return 0;
    }

    return 1;
}


// ==================================== COMMANDS ===========================================


void cmd_help(void) {

    kprintf("\nWelcome to raxzus shell help screen. The following commands are currently available: \n");

    kprintf("1. help (");
    kprintf_magenta("Command you called to print this list");
    kprintf(")\n");

    kprintf("2. neofetch (");
    kprintf_magenta("used for kernel special ascii art");
    kprintf(")\n");

    kprintf("3. calculate (");
    kprintf_magenta("To be implemented so you can do simple calculations");
    kprintf(")\n");

    kprintf("4. time (");
    kprintf_magenta("Shows the current time");
    kprintf(")\n");


}

static const char* RaxzusDragon[] = {
    "              /-------\\ ",
    " /-\\        /  (*)  \\",
    "| ) )-------- _         \\",
    "|        ____/_/\\_____ /",
    " \\/----/____/         | ",
    " /\\/---/           ___|",
    " |          _______/ ",
    "  \\_______/",
    0
};


void cmd_neofetch(void) {
    kprintf("\n");
    int i = 0;
    while (RaxzusDragon[i] != 0) {
        kprintf_green("%s", RaxzusDragon[i]);

        if (i == 2) kprintf_light_gray("   OS: RaxzusOS v1.0");
        if (i == 3) kprintf_light_gray("   Kernel: Raxzus_kernel");
        if (i == 4) kprintf_light_gray("   Shell: RaxShell");
        if (i == 5) kprintf_light_gray("   Uptime: Too powerful to measure");
        kprintf("\n");
        i++;
    }
    kprintf("\n");

}

void boot_intro(void) {




}


void draw_box_top(int width, int wall_color, int text_color, char* title){

    kprintf("\n");

    int text_size = string_length(title);
    width = width - text_size;

    set_color(wall_color);
    kprintf("%c", 0xC9); // '╔'
    for (int i = 0; i < width-1; i++) {

        if (i == width/2)  {
            set_color(text_color);
            kprintf("%s", title);
        } else {
            set_color(wall_color);
            kprintf("%c", 0xCD); // '═'
        }
    }
    set_color(wall_color);
    kprintf("%c\n", 0xBB); // '╗'

}

void draw_box_sides(int width, int num_lines, int color) {
    
    for (int i = 0; i < num_lines/3; i++) {
        for (int ii = 0; ii < width; ii++) {
            if (ii == 0 || ii == width-1) {
                set_color(color);
                kprintf("%c", 0xBA); // '║'
            } else {
                kprintf(" ");
            }
        }
        kprintf("\n");
    }
}

void draw_box_bottom(int width, int color) {
    set_color(color);
    kprintf("%c", 0xC8); // '╚'
    for (int i = 0; i < width-2; i++) {
        set_color(color);
        kprintf("%c", 0xCD); // '═'
    }
    set_color(color);
    kprintf("%c\n", 0xBC); // '╝'

}

void print_text_align(char* text, int width, int align, int wall_color, int text_color, int text_delay_ms) {


    int text_size = string_length(text);
    width = width - text_size;

    

    switch (align) {

        case 1:
            for (int ii = 0; ii <= width; ii++) {
                if (ii == 0 || ii == width) {
                    set_color(wall_color);
                    kprintf("%c", 0xBA); // '║'
                } else if (ii == 3)  {
                    set_color(text_color);
                    kprintf("%s", text);
                } else {
                    kprintf(" ");
                }
            }
            kprintf("\n");
        break;

        case 2:
            for (int ii = 0; ii <= width; ii++) {
                if (ii == 0 || ii == width) {
                    set_color(wall_color);
                    kprintf("%c", 0xBA); // '║'
                } else if (ii == (width - 2))  {
                    set_color(text_color);
                    kprintf("%s", text);
                } else {
                    kprintf(" ");
                }
            }
            kprintf("\n");
        break;

        case 3:
            for (int ii = 0; ii <= width; ii++) {
                if (ii == 0 || ii == width) {
                    set_color(wall_color);
                    kprintf("%c", 0xBA); // '║'
                } else if (ii == width/2)  {
                    set_color(text_color);
                    kprintf("%s", text);
                } else {
                    kprintf(" ");
                }
            }
            kprintf("\n");
        break;


    }

}

void draw_box(char* text, int width, int align, int wall_color, int text_color, int text_delay_ms) {

}


int string_length(char* text) {
    int count = 0;
    
    while (*text != '\0') {
        count = count + 1;
        text = text + 1;
    }

    return count;
}
