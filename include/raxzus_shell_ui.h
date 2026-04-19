#ifndef RAXZUS_SHELL_UI_H
#define RAXZUS_SHELL_UI_H

#define ALIGN_LEFT 1
#define ALIGN_RIGHT 2
#define ALIGN_CENTER 3


int checkString(char* input, char* compareString);
void cmd_help(void);
void cmd_clear(void);
void cmd_neofetch(void);  // The OS logo + info!
void boot_intro(void);



// Help ui functions.

// ═══════════════════════════════════════════════════════════════
//                    RAXZUS SHELL UI LIBRARY
// ═══════════════════════════════════════════════════════════════
// Box Drawing and Text Alignment Utilities
// 
// Example Usage:
//   draw_box("Hello", 40, 6, CENTER, COLOR_CYAN, 0);
//   print_text_align("Hello", 40, CENTER, COLOR_WHITE, COLOR_BLUE, 0);
//
// Alignment: LEFT, RIGHT, CENTER
// Colors: Use COLOR_* defines from vga.h
// delay_ms: 0 = instant, >0 = crawl effect per character
// ═══════════════════════════════════════════════════════════════



// To write a color use COLOR define from vga.h library
void draw_box_top(int width, int wall_color, int text_color, char* title);                                    //   ╔════════════════════╗
void draw_box_sides(int width, int num_lines, int color);                                                     //   ║                    ║
void print_text_align(char* text, int width, int align, int wall_color, int text_color, int text_delay_ms);   //   ║        TEXT        ║   1: left aligned, 2: right aligned, 3: center
void draw_box_bottom(int width, int color);                                                                   //   ╚════════════════════╝
void draw_box(char* text, int width, int align, int wall_color, int text_color, int text_delay_ms);

// Helper string functions
void string_copy(char* original_text, char* text_to_copy);
void string_crawl(char* text, int delay, int color);
int string_length(char* text);


#endif