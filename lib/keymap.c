#include "../include/keymap.h"

// Swedish keyboard layout - lowercase/unshifted
static char keymap_lower[128] = {
    0,    27,   '1',  '2',  '3',  '4',  '5',  '6',   // 0x00-0x07
    '7',  '8',  '9',  '0',  '+',  0xB4,  '\b', '\t',  // 0x08-0x0F (+ and ´ instead of - =)
    'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',   // 0x10-0x17 (same as US)
    'o',  'p',  0xE5,  0xA8,  '\n', 0,    'a',  's',   // 0x18-0x1F (å, ¨ instead of [ ])
    'd',  'f',  'g',  'h',  'j',  'k',  'l',  0xF6,   // 0x20-0x27 (ö instead of ;)
    0xE4,  0xA7,  0,    '\'', 'z',  'x',  'c',  'v',   // 0x28-0x2F (ä, § instead of ', `)
    'b',  'n',  'm',  ',',  '.',  '-',  0,    '*',   // 0x30-0x37 (- instead of /)
    0,    ' ',  0,    0,    0,    0,    0,    0,     // 0x38-0x3F
    0,    0,    0,    0,    0,    0,    0,    0,     // 0x40-0x47
    0,    0,    0,    0,    0,    0,    0,    0,     // 0x48-0x4F
    0,    0,    0,    0,    0,    0,    0,    0,     // 0x50-0x57
    0,    0,    0,    0,    0,    0,    0,    0,     // 0x58-0x5F
    0,    0,    0,    0,    0,    0,    0,    0,     // 0x60-0x67
    0,    0,    0,    0,    0,    0,    0,    0,     // 0x68-0x6F
    0,    0,    0,    0,    0,    0,    0,    0,     // 0x70-0x77
    0,    0,    0,    0,    0,    0,    0,    0      // 0x78-0x7F
};

// Swedish keyboard layout - uppercase/shifted
static char keymap_upper[128] = {
    0,    27,   '!',  '"',  '#',  0xA4,  '%',  '&',   // 0x00-0x07 (" instead of @, ¤ instead of $)
    '/',  '(',  ')',  '=',  '?',  '`',  '\b', '\t',  // 0x08-0x0F (/, (, ), = instead of &*()_+)
    'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',   // 0x10-0x17 (same as US)
    'O',  'P',  0xC5,  '^',  '\n', 0,    'A',  'S',   // 0x18-0x1F (Å, ^ instead of { })
    'D',  'F',  'G',  'H',  'J',  'K',  'L',  0xD6,   // 0x20-0x27 (Ö instead of :)
    0xC4,  0xBD,  0,    '*',  'Z',  'X',  'C',  'V',   // 0x28-0x2F (Ä, ½ instead of ", ~)
    'B',  'N',  'M',  ';',  ':',  '_',  0,    '*',   // 0x30-0x37 (; : _ instead of < > ?)
    0,    ' ',  0,    0,    0,    0,    0,    0,     // 0x38-0x3F
    0,    0,    0,    0,    0,    0,    0,    0,     // 0x40-0x47
    0,    0,    0,    0,    0,    0,    0,    0,     // 0x48-0x4F
    0,    0,    0,    0,    0,    0,    0,    0,     // 0x50-0x57
    0,    0,    0,    0,    0,    0,    0,    0,     // 0x58-0x5F
    0,    0,    0,    0,    0,    0,    0,    0,     // 0x60-0x67
    0,    0,    0,    0,    0,    0,    0,    0,     // 0x68-0x6F
    0,    0,    0,    0,    0,    0,    0,    0,     // 0x70-0x77
    0,    0,    0,    0,    0,    0,    0,    0      // 0x78-0x7F
};

char scancode_to_ascii(uint8_t scancode, int shift_pressed) {
    if (scancode >= 128) return 0;  // Invalid scancode
    
    if (shift_pressed) {
        return keymap_upper[scancode];
    } else {
        return keymap_lower[scancode];
    }
}