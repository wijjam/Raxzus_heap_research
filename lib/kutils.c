#include "../include/kutils.h"
#include "../include/vga.h"

void kmemset(void *ptr, uint32_t value, uint32_t size)
{
    uint8_t *p = (uint8_t *)ptr;
    for (uint32_t i = 0; i < size; i++)
        p[i] = value;
}

char** tokinize_words(char* input, int* parser_word_count) {

        char** argv = kmalloc(200);

    int is_string = 0;
    char* sim_input = input;
    int index = 1;
    argv[0] = sim_input;
    while (*sim_input != '\0') {


        if (*sim_input == '"') {
            if (is_string == 0) {
                is_string = 1;
            } else {
                is_string = 0;
            }
        }


        if (*sim_input == ' ' && is_string == 0) {
            *sim_input = '\0';
            sim_input++;
            argv[index] = sim_input;
            index++;
            continue;
        }
        
        sim_input++;
    }
    *parser_word_count = index;
    return argv;

}

int is_valid_number(char* str) {
    while (*str) {
        if (*str < '0' || *str > '9') {
            return 0;
        }
        str++;
    }
    return 1;
}

int string_to_int(char* input) {


    int result = 0;
    int digit = 0;
    while (*input != '\0') {
        digit = *input - '0';
        result = result * 10 + digit;
        input++;
    }

    return result;
}