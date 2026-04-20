#ifndef KUTILS_H
#define KUTILS_H
#include <stdint.h>

void kmemset(void *ptr, uint32_t value, uint32_t size);
char** tokinize_words(char* input, int* parser_word_count);
int is_valid_number(char* str);
int string_to_int(char* input);

#endif