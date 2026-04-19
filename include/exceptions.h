#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <stdint.h>


    void divide_by_zero_handler();
    void page_fault_handler(uint32_t* stack);
    void general_protection_fault_handler(uint32_t* stack);
    void double_fault_handler(uint32_t error_code);

#endif