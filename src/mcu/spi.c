#include "spi.h"

void write_register(uintptr_t addr, uint32_t value){
    volatile uint32_t *local_address = (volatile uint32_t *)addr;
    *local_address = value;
}