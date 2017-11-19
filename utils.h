
#include "mbed.h"

void print_tab(Serial *ps,uint8_t *data,uint8_t size);

uint8_t get_hex_char(uint8_t c);

uint8_t get_hex(uint8_t* buffer,uint8_t pos);

uint8_t strbegins (uint8_t * s1, const char * s2);

void wait_us_cpu(uint32_t delay_us);
