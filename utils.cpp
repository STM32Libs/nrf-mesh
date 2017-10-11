
#include "utils.h"

void print_tab(Serial *ps,uint8_t *data,uint8_t size)
{
    ps->printf("0x");
    for(int i=0;i<size;i++)
    {
        ps->printf("%02X ",data[i]);
    }
    ps->printf("\r");
}

uint8_t get_hex_char(uint8_t c)
{
	uint8_t res = 0;
	if(c <= '9')
	{
		res = c - '0';
	}
	else if(c <= 'F')
	{
		res = c - 'A' + 10;
	}
	else if(c <= 'f')
	{
		res = c - 'a' + 10;
	}
	return res;
}

uint8_t get_hex(uint8_t* buffer,uint8_t pos)
{
	uint8_t hex_val;
	pos+=2;//skip "0x"
	hex_val = get_hex_char(buffer[pos++]);
	hex_val <<= 4;
	hex_val |= get_hex_char(buffer[pos]);
	return hex_val;
}

uint8_t strbegins (uint8_t * s1, const char * s2)
{
    for(; *s1 == *s2; ++s1, ++s2)
        if(*s2 == 0)
            return 0;
    return (*s2 == 0)?0:1;
}
