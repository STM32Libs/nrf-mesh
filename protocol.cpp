/** @file rf_protocol.c
 *
 * @author Wassim FILALI  STM8L
 *
 * @compiler GCC for STM32
 *
 *
 * $Date: 29.10.2016 - creation
 * $Date: 08.10.2017 - take over to STM32 with mbed
 * $Revision: 2
 *
*/

#include "protocol.h"


Proto::Proto(Serial *ps):pser(ps)
{

}

//TODO can bring the fill here as well

void Proto::print_light_rgb(uint8_t *rxPayload)
{
	uint16_t light,R,G,B;
	light = rxPayload[0];
	light = (light<<8) + rxPayload[1];
	R = rxPayload[0];
	R = (R<<8) + rxPayload[1];
	G = rxPayload[2];
	G = (G<<8) + rxPayload[3];
	B = rxPayload[4];
	B = (B<<8) + rxPayload[5];
	pser->printf("light:%u;red:%u,green:%u;blue:%u\r\n",light,R,G,B);
}


//light[0] msb - light[1] lsb
//[8bits]        [4 unused-4bits]
void Proto::fill_light_paylod(uint16_t light,uint8_t *rxPayload)
{
	rxPayload[0] =   0xFF & (light >> 4);
	rxPayload[1] = 0x000F & (light);
}

void Proto::print_light(uint8_t *rxPayload)
{
	unsigned int SensorVal = rxPayload[0];
	SensorVal <<= 4;//shift to make place for the 4 LSB
	SensorVal = SensorVal + (0x0F & rxPayload[1]);
	pser->printf("light:%u\r",SensorVal);
}

void Proto::print_magnet(uint8_t *rxPayload)
{
	if(rxPayload[0] == 0)
	{
		pser->printf("magnet:Low\r");
	}
	else
	{
		pser->printf("magnet:High\r");
	}
}

void Proto::print_bme280(uint8_t *rxPayload)
{
	pser->printf("bme280: 0x");
	for(int i=0;i<8;i++)
	{
		pser->printf("%02x ",rxPayload[i]);
	}
    pser->printf("\r");
}
