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

void Proto::print_light_uint32(uint8_t *data)
{
	uint32_t light  = data[3] << 24;
			 light |= data[2] << 16;
			 light |= data[1] <<  8;
			 light |= data[0];
	pser->printf("light_u:%u\r",light);
}

void Proto::print_new_light(uint8_t *rxPayload)
{
	unsigned int SensorVal = rxPayload[1];
	SensorVal <<= 8;
	SensorVal = SensorVal + rxPayload[0];
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

void Proto::print_battery(uint8_t *rxPayload)
{
	uint16_t 	bat_val = rxPayload[0] << 8;
				bat_val |= rxPayload[1];
	uint16_t msv = bat_val / 1000;
	uint16_t lsv = bat_val % 1000;
	pser->printf("battery:%u.%03u\r",msv,lsv);
}

void Proto::print_bme280_values(uint8_t nodeid,uint8_t *data)
{
	int32_t temp  = data[0] << 24;
			temp |= data[1] << 16;
			temp |= data[2] <<  8;
			temp |= data[3];
	pser->printf("temperatureX100:%d\r",temp);
	uint32_t hum  = data[4] << 24;
			 hum |= data[5] << 16;
			 hum |= data[6] <<  8;
			 hum |= data[7];
	uint32_t msh = hum>>10;
	uint32_t lsh = hum & 0x3FF;
	pser->printf("NodeId:%u;humidity:%u.%03u\r",nodeid,msh,lsh);
	uint32_t press  = data[8]  << 24;
			 press |= data[9]  << 16;
			 press |= data[10] <<  8;
			 press |= data[11];
	uint32_t msp = press>>8;
	uint32_t lsp = hum & 0xFF;
	pser->printf("NodeId:%u;pressure:%u.%u\r",nodeid,msp,lsp);
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

int32_t Proto::get_int32(uint8_t *rxPayload)
{
	int32_t val;
	uint8_t *p_val = (uint8_t*)&val;
	p_val[0] = rxPayload[3];
	p_val[1] = rxPayload[2];
	p_val[2] = rxPayload[1];
	p_val[3] = rxPayload[0];
	return val;
}

uint32_t Proto::get_uint32(uint8_t *rxPayload)
{
	uint32_t val;
	uint8_t *p_val = (uint8_t*)&val;
	p_val[0] = rxPayload[3];
	p_val[1] = rxPayload[2];
	p_val[2] = rxPayload[1];
	p_val[3] = rxPayload[0];
	return val;
}


int32_t int_accell(uint8_t *data)
{
	int16_t val;
	uint8_t* p_val = (uint8_t*)&val;
	p_val[0] = data[1];
	p_val[1] = data[0];
	int32_t b_val = val * 100;
	b_val /=16384;
	return b_val;
}

void Proto::print_acceleration(uint8_t *rxPayload)
{
	pser->printf("acc_x:%d;",int_accell(rxPayload));
	rxPayload+=2;
	pser->printf("acc_y:%d;",int_accell(rxPayload));
	rxPayload+=2;
	pser->printf("acc_z:%d;\r",int_accell(rxPayload));
}
