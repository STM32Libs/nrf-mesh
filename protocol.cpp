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

void Proto::rx_alive(uint8_t src_NodeId)
{
	pser->printf("NodeId:%d;is_Alive\r",src_NodeId);
}

void Proto::rx_reset(uint8_t src_NodeId)
{
	pser->printf("NodeId:%d;was:Reset\r");
}

void Proto::rx_light(uint8_t src_NodeId,uint8_t *rxPayload)
{
	unsigned int SensorVal = rxPayload[0];
	SensorVal <<= 4;//shift to make place for the 4 LSB
	SensorVal = SensorVal + (0x0F & rxPayload[1]);
	pser->printf("NodeId:%d;Light:%u\r",src_NodeId,SensorVal);
}

void Proto::rx_magnet(uint8_t src_NodeId,uint8_t *rxPayload)
{
	pser->printf("NodeId:%d;Magnet:",src_NodeId);
	if(rxPayload[0] == 0)
	{
		pser->printf("Low\r");
	}
	else
	{
		pser->printf("High\r");
	}
}

void Proto::bme280_rx_measures(uint8_t src_NodeId,uint8_t *rxPayload)
{
	pser->printf("NodeId:%d;BME280: 0x",src_NodeId);
	for(int i=0;i<8;i++)
	{
		pser->printf("%02x ",rxPayload[i]);
	}
    pser->printf("\r");
}
