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

void Proto::rf_get_tx_alive_3B(uint8_t NodeId, uint8_t* tx_data)
{
      tx_data[0]= rf_pid_0xF5_alive;
      tx_data[1]= NodeId;
      tx_data[2]= tx_data[0] ^ NodeId;
}

//Rx 3 Bytes
void Proto::rx_alive(uint8_t src_NodeId)
{
	pser->printf("NodeId:%d;is_Alive\r",src_NodeId);
}

// Reset
void Proto::rf_get_tx_reset_3B(uint8_t NodeId, uint8_t* tx_data)
{
      tx_data[0]= rf_pid_0xC9_reset;
      tx_data[1]= NodeId;
      tx_data[2]= tx_data[0] ^ NodeId;
}

void Proto::rx_reset(uint8_t src_NodeId)
{
	pser->printf("NodeId:%d;was:Reset\r");
}

//Rx 5 Bytes
void Proto::rx_light(uint8_t src_NodeId,uint8_t *rxPayload)
{
	unsigned int SensorVal = rxPayload[0];
	SensorVal <<= 4;//shift to make place for the 4 LSB
	SensorVal = SensorVal + (0x0F & rxPayload[1]);
	pser->printf("NodeId:%d;Light:%u\r",src_NodeId,SensorVal);
}

//Rx 4 Bytes
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
