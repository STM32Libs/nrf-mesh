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
	pser->printf("NodeId:");
	/*UARTPrintf_uint(src_NodeId);
    printf_ln(";is:Alive");
    */
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
	pser->printf("NodeId:");
	/*UARTPrintf_uint(src_NodeId);
	printf_ln(";was:Reset");*/
}

//Rx 5 Bytes
void Proto::rx_light(uint8_t src_NodeId,uint8_t *rxPayload,uint8_t rx_PayloadSize)
{
	unsigned int SensorVal;
	pser->printf("NodeId:");
	/*printf_uint(src_NodeId);
	printf(";Light:");
	SensorVal = rxPayload[0];
	SensorVal <<= 4;//shift to make place for the 4 LSB
	SensorVal = SensorVal + (0x0F & rxPayload[1]);
	printf_uint(SensorVal);
	printf_eol();*/
}

//Rx 4 Bytes
void Proto::rx_magnet(uint8_t src_NodeId,uint8_t *rxPayload,uint8_t rx_PayloadSize)
{
	pser->printf("NodeId:");
	/*printf_uint(src_NodeId);
	printf(";Magnet:");
	if(rxPayload[0] == 0)
	{
		printf_ln("Low");
	}
	else
	{
		printf_ln("High");
	}*/
}
