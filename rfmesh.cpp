

#include "rfmesh.h"

RfMesh::RfMesh(Serial *ps): nrf(PB_15, PB_14, PB_13, PB_12, PA_8, PA_9),    // mosi, miso, sck, csn, ce, irq
                            pser(ps)
{
}

void RfMesh::init()
{
    pser->printf( "Hello Mesh .... Powering Up the nRF\r\n");
    nrf.powerUp();
    print_nrf();
}

void RfMesh::print_nrf()
{
    // Display the (default) setup of the nRF24L01+ chip
    pser->printf( "nRF24L01+ Output power : %d dBm\r\n",    nrf.getRfOutputPower() );
    pser->printf( "nRF24L01+ Frequency    : %d MHz\r\n",    nrf.getRfFrequency() );
    pser->printf( "nRF24L01+ Data Rate    : %d kbps\r\n",   nrf.getAirDataRate() );
    //pser->printf( "nRF24L01+ TX Address   : 0x%010llX\r\n", nrf.getTxAddress() );
    //pser->printf( "nRF24L01+ RX Address   : 0x%010llX\r\n", nrf.getRxAddress() );
    
}
