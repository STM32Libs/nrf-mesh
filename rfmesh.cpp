

#include "rfmesh.h"

#define NRF_NUM (1)

static uint32_t nrf_handlers[NRF_NUM] = {0};

static void nrf_donothing() {};

void nrf_irq()
{
    //on multi instance should derive the id from the irq
    RfMesh *handler = (RfMesh*)nrf_handlers[0];
    //decide here which irq to call
    handler->pser->printf("from nrf IRQ\n");
    handler->_callbacks[static_cast<int>(RfMesh::CallbackType::Message)]();
    // Clear any pending interrupts
    handler->nrf.writeRegister(nrf::reg::STATUS,    nrf::bit::STATUS_MAX_RT | nrf::bit::STATUS_TX_DS | nrf::bit::STATUS_RX_DR );

}

RfMesh::RfMesh(Serial *ps,PinName ce, PinName csn, PinName sck, PinName mosi, PinName miso,PinName irq):
                            nrf(ce, csn, sck, mosi, miso),    //1:Gnd, 2:3.3v, 3:ce, 4:csn, 5:sck, 6:mosi, 7:miso, 8:irq 
                            pser(ps),
                            nRFIrq(irq)
{
    for (size_t i = 0; i < sizeof _callbacks / sizeof _callbacks[0]; i++) 
    {
        _callbacks[i] = nrf_donothing;
    }
    
    nRFIrq.fall(&nrf_irq);//can set : callback(this, &Counter::increment)
    nRFIrq.mode(PullNone);
    nRFIrq.enable_irq();
    //current implementation with a single instance
    nrf_handlers[0] = (uint32_t)this;
}

void RfMesh::init()
{
    pser->printf( "Hello Mesh .... Powering Up the nRF\r\n");
    nrf.setMode(nrf::Mode::Standby);//PowerUp


    pser->printf("set_DataRate()\r\n");
    nrf.set_DataRate(nrf::datarate::d_2Mbps);
    pser->printf("set_CrcConfig()\r\n");
    nrf.set_CrcConfig(nrf::crc::NoCrc);
    /*pser->printf("setTxAddress()\r\n");
    nrf.setTxAddress(DEFAULT_NRF24L01P_ADDRESS,5);
    pser->printf("setRxAddress()\r\n");
    nrf.setRxAddress(DEFAULT_NRF24L01P_ADDRESS,5);
    pser->printf("setCrcWidth()\r\n");
    */

    nrf.setbit(nrf::reg::STATUS,nrf::bit::STATUS_RX_DR);//write one to clear status bit
    nrf.clearbit(nrf::reg::CONFIG,nrf::bit::CONFIG_MASK_RX_DR);//enable Rx DR interrupt

    nrf.setMode(nrf::Mode::Rx);
    nrf.ce_pin_highEnable();
    
    print_nrf();
}

void RfMesh::nrf_print_status()
{
    //nrf.print_Status(pser->printf);
    //int status = nrf.readStatus();
    //pser->printf("status:0x%x - ",status);
    nrf.print_info();
    int config = nrf.readRegister(nrf::reg::CONFIG);
    pser->printf("config:0x%x - ",config);
    int irq_status = nRFIrq.read();
    pser->printf("irq pin %d\n",irq_status);
}


void RfMesh::print_nrf()
{
    // Display the (default) setup of the nRF24L01+ chip
    /*pser->printf( "nRF24L01+ Output power : %d dBm\r\n",    nrf.getRfOutputPower() );
    pser->printf( "nRF24L01+ Frequency    : %d MHz\r\n",    nrf.getRfFrequency() );
    pser->printf( "nRF24L01+ Data Rate    : %d kbps\r\n",   nrf.getAirDataRate() );
    pser->printf( "nRF24L01+ TX Address   : 0x%010llX\r\n", nrf.getTxAddress() );
    pser->printf( "nRF24L01+ RX Address   : 0x%010llX\r\n", nrf.getRxAddress() );
    */
    
    uint8_t rx_lsb = nrf.readRegister(nrf::reg::RX_ADDR_P0);
    pser->printf( "nRF24L01+ RX Address LSB: 0x%x\r\n", rx_lsb );
}

void RfMesh::attach(Callback<void()> func,RfMesh::CallbackType type)
{
    _callbacks[static_cast<int>(type)] = func;
}
