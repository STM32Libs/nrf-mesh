

#include "rfmesh.h"

#define NRF_NUM (1)

static uint32_t nrf_handlers[NRF_NUM] = {0};

static void nrf_donothing() {};

#define _NRF24L01P_REG_STATUS                0x07

#define _NRF24L01P_STATUS_MAX_RT         (1<<4)
#define _NRF24L01P_STATUS_TX_DS          (1<<5)
#define _NRF24L01P_STATUS_RX_DR          (1<<6)


void nrf_irq()
{
    //on multi instance should derive the id from the irq
    RfMesh *handler = (RfMesh*)nrf_handlers[0];
    //decide here which irq to call
    handler->pser->printf("from nrf IRQ\n");
    handler->_callbacks[RfMesh::Message]();
    // Clear any pending interrupts
    handler->nrf.setRegister(_NRF24L01P_REG_STATUS, _NRF24L01P_STATUS_MAX_RT|_NRF24L01P_STATUS_TX_DS|_NRF24L01P_STATUS_RX_DR);

}

RfMesh::RfMesh(Serial *ps,PinName irq):
                            nrf(PA_7, PA_6, PA_5, PA_4, PC_15),    // mosi, miso, sck, csn, ce, irq not used here
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
    nrf.powerUp();

    pser->printf("setAirDataRate()\r\n");
    nrf.setAirDataRate(NRF24L01P_DATARATE_2_MBPS);
    pser->printf("setTxAddress()\r\n");
    nrf.setTxAddress(DEFAULT_NRF24L01P_ADDRESS,5);
    pser->printf("setRxAddress()\r\n");
    nrf.setRxAddress(DEFAULT_NRF24L01P_ADDRESS,5);
    pser->printf("setCrcWidth()\r\n");
    nrf.setCrcWidth(NRF24L01P_CRC_NONE);
    nrf.setTransferSize( 32 );

    enable_nrf_rx_interrupt();

    nrf.setReceiveMode();
    nrf.enable();
    
    print_nrf();
}

#define _NRF24L01P_REG_CONFIG           0x00
#define _NRF24L01P_CONFIG_MASK_RX_DR    (1<<6)
#define _NRF24L01P_REG_RX_ADD_P0        0x0A

void RfMesh::enable_nrf_rx_interrupt()
{
    int config = nrf.getRegister(_NRF24L01P_REG_CONFIG);
    config &= ~_NRF24L01P_CONFIG_MASK_RX_DR;
    nrf.setRegister(_NRF24L01P_REG_CONFIG, config);

}

void RfMesh::print_nrf()
{
    // Display the (default) setup of the nRF24L01+ chip
    pser->printf( "nRF24L01+ Output power : %d dBm\r\n",    nrf.getRfOutputPower() );
    pser->printf( "nRF24L01+ Frequency    : %d MHz\r\n",    nrf.getRfFrequency() );
    pser->printf( "nRF24L01+ Data Rate    : %d kbps\r\n",   nrf.getAirDataRate() );
    pser->printf( "nRF24L01+ TX Address   : 0x%010llX\r\n", nrf.getTxAddress() );
    pser->printf( "nRF24L01+ RX Address   : 0x%010llX\r\n", nrf.getRxAddress() );
    
    int rx_lsb = nrf.getRegister(_NRF24L01P_REG_RX_ADD_P0);
    pser->printf( "nRF24L01+ RX Address LSB: 0x%x\r\n", rx_lsb );
}

void RfMesh::attach(Callback<void()> func,CallbackType type)
{
    _callbacks[type] = func;
}
