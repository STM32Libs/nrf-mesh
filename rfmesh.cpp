

#include "rfmesh.h"
#include "crc.h"
#include "protocol.h"
#include "utils.h"

//DigitalOut debug_rf(PB_13);
#define DEBUG_CRC_FAIL 0

#define NRF_NUM (1)

static uint32_t nrf_handlers[NRF_NUM] = {0};

static void nrf_donothing(uint8_t *data,uint8_t size) {};

//forward declaration
void rf_message_handler(uint8_t *data);
void rf_peer2peer_handler(uint8_t *data);
void rf_bridge_handler(uint8_t *data);

uint8_t p2p_message[32];//size included
uint8_t brc_message[32];//size included

uint8_t isReturned;//to check if a ack is returned
bool p2p_ack = false;
uint8_t p2p_expected_Pid = 0;
uint8_t g_nodeId = 0;
uint8_t p2p_nb_retries = 3;
uint16_t p2p_ack_delay = 290;

void nrf_irq()
{
    //on multi instance should derive the id from the irq
    RfMesh *handler = (RfMesh*)nrf_handlers[0];


    //handler->pser->printf("into irq   : irq %d, ce %d\r",handler->nRFIrq.read(), handler->nrf.ce_pin.read());
    
    //--------------------------------------------------------------------------------------------------
    //------------- see note c from datasheet on handling RX_DR IRQ on steps 1) 2) 3) ------------------
    //--------------------------------------------------------------------------------------------------
    //decide here which irq to call
    //handler->nrf.print_status();
    uint8_t status = handler->nrf.readStatus();
    if(status & nrf::bit::status::RX_DR)
    {
        uint8_t rx_pipe_nb = (status & nrf::bit::status::RX_P_NO)>>1;
        uint8_t max_reread = 100;//just to avoid infinite loop, but with such a number loss is already likely
        while((rx_pipe_nb != 0x07) && (--max_reread!=0))//while RX_FIFO Not Empty
        {
            uint8_t data[32];
            #ifdef NRF_DYNAMIC_PAYLOAD
            uint8_t rf_size = getRxPayloadWidth();
            if(rf_size > 32)
            {
                handler->nrf.flushRX();
            }
            #else
            uint8_t rf_size = 32;
            #endif
            handler->nrf.readBuffer(nrf::cmd::R_RX_PLOAD,data,rf_size);             // 1) - read payload through SPI
            //handler->pser->printf("buffer read : irq %d, ce %d\r",handler->nRFIrq.read(), handler->nrf.ce_pin.read());
            handler->nrf.setbit(nrf::reg::STATUS, nrf::bit::status::RX_DR );        // 2) - clear RX_DR IRQ : this should set the IRQ pin back up
            //handler->pser->printf("RX_RD cleared : irq %d, ce %d\r",handler->nRFIrq.read(), handler->nrf.ce_pin.read());
            rf_message_handler(data);
            //reread the status to check if you need to get another buffer
            //handler->nrf.print_status();
            status = handler->nrf.readStatus();
            rx_pipe_nb = (status & nrf::bit::status::RX_P_NO)>>1;                   // 3) - read FIFO status, if more data repeat step 1)
        }
        if(max_reread == 0)
        {
            handler->pser->printf("Error>max_reread\r");
            //do something as we might have been stuck or Under DoS Attack
        }
    }
    else
    {
        //handler->pser->printf("--> Other interrupt Not RX\r");
        // Clear any pending interrupts
        handler->nrf.writeRegister(nrf::reg::STATUS,    nrf::bit::status::MAX_RT | nrf::bit::status::TX_DS | nrf::bit::status::RX_DR );
    }

    //handler->pser->printf("out of irq : irq %d, ce %d\r",handler->nRFIrq.read(), handler->nrf.ce_pin.read());
}

void rf_message_handler(uint8_t *data)
{
    RfMesh *handler = (RfMesh*)nrf_handlers[0];
    //----------------------- sniffing -----------------------------
    handler->_callbacks[static_cast<int>(RfMesh::CallbackType::Sniff)](data,32);
    //--------------- size check -------------------
    if(data[0] > 32)//failure, crc should not be checked is unsafe
    {
        #if (DEBUG_SIZE_FAIL == 1)
            handler->pser->printf("rx size Fail:%d\r",data[0]);
        #endif
        return;
    }
    //--------------- crc check -------------------
    if(!crc::check(data))
    {
        #if (DEBUG_CRC_FAIL == 1)
            handler->pser->printf("rx crc Fail:");
            uint8_t print_size = data[rf::ind::size]+2;
            if(print_size>30)
            {
                print_size = 30;
            }
            print_tab(handler->pser,data,print_size);
            handler->pser->printf("\r\n");
        #endif
        return;
    }
    if(handler->isBridge)
    {
        rf_bridge_handler(data);
    }
    if((data[rf::ind::control] & 0x80) == rf::ctr::Broadcast)
    {
        //we catched a broadcast, forward it to the user as such
        uint8_t user_size = data[rf::ind::size];
        handler->_callbacks[static_cast<int>(RfMesh::CallbackType::Broadcast)](data,user_size);
    }
    else
    {
        rf_peer2peer_handler(data);
    }
}

//at this point, the size and crc are already verified
void rf_bridge_handler(uint8_t *data)
{
    RfMesh *handler = (RfMesh*)nrf_handlers[0];
    uint8_t time_to_live = data[rf::ind::control] & rf::ctr::ttl_mask;
    if(time_to_live != 0)
    {
        time_to_live--;
        data[rf::ind::control] = (data[rf::ind::control] & rf::ctr::ttl_clear) + time_to_live;
        crc::set(data);
        handler->nrf.transmit_Rx(data,data[rf::ind::size]+2);
    }
    //else drop message
}

void rf_peer2peer_handler(uint8_t *data)
{
    RfMesh *handler = (RfMesh*)nrf_handlers[0];

    //handler->pser->printf("p2p\r\n");

    if(data[rf::ind::dest] != g_nodeId)
    {
        //handler->pser->printf("not this node id\r");
        return;//as this packet is not directed to us
    }
    if((data[rf::ind::control] & 0x40) == rf::ctr::Msg_Ack)
    {
        if((data[rf::ind::control] & 0x20) == rf::ctr::Message)
        {
            handler->_callbacks[static_cast<int>(RfMesh::CallbackType::Message)](data,data[rf::ind::size]);
            if((data[rf::ind::control] & 0x10) == rf::ctr::Send_Ack )
            {
                handler->send_ack(data);
            }
        }
        else//it's an acknowledge
        {
            #if(DEBUG_CRC == 1)
            handler->pser->printf("is returned\r\n");
            #endif
            isReturned = 1;
        }
    }
    else//it's Request / Response
    {
        if((data[rf::ind::control] & 0x20) == rf::ctr::Request)
        {
            //handler->pser->printf("call request\r");
            handler->_callbacks[static_cast<int>(RfMesh::CallbackType::Request)](data,data[rf::ind::size]);
        }
        else
        {
            //handler->pser->printf("call response\r");
            isReturned = 1;
            handler->_callbacks[static_cast<int>(RfMesh::CallbackType::Response)](data,data[rf::ind::size]);
        }
    }
    if(isReturned)
    {
        if((p2p_expected_Pid == data[rf::ind::pid]) && (data[rf::ind::dest] == g_nodeId)  )
        {
            //handler->pser->printf("p2p_ack :ok\r");
            p2p_ack = true;//notify the re-transmitter
        }
    }
}


RfMesh::RfMesh(Serial *ps,uint8_t spi_mod,PinName ce, PinName csn, PinName sck, PinName mosi, PinName miso,PinName irq):
                            //1:Gnd, 2:3.3v, 3:ce, 4:csn, 5:sck, 6:mosi, 7:miso, 8:irq
                            nrf(ps,spi_mod, ce, csn, sck, mosi, miso),
                            pser(ps),
                            nRFIrq(irq)
{
    for (size_t i = 0; i < sizeof _callbacks / sizeof _callbacks[0]; i++) 
    {
        _callbacks[i] = nrf_donothing;
    }
    
    nrf_handlers[0] = (uint32_t)this;

    //current implementation with a single instance
}

void RfMesh::init(uint8_t chan)
{
    wait_ms(100);//Let the Power get stable

    //pser->printf("Hello Mesh .... nRF24L01+ Dump :\r\n");
    //print_nrf();

    //pser->printf("Configuration\r\n");
    
    //disable all interrupts - no CRC
    nrf.writeRegister(nrf::reg::CONFIG, nrf::bit::config::MASK_RX_DR | 
                                        nrf::bit::config::MASK_TX_DS | 
                                        nrf::bit::config::MASK_MAX_RT );
    
    nrf.setMode(nrf::Mode::PowerDown);//Power Down

    //Flush any previously used RX and TX FIFOs
    //pser->printf("Flushing Buffers\r\n");
    nrf.command(nrf::cmd::FLUSH_TX);
    nrf.command(nrf::cmd::FLUSH_RX);
    //clear all pending interrupts
    nrf.writeRegister(nrf::reg::STATUS,    nrf::bit::status::MAX_RT | nrf::bit::status::TX_DS | nrf::bit::status::RX_DR );

    nrf.disableAutoAcknowledge();
    nrf.disableRetransmission();

    nrf.enableRxPipes(nrf::bit::en_rxadd::ERX_P0);
    nrf.setPipeWidth(0,32);


    //pser->printf("Power Up\r\n");
    nrf.setMode(nrf::Mode::Standby);//PowerUp


    //pser->printf("set_DataRate(2Mbps)\r\n");
    nrf.setDataRate(nrf::datarate::d_2Mbps);
    //pser->printf("set_CrcConfig(NoCrc)\r\n");
    nrf.setCrcConfig(nrf::crc::NoCrc);
    //pser->printf("set_RF_Channel(2) 2402 MHz\r\n");
    nrf.selectChannel(chan);

    //configure the pio irq
    nRFIrq.fall(&nrf_irq);//can set : callback(this, &Counter::increment)
    nRFIrq.mode(PullNone);
    NVIC_SetPriority(EXTI4_IRQn,10);//10 as relative low

    nRFIrq.enable_irq();

    //enable Rx DR interrupt
    nrf.clearbit(nrf::reg::CONFIG,nrf::bit::config::MASK_RX_DR);

    //pser->printf("Start listening\r\n");
    nrf.setMode(nrf::Mode::Rx);

    isBridge = false;
    
}

void RfMesh::setBridgeMode()
{
    isBridge = true;
}

void RfMesh::attach(Callback<void(uint8_t *data,uint8_t size)> func,RfMesh::CallbackType type)
{
    _callbacks[static_cast<int>(type)] = func;
}

void RfMesh::print_nrf()
{
    nrf.dump_regs();
    nrf.print_info();
    pser->printf("irq pin :%d  ; ce pin  :%d\r",nRFIrq.read(),nrf.ce_pin.read());
}

void RfMesh::setNodeId(uint8_t nid)
{
    g_nodeId = nid;
}

void RfMesh::setRetries(uint8_t nb_retries)
{
    p2p_nb_retries = nb_retries;
}

void RfMesh::setAckDelay(uint16_t delay)
{
    p2p_ack_delay = delay;
}

//can only be called from main due to wait_ms()
bool RfMesh::send_check_ack()
{
    p2p_ack = false;
    p2p_expected_Pid = p2p_message[rf::ind::pid];//Pid
    nrf.transmit_Rx(p2p_message,p2p_message[rf::ind::size]+2);
    #if(DEBUG_CRC == 1)
    pser->printf("sent:(%u):",p2p_message[0]+2);
    print_tab(pser,p2p_message,p2p_message[0]+2);
    pser->printf("\r\n");
    #endif
    //if we want the receiver to send back an Acknowledge
    if((p2p_message[rf::ind::control] & 0x10) == rf::ctr::Send_Ack)
    {
        wait_ms(p2p_ack_delay);// >>> Timeout important, might depend on Nb briges, and on the ReqResp or just MsgAck
    }
    //pser->printf("p2p_ack :%d\r",p2p_ack);
    return p2p_ack;
}

//can only be called from main due to wait_ms() in send_check_ack()
uint8_t RfMesh::send_retries()
{
	bool success = false;
	uint8_t retries = 0;
	do
	{
		success = send_check_ack();//1 on success, 0 on fail
		retries++;
	}while((retries<p2p_nb_retries) && (!success) );
    if(!success)
    {
        retries = 0;
    }
	return retries;//nb_retries in case of success otherwise 0
}

void RfMesh::send_ack(uint8_t *data)
{
    //Ack           : Size Pid  SrcId DstId CRC
    p2p_message[rf::ind::size]      = 5;
    p2p_message[rf::ind::control]   = rf::ctr::Peer2Peer | rf::ctr::Msg_Ack | rf::ctr::Acknowledge;
    p2p_message[rf::ind::pid]       = data[rf::ind::pid];
    p2p_message[rf::ind::source]    = g_nodeId;
    p2p_message[rf::ind::dest]      = data[rf::ind::source];
    crc::set(p2p_message);
	nrf.transmit_Rx(p2p_message,p2p_message[rf::ind::size]+2);
}

//can only be called from main due to wait_ms() in send_check_ack()
uint8_t RfMesh::send_msg(uint8_t* buf)
{
    uint8_t res = false;
    p2p_message[rf::ind::size] = buf[0];
    uint8_t msg_size = p2p_message[rf::ind::size];
    for(int i=1;i<msg_size;i++)
    {
        p2p_message[i] = buf[i];
    }
    crc::set(p2p_message);
    
    if(p2p_message[rf::ind::pid] & rf::ctr::Broadcast)
    {
        nrf.transmit_Rx(p2p_message,p2p_message[rf::ind::size]+2);
        res = true;
    }
    else//directed
    {
        res = send_retries();
    }
    
    return res;
}

//can only be called from main due to wait_ms() in send_check_ack()
uint8_t RfMesh::send_rgb(uint8_t dest,uint8_t r,uint8_t g,uint8_t b)
{
    p2p_message[rf::ind::size]  = 8;
    p2p_message[rf::ind::pid]   = rf::ctr::Peer2Peer | rf::ctr::Msg_Ack | rf::ctr::Message | rf::ctr::Send_Ack;
    p2p_message[rf::ind::pid]   =  rf::pid::rgb;
    p2p_message[rf::ind::source]= g_nodeId;
    p2p_message[rf::ind::dest]  = dest;
    p2p_message[5] = r;
    p2p_message[6] = g;
    p2p_message[7] = b;
    crc::set(p2p_message);
    //print_tab(pser,p2p_message,9);
    return send_retries();
}

//can only be called from main due to wait_ms() in send_check_ack()
uint8_t RfMesh::send_heat(uint8_t dest,uint8_t val)
{
    p2p_message[rf::ind::size]      = 6;
    p2p_message[rf::ind::control]   = rf::ctr::Peer2Peer | rf::ctr::Msg_Ack | rf::ctr::Message | rf::ctr::Send_Ack;
    p2p_message[rf::ind::pid]       = rf::pid::heat;
    p2p_message[rf::ind::source]    = g_nodeId;
    p2p_message[rf::ind::dest]      = dest;
    p2p_message[5] = val;
    crc::set(p2p_message);
    //print_tab(pser,p2p_message,9);
    return send_retries();
}

void RfMesh::broadcast(uint8_t pid)
{
    brc_message[rf::ind::size]      = 4;
    brc_message[rf::ind::control]   = rf::ctr::Broadcast;
    brc_message[rf::ind::pid]       = pid;
    brc_message[rf::ind::source]    = g_nodeId;
    crc::set(brc_message);
    nrf.transmit_Rx(brc_message,brc_message[rf::ind::size]+2);
}

void RfMesh::broadcast_heat(uint8_t heat)
{
    brc_message[rf::ind::size]      = 5;
    brc_message[rf::ind::control]   = rf::ctr::Broadcast;
    brc_message[rf::ind::pid]       = rf::pid::heat;
    brc_message[rf::ind::source]    = g_nodeId;
    brc_message[4] = heat;
    crc::set(brc_message);
    //print_tab(pser,brc_message,5);
    nrf.transmit_Rx(brc_message,brc_message[rf::ind::size]+2);
}

