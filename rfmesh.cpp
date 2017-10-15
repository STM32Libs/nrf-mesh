

#include "rfmesh.h"
#include "crc.h"
#include "protocol.h"
#include "utils.h"

#define NRF_NUM (1)

static uint32_t nrf_handlers[NRF_NUM] = {0};

static void nrf_donothing(uint8_t *data,uint8_t size) {};

//forward declaration
void rf_message_handler(uint8_t *data);
void rf_peer2peer_handler(uint8_t *data);

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
    //--------------------------------------------------------------
    #if P2P_BRIDGE_RETRANSMISSION == 1
        if(check_bridge_retransmissions(data))
        {
            return;//it is not directed to this node and just retransmitted
        }
    #endif
    //--------------- retransmission check -------------------
    if((data[0] & 0xF0) == 0xD0)
    {
        uint8_t ttl = data[1] & 0x0F;
        handler->pser->printf("RTX:%d;",ttl);
        data+=2;//retransmission header is removed, data[0] now have the beginning of the retransmission payload
    }
    //--------------- size check -------------------
    if(data[0] > 30)//failure, crc should not be checked is unsafe
    {
        handler->pser->printf("rx size Fail:%d\r",data[0]);
        return;
    }
    //--------------- crc check -------------------
    if(!crc::check(data))
    {
        handler->pser->printf("rx crc Fail\r");//TODO print tab in util
        uint8_t print_size = data[0];
        if(print_size>30)
        {
            print_size = 30;
        }
        print_tab(handler->pser,data,print_size);
        return;
    }
    uint8_t user_size = data[0];
    if((data[rfi_pid] & mesh::p2p::BROADCAST_MASK) == mesh::p2p::BROADCAST_MASK)
    {
        //we catched a broadcast, forward it to the user as such
        //handler->pser->printf("call broadcast\r");
        handler->_callbacks[static_cast<int>(RfMesh::CallbackType::Broadcast)](data,user_size);
    }
    else
    {
        //handler->pser->printf("call peer2peer\r");
        rf_peer2peer_handler(data);
    }
}

void rf_peer2peer_handler(uint8_t *data)
{
    RfMesh *handler = (RfMesh*)nrf_handlers[0];

    if(data[rfi_dst] != g_nodeId)
    {
        //handler->pser->printf("not this node id\r");
        return;//as this packet is not directed to us
    }
    if((data[rfi_pid] & mesh::p2p::TYPE_MASK) == mesh::p2p::TYPE_MSQ_ACK)
    {
        if((data[rfi_pid] & mesh::p2p::MESSAGE_MASK) == mesh::p2p::MESSAGE_MASK)
        {
            //send_ack(data);
            //TODO send the acknowledge
            data[rfi_pid]&= 0x01F;// clear bit7, bit6, bit5 and keep id
            //handler->pser->printf("call Message\r");
            handler->_callbacks[static_cast<int>(RfMesh::CallbackType::Message)](data,data[0]);
        }
        else//it's an acknowledge
        {
            //handler->pser->printf("is returned\r");
            isReturned = 1;
        }
    }
    else//it's Request / Response
    {
        if((data[rfi_pid] & mesh::p2p::REQUEST_MASK) == mesh::p2p::REQUEST_MASK)
        {
            //handler->pser->printf("call request\r");
            handler->_callbacks[static_cast<int>(RfMesh::CallbackType::Request)](data,data[0]);
        }
        else
        {
            //handler->pser->printf("call response\r");
            isReturned = 1;
            handler->_callbacks[static_cast<int>(RfMesh::CallbackType::Response)](data,data[0]);
        }
    }
    if(isReturned)
    {
        if((p2p_expected_Pid & 0x1F) == (data[rfi_pid] & 0x1F) && (data[rfi_dst] == g_nodeId)  )
        {
            //handler->pser->printf("p2p_ack :ok\r");
            p2p_ack = true;//notify the re-transmitter
        }
    }
}


RfMesh::RfMesh(Serial *ps,PinName ce, PinName csn, PinName sck, PinName mosi, PinName miso,PinName irq):
                            //1:Gnd, 2:3.3v, 3:ce, 4:csn, 5:sck, 6:mosi, 7:miso, 8:irq
                            nrf(ps, ce, csn, sck, mosi, miso),
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
    nRFIrq.enable_irq();

    //enable Rx DR interrupt
    nrf.clearbit(nrf::reg::CONFIG,nrf::bit::config::MASK_RX_DR);

    //pser->printf("Start listening\r\n");
    nrf.setMode(nrf::Mode::Rx);
    
}


void RfMesh::attach(Callback<void(uint8_t *data,uint8_t size)> func,RfMesh::CallbackType type)
{
    _callbacks[static_cast<int>(type)] = func;
}

void RfMesh::print_nrf()
{
    nrf.dump_regs();
    nrf.print_info();
    pser->printf("irq pin :%d\n",nRFIrq.read());
    pser->printf("ce pin  :%d\n",nrf.ce_pin.read());
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


bool RfMesh::send_check_ack()
{
    p2p_ack = false;
    p2p_expected_Pid = p2p_message[rfi_pid];//Pid
	nrf.transmit_Rx(p2p_message,p2p_message[rfi_size]+2);
	wait_ms(p2p_ack_delay);// >>> Timeout important, might depend on Nb briges, and on the ReqResp or just MsgAck
    //pser->printf("p2p_ack :%d\r",p2p_ack);
    return p2p_ack;
}

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

uint8_t RfMesh::send_rgb(uint8_t dest,uint8_t r,uint8_t g,uint8_t b)
{
    p2p_message[rfi_size] = 7;
    p2p_message[rfi_pid] =  mesh::p2p::BIT7_DIRECTED    | mesh::p2p::BIT6_MSGACK | 
                            mesh::p2p::BIT5_MESSAGE     | rf_pid_rgb;
    p2p_message[rfi_src] = g_nodeId;
    p2p_message[rfi_dst] = dest;
    p2p_message[4] = r;
    p2p_message[5] = g;
    p2p_message[6] = b;
    crc::set(p2p_message);
    //print_tab(pser,p2p_message,9);
    return send_retries();
}

void RfMesh::broadcast_reset()
{
    brc_message[rfi_size] = 3;
    brc_message[rfi_pid] =  mesh::p2p::BIT7_BROADCAST | rf_pid_0xC9_reset;
    brc_message[rfi_src] = g_nodeId;
    crc::set(brc_message);
    //print_tab(pser,brc_message,5);
    nrf.transmit_Rx(brc_message,brc_message[rfi_size]+2);
}