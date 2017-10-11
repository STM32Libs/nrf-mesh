
#ifndef __NRFMESH__
#define __NRFMESH__

#include "mbed.h"
#include "nrf24l01p.h"

namespace mesh
{
    namespace p2p
    {
        uint8_t const BROADCAST_MASK    =   0x80;
        uint8_t const TYPE_MASK         =   0x40;
        uint8_t const TYPE_MSQ_ACK      =   0x40;
        uint8_t const MESSAGE_MASK      =   0x20;
        uint8_t const REQUEST_MASK      =   0x20;

        uint8_t const BIT7_DIRECTED     =   0x00;
        uint8_t const BIT7_BROADCAST    =   0x80;
        uint8_t const BIT6_MSGACK       =   0x40;
        uint8_t const BIT6_REQRESP      =   0x00;
        uint8_t const BIT5_MESSAGE      =   0x20;
    }

}

class RfMesh
{
    public:
    enum class CallbackType {
        Message = 0,
        Broadcast = 1,
        Request = 2,
        Response = 3,
        TxComplete = 4,
        Sniff = 5,
        CallbackCnt = 6
    };

    public:
        RfMesh(Serial *ps,PinName ce, PinName csn, PinName sck, PinName mosi, PinName miso,PinName irq = NC);
        void init();
        void print_nrf();
        void attach(Callback<void(uint8_t *data,uint8_t size)> func,CallbackType type);
        void setNodeId(uint8_t nid);
        void setRetries(uint8_t nb_retries);
        void setAckDelay(uint16_t delay);
        //---------------------------- Communication ------------------------
    public:
        uint8_t send_rgb(uint8_t dest,uint8_t r,uint8_t g,uint8_t b);
        void broadcast_reset();
        private:
        bool send_check_ack();
        uint8_t send_retries();
        //-------------------------------------------------------------------
public:
        Nrf24l01p   nrf;
        Serial      *pser;
        Callback<void(uint8_t *data,uint8_t size)> _callbacks[static_cast<int>(CallbackType::CallbackCnt)];
        InterruptIn nRFIrq;
};

#endif /*__NRFMESH__*/
