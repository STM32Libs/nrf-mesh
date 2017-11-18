
#ifndef __NRFMESH__
#define __NRFMESH__

#include "mbed.h"
#include "nrf24l01p.h"

namespace mesh
{

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
        void init(uint8_t chan);
        void print_nrf();
        void attach(Callback<void(uint8_t *data,uint8_t size)> func,CallbackType type);
        void setNodeId(uint8_t nid);
        void setRetries(uint8_t nb_retries);
        void setAckDelay(uint16_t delay);
        void setBridgeMode();
        //---------------------------- Communication ------------------------
    public:
        void broadcast(uint8_t pid);
        void broadcast_heat(uint8_t heat);
        uint8_t send_msg(uint8_t* buf);
        uint8_t send_rgb(uint8_t dest,uint8_t r,uint8_t g,uint8_t b);
        uint8_t send_heat(uint8_t dest,uint8_t val);
        void send_ack(uint8_t *data);
    private:
        bool send_check_ack();
        uint8_t send_retries();
        //-------------------------------------------------------------------
public:
        Nrf24l01p   nrf;
        Serial      *pser;
        Callback<void(uint8_t *data,uint8_t size)> _callbacks[static_cast<int>(CallbackType::CallbackCnt)];
        InterruptIn nRFIrq;
        bool        isBridge;
};

#endif /*__NRFMESH__*/
