
#ifndef __NRFMESH__
#define __NRFMESH__

#include "mbed.h"
#include "nrf24l01p.h"

namespace mesh
{

}

void rf_bridge_delegate();

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
        RfMesh(Serial *ps,uint8_t spi_mod,PinName ce, PinName csn, PinName sck, PinName mosi, PinName miso,PinName irq = NC);
        void init(uint8_t chan);
        void print_nrf();
        void attach(Callback<void(uint8_t *data,uint8_t size)> func,CallbackType type);
        void setNodeId(uint8_t nid);
        void setRetries(uint8_t nb_retries);
        uint8_t getRetries();
        void setAckDelay(uint16_t delay);
        void setBridgeMode();

        //---------------------------- Communication ------------------------
    public:
        void broadcast(uint8_t pid,uint8_t ttl=1);
        void broadcast_byte(uint8_t v_pid,uint8_t val,uint8_t ttl=1);
        void broadcast_int16(uint8_t v_pid,int16_t val,uint8_t ttl=1);
        void broadcast_int32(uint8_t v_pid,int32_t val,uint8_t ttl=1);
        void broadcast_light_rgb(uint16_t *lrgb,uint8_t ttl=1);
        uint8_t send_msg(uint8_t* buf);
        uint8_t send_rgb(uint8_t dest,uint8_t r,uint8_t g,uint8_t b,bool ask_for_ack=true,uint8_t ttl=1);
        uint8_t send_byte(uint8_t pid,uint8_t dest,uint8_t val,bool ask_for_ack=true,uint8_t ttl=1);
        uint8_t send_cmd_byte(uint8_t dest,uint8_t cmd_id,uint8_t param);
        void send_ack(uint8_t *data,uint8_t ttl=1);
        uint8_t send_request(uint8_t pid,uint8_t dest,uint8_t ttl=1);
        uint8_t send_pid(uint8_t pid,uint8_t dest,uint8_t ttl=1);
        //---------------------------- RF signal Test ------------------------
        //test_rf to be called from main() context
        uint8_t test_rf(uint8_t target,uint8_t channel,uint8_t nb_ping=100);
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
