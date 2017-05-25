
#ifndef __NRFMESH__
#define __NRFMESH__

#include "mbed.h"
#include "nrf24l01p.h"

class RfMesh
{
    public:
    enum class CallbackType {
        Message = 0,
        Request = 1,
        TxComplete = 2,
        CallbackCnt = 3
    };

    public:
        RfMesh(Serial *ps,PinName ce, PinName csn, PinName sck, PinName mosi, PinName miso,PinName irq = NC);
        void init();
        void print_nrf();
        void attach(Callback<void(uint8_t *data,uint8_t size)> func,CallbackType type);
    public:
        Nrf24l01p   nrf;
        Serial      *pser;
        Callback<void(uint8_t *data,uint8_t size)> _callbacks[static_cast<int>(CallbackType::CallbackCnt)];
        InterruptIn nRFIrq;
};

#endif /*__NRFMESH__*/
