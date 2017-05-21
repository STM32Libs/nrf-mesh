
#ifndef __NRFMESH__
#define __NRFMESH__

#include "mbed.h"
#include "nrf24l01p.h"

class RfMesh
{
    public:
    enum CallbackType {
        Message = 0,
        Request,
        TxComplete,
        CallbackCnt
    };

    public:
        RfMesh(Serial *ps,PinName ce, PinName csn, PinName sck, PinName mosi, PinName miso,PinName irq = NC);
        void init();
        void print_nrf();
        void nrf_print_status();
        void attach(Callback<void()> func,CallbackType type);
    public:
        Nrf24l01p   nrf;
        Serial      *pser;
        Callback<void()> _callbacks[CallbackCnt];
        InterruptIn nRFIrq;
};

#endif /*__NRFMESH__*/
