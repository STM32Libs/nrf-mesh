
#ifndef __NRFMESH_H__
#define __NRFMESH_H__

#include "mbed.h"
#include "nRF24L01P.h"

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
        RfMesh(Serial *ps,PinName irq = NC);
        void init();
        void print_nrf();
        void attach(Callback<void()> func,CallbackType type);
        void enable_nrf_rx_interrupt();
    public:
        nRF24L01P   nrf;
        Serial      *pser;
        Callback<void()> _callbacks[CallbackCnt];
        InterruptIn nRFIrq;
};

#endif /*__NRFMESH_H__*/
