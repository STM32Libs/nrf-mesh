
#ifndef __NRFMESH_H__
#define __NRFMESH_H__

#include "mbed.h"
#include "nRF24L01P.h"

class RfMesh
{
    public:
        RfMesh(Serial *ps);
        void init();
        void print_nrf();
        void print_nrf2();
    public:
        nRF24L01P   nrf;
        Serial      *pser;
};

#endif /*__NRFMESH_H__*/
