/** @file rf_protocol.h
 *
 * @author Wassim FILALI
 *
 * @compiler GCC for STM32
 *
 *
 * $Date: 29.10.2016 - creation
 * $Date: 08.10.2017 - takeover for the stm32 with mbed
 * $Date: 18.11.2017 - Change to Mesh Protocol 2.0 (separate control and pid)
 * $Revision: 3
 *
*/

#include "mbed.h"

namespace rf
{
    namespace ind
    {
        uint8_t const size      = 0;
        uint8_t const control   = 1;
        uint8_t const pid       = 2;
        uint8_t const source    = 3;
        uint8_t const dest      = 4;
        uint8_t const bcst_payload  = 4;
        uint8_t const p2p_payload  = 5;
    }

    namespace ctr
    {
        //bit 7
        uint8_t const Broadcast     = 0x80;
        uint8_t const Peer2Peer     = 0x00;
        //bit 6
        uint8_t const Msg_Ack     = 0x40;
        uint8_t const ReqResp     = 0x00;
        //bit 5
        uint8_t const Message     = 0x20;
        uint8_t const Acknowledge = 0x00;
        //bit 5
        uint8_t const Request     = 0x20;
        uint8_t const Response    = 0x00;
        //bit 4
        uint8_t const Send_Ack    = 0x10;
        uint8_t const No_Ack      = 0x00;
        //bits 3-0
        uint8_t const ttl_mask    = 0x0F;
        uint8_t const ttl_clear   = 0xF0;
    }

    namespace pid
    {
        uint8_t const ping          = 0x01;
        uint8_t const request_pid   = 0x02;
        uint8_t const chan_switch   = 0x03;
        uint8_t const reset         = 0x04;
        uint8_t const alive         = 0x05;
        uint8_t const button        = 0x06;
        uint8_t const light         = 0x07;//light sensing 16-bit
        uint8_t const temperature   = 0x08;//is a temperature sensor
        uint8_t const heat          = 0x09;//is temperature heating order
        uint8_t const bme280        = 0x0A;
        uint8_t const rgb           = 0x0B;
        uint8_t const magnet        = 0x0C;
        uint8_t const dimmer        = 0x0D;//action of changing light intensity
        uint8_t const light_rgb     = 0x0E;//sensing light in luminance and colors 4x(16-bit / channel)
        uint8_t const gesture       = 0x0F;//APDS gesture sensor
    }

    namespace gest
    {
        uint8_t const none  = 0x00;
        uint8_t const left  = 0x01;
        uint8_t const right = 0x02;
        uint8_t const up    = 0x03;
        uint8_t const down  = 0x04;
        uint8_t const near  = 0x05;
        uint8_t const far   = 0x06;
        uint8_t const all   = 0x07;
    }
}

class Proto
{
    public:
        Proto(Serial *ps);
        void print_light_rgb(uint8_t *rxPayload);
        void fill_light_paylod(uint16_t light,uint8_t *rxPayload);
        void print_light(uint8_t *rxPayload);
        void print_magnet(uint8_t *rxPayload);
        void print_bme280(uint8_t *rxPayload);
    public:
        Serial      *pser;
};
