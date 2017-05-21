
#include "nrf24l01p.h"


Nrf24l01p::Nrf24l01p(PinName ce, PinName csn, PinName sck, PinName mosi, PinName miso):
                spi(mosi,miso,sck),
                csn_pin(csn),
                ce_pin(ce),
                mode(nrf::Mode::Uninitialised)
{

}

void ce_pin_highEnable()
{
    ce_pin = 1;
    wait_us(nrf::delay::Tpece2csn_us);
}
void ce_pin_lowDisable()
{
    ce_pin = 0;
}

void Nrf24l01p::csn_pin_lowSelect()
{
    csn_pin = 0;
}
void csn_pin_highClear()
{
    csn_pin = 1;
}

uint8_t Nrf24l01p::writeRegister(uint8_t reg,uint8_t val)
{
    uint8_t res = 0;
    uint8_t last_ce = ce_pin;

    ce_pin_lowDisable();
    csn_pin_lowSelect();

    res = spi.write(nrf::cmd::WRITE_REG | reg);
    spi.write(val);

    csn_pin_highClear();
    if(last_ce)
    {
        ce_pin_highEnable();
    }

    wait_us(nrf::delay::Tpece2csn_us);

    return res;
}

uint8_t Nrf24l01p::readRegister(uint8_t reg);
{
    csn_pin_lowSelect();

    spi.write(nrf::cmd::READ_REG | reg);
    uint8_t res_val = spi.write(nrf::cmd::NOP);

    csn_pin_highClear();

    return res_val;
}

uint8_t Nrf24l01p::writeBuffer(uint8_t reg,uint8_t *buf,uint8_t size)
{

}

uint8_t Nrf24l01p::readBuffer(uint8_t reg,uint8_t *buf,uint8_t size)
{

}

uint8_t Nrf24l01p::command(uint8_t reg)
{

}
