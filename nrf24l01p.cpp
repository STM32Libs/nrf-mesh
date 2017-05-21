
#include "nrf24l01p.h"


Nrf24l01p::Nrf24l01p(PinName ce, PinName csn, PinName sck, PinName mosi, PinName miso):
                spi(mosi,miso,sck),
                csn_pin(csn),
                ce_pin(ce),
                mode(nrf::Mode::Uninitialised)
{

}

//--------------------- Level 0 ----------------------------
void Nrf24l01p::ce_pin_highEnable()
{
    ce_pin = 1;
    wait_us(nrf::delay::Tpece2csn_us);
}
void Nrf24l01p::ce_pin_lowDisable()
{
    ce_pin = 0;
}

void Nrf24l01p::csn_pin_lowSelect()
{
    csn_pin = 0;
}
void Nrf24l01p::csn_pin_highClear()
{
    csn_pin = 1;
}

//--------------------- Level 1 ----------------------------
void Nrf24l01p::command(uint8_t v_cmd)
{
    uint8_t res = 0;
    uint8_t last_ce = ce_pin;

    ce_pin_lowDisable();
    csn_pin_lowSelect();

    res = spi.write(v_cmd);

    csn_pin_highClear();
    if(last_ce)
    {
        ce_pin_highEnable();
    }

    wait_us(nrf::delay::Tpece2csn_us);

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

uint8_t Nrf24l01p::readRegister(uint8_t reg)
{
    csn_pin_lowSelect();

    spi.write(nrf::cmd::READ_REG | reg);
    uint8_t res_val = spi.write(nrf::cmd::NOP);

    csn_pin_highClear();

    return res_val;
}

uint8_t Nrf24l01p::readStatus()
{
    csn_pin_lowSelect();
    uint8_t res_val = spi.write(nrf::cmd::NOP);
    csn_pin_highClear();
    return res_val;
}

void Nrf24l01p::setbit(uint8_t reg, uint8_t bit)
{
    uint8_t val = readRegister(reg);
    val |= bit;
    writeRegister(reg,val);
}

void Nrf24l01p::clearbit(uint8_t reg, uint8_t bit)
{
    uint8_t val = readRegister(reg);
    val &= ~bit;
    writeRegister(reg,val);
}

void Nrf24l01p::writeBuffer(uint8_t add,uint8_t *buf,uint8_t size)
{
    uint8_t last_ce = ce_pin;

    ce_pin_lowDisable();
    csn_pin_lowSelect();

    spi.write(add);
    for(int i=0;i<size;i++)
    {
        spi.write(*buf++);
    }

    csn_pin_highClear();
    if(last_ce)
    {
        ce_pin_highEnable();
    }

    wait_us(nrf::delay::Tpece2csn_us);

}

void Nrf24l01p::readBuffer(uint8_t add,uint8_t *buf,uint8_t size)
{
    csn_pin_lowSelect();

    spi.write(add);
    for(int i=0;i<size;i++)
    {
        (*buf++) = spi.write(nrf::cmd::NOP);
    }

    csn_pin_highClear();

}

//--------------------- Level 2 - Modes ----------------------------
void Nrf24l01p::setMode(nrf::Mode m)
{
    switch(m)
    {
        case nrf::Mode::Standby :
        {
            setbit(nrf::reg::CONFIG,nrf::bit::CONFIG_PWR_UP);
            wait_us(nrf::delay::Tpd2stby_us);
            mode = nrf::Mode::Standby;
        }
        break;
        case nrf::Mode::PowerDown :
        {
            clearbit(nrf::reg::CONFIG,nrf::bit::CONFIG_PWR_UP);
            mode = nrf::Mode::PowerDown;
        }
        break;
        case nrf::Mode::Rx :
        {
            if(mode == nrf::Mode::PowerDown)
            {
                setMode(nrf::Mode::Standby);
            }
            setbit(nrf::reg::CONFIG,nrf::bit::CONFIG_PRIM_RX);
            mode = nrf::Mode::Rx;
        }
        break;
        case nrf::Mode::Tx :
        {
            if(mode == nrf::Mode::PowerDown)
            {
                setMode(nrf::Mode::Standby);
            }
            clearbit(nrf::reg::CONFIG,nrf::bit::CONFIG_neg_PRIM_RX);//Rx cleared => Tx mode
            mode = nrf::Mode::Tx;
        }
        break;
        default:
        {
            //? What to do ?
        }
        break;
        
    }
}
